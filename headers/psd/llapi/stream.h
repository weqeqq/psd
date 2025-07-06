
#include <algorithm>
#include <cstdint>
#include <filesystem>

#include <iterator>
#include <psd/error.h>
#include <type_traits>
#include <file/input.h>
#include <file/output.h>
#include <vector>

#ifdef _MSC_VER
#define PSD_COMPILER_MSVC 1
#else
#define PSD_COMPILER_MSVC 0
#endif

#ifdef __GNUC__
#define PSD_COMPILER_GCC 1
#else
#define PSD_COMPILER_GCC 0
#endif

#ifdef __clang__
#define PSD_COMPILER_CLANG 1
#else
#define PSD_COMPILER_CLANG 0
#endif

namespace PSD::llapi {
//

using U8  = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;
using I8  = std::int8_t;
using I16 = std::int16_t;
using I32 = std::int32_t;
using I64 = std::int64_t;

namespace Detail {
//
template <typename T>
inline constexpr
std::enable_if_t<
  std::is_same_v<T, I16> ||
  std::is_same_v<T, U16>,
  T>
ByteSwap16(T value) {
  #if PSD_COMPILER_GCC || PSD_COMPILER_CLANG
    return __builtin_bswap16(value);
  #elif PSD_COMPILER_MSVC
    return _byteswap_ushort(value);
  #else
    return ((value & 0xFF00) >> 8) |
           ((value & 0x00FF) << 8);
  #endif
}

template <typename T>
inline constexpr
std::enable_if_t<
  std::is_same_v<T, I32> ||
  std::is_same_v<T, U32>,
  T>
ByteSwap32(T value) {
  #if PSD_COMPILER_GCC || PSD_COMPILER_CLANG
    return __builtin_bswap32(value);
  #elif PSD_COMPILER_MSVC
    return _byteswap_ulong(value);
  #else
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x000000FF) << 24);
  #endif
}

template <typename T>
inline constexpr
std::enable_if_t<
  std::is_same_v<T, I64> ||
  std::is_same_v<T, U64>,
  T>
ByteSwap64(T value) {
  #if PSD_COMPILER_GCC || PSD_COMPILER_CLANG
    return __builtin_bswap64(value);
  #elif PSD_COMPILER_MSVC
    return _byteswap_uint64(value);
  #else
    return ((value & 0xFF00000000000000ULL) >> 56) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x000000FF00000000ULL) >> 8) |
           ((value & 0x00000000FF000000ULL) << 8) |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x00000000000000FFULL) << 56);
  #endif
}
template <typename T>
inline constexpr std::enable_if_t<std::is_integral_v<T>, T>
ByteSwap(T value) {
  if constexpr (sizeof(T) == 1) {
    return value;
  } else if constexpr (sizeof(T) == 2) {
    return ByteSwap16(value);
  } else if constexpr (sizeof(T) == 4) {
    return ByteSwap32(value);
  } else if constexpr (sizeof(T) == 8) {
    return ByteSwap64(value);
  } else {
    static_assert(
      sizeof(T) == 1 ||
      sizeof(T) == 2 ||
      sizeof(T) == 4 ||
      sizeof(T) == 8, "Unsupported");
  }
}

template <typename O, typename I>
inline constexpr O MemCast(I value) {
  static_assert(sizeof(O) == sizeof(I), "Unsupported");
  return *reinterpret_cast<O *>(&value);
}

template <typename T>
inline constexpr T MemSwap(T value) {
  if constexpr (sizeof(T) == 1) {
    return value;
  } else if constexpr (sizeof(T) == 2) {
    return MemCast<T>(ByteSwap16(MemCast<std::uint16_t>(value)));
  } else if constexpr (sizeof(T) == 4) {
    return MemCast<T>(ByteSwap32(MemCast<std::uint32_t>(value)));
  } else if constexpr (sizeof(T) == 8) {
    return MemCast<T>(ByteSwap64(MemCast<std::uint64_t>(value)));
  } else {
    static_assert(
      sizeof(T) == 1 ||
      sizeof(T) == 2 ||
      sizeof(T) == 4 ||
      sizeof(T) == 8, "Unsupported");
  }
};
template <typename T>
inline constexpr T SwapLE(T value) {
  #ifdef PSD_LITTLE_ENDIAN
    if constexpr (std::is_floating_point_v<T>) {
      return MemSwap(value);
    } else {
      return ByteSwap(value);
    }
  #else
    return value;
  #endif
}
template <typename T>
static constexpr bool ByteSwapSupported = std::is_integral_v<T> || std::is_floating_point_v<T>;

template <typename T>
using IteratorValue = typename std::iterator_traits<T>::value_type;

template <typename T>
using IteratorCategory = typename std::iterator_traits<T>::iterator_category;

template <typename T>
static constexpr bool SupportedIterator =
  (std::is_same_v<IteratorCategory<T>, std::forward_iterator_tag>       ||
   std::is_same_v<IteratorCategory<T>, std::bidirectional_iterator_tag> ||
   std::is_same_v<IteratorCategory<T>, std::random_access_iterator_tag>) &&
  ByteSwapSupported<IteratorValue<T>>;

} // namespace Detail
class Stream {
public:
  Stream() = default;
  Stream(const std::filesystem::path &path) : buffer_(File::From(path)) {}

  template <typename T>
  std::enable_if_t<Detail::ByteSwapSupported<T>, T>
  Read() {
    if (Overflow(sizeof(T))) {
      throw Error("Stream::NotEnoughData");
    }
    return Detail::SwapLE(Access<T>());
  }

  template <typename T>
  std::enable_if_t<Detail::ByteSwapSupported<T>>
  Write(T value) {
    if (Overflow(sizeof(T))) {
      AdjustBuffer(sizeof(T));
    }
    Access<T>() = Detail::SwapLE(value);
  }
  template <typename I>
  std::enable_if_t<Detail::SupportedIterator<I>>
  Read(I begin, I end) {
    auto distance = std::distance(begin, end);
    if (Overflow(distance)) {
      throw Error("Stream::NotEnoughData");
    }
    using Value = Detail::IteratorValue<I>;
    if constexpr (
      std::is_same_v<Value, U8> ||
      std::is_same_v<Value, I8>) {
        auto current = reinterpret_cast<Value *>(buffer_.data() + offset_);
        std::copy(current, current + distance, begin);
    } else {
      for (; begin != end; ++begin) *begin = Read<Value>();
    }
    offset_ += distance;
  }
  template <typename I>
  std::enable_if_t<Detail::SupportedIterator<I>>
  Write(I begin, I end) {
    const auto distance = std::distance(begin, end);
    if (Overflow(distance)) {
      AdjustBuffer(distance);
    }
    using Value = Detail::IteratorValue<I>;
    if constexpr (
      std::is_same_v<Value, U8> ||
      std::is_same_v<Value, I8>) {
        const auto current = Current<Value>();
        std::copy(begin, end, current);
    } else {
      std::for_each(begin, end, [this](const auto &value){
        Write(value);
      });
    }
    offset_ += distance;
  }

  void operator+=(unsigned value) {
    offset_ += value;
  }
  void operator-=(unsigned value) {
    offset_ -= value;
  }
  void operator++() {
    offset_++;
  }
  void operator++(int) {
    offset_++;
  }
  void operator--() {
    offset_--;
  }
  void operator--(int) {
    offset_--;
  }
  void SetOffset(unsigned value) {
    offset_ = value;
  }
private:
  std::vector<U8> buffer_; unsigned offset_ = 0;

  void AdjustBuffer(unsigned length) {
    buffer_.resize(offset_ + length);
  }

  bool Overflow(unsigned required) {
    return offset_ + required > buffer_.size();
  }

  template <typename T>
  T *Current() {
    return reinterpret_cast<T *>(buffer_.data() + offset_);
  }
  template <typename T>
  T &Access() {
    return *reinterpret_cast<T *>(
      buffer_.data() + (offset_ += sizeof(T)) - sizeof(T)
    );
  }
};
}; // namespace PSD::llapi
