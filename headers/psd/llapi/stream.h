
#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>

#include <initializer_list>
#include <iterator>
#include <map>
#include <psd/error.h>
#include <type_traits>
#include <file/input.h>
#include <file/output.h>
#include <utility>
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

class Stream;

template <typename T>
struct FromStreamFn;

template <typename T>
struct ToStreamFn;

using U8  = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;
using I8  = std::int8_t;
using I16 = std::int16_t;
using I32 = std::int32_t;
using I64 = std::int64_t;
using F32 = float;
using F64 = double;

namespace detail {
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
   std::is_same_v<IteratorCategory<T>, std::random_access_iterator_tag>);

} // namespace detail
class Stream {
  template <typename V, typename T, typename F, typename... A>
  struct FromStreamFnOperatorImplemented_ {
    static constexpr bool Value = false;
  };
  template <typename T, typename F, typename... A>
  struct FromStreamFnOperatorImplemented_<std::void_t<decltype(std::declval<F>()(
    std::declval<Stream &>(),
    std::declval<T &>(),
    std::declval<std::conditional_t<std::is_fundamental_v<T>, A, const A &>>()...
  ))>, T, F, A...> {
    static constexpr bool Value = true;
  };
  template <typename T, typename F, typename... A>
  static constexpr bool FromStreamFnOperatorImplemented =
    FromStreamFnOperatorImplemented_<void, T, F, A...>::Value;

  template <typename V, typename T, typename... A>
  struct FromStreamFnImplemented_ {
    static constexpr bool Value = false;
  };
  template <typename T, typename... A>
  struct FromStreamFnImplemented_<std::void_t<decltype(FromStreamFn<T>())>, T, A...> {
    static constexpr bool Value =
      FromStreamFnOperatorImplemented<T, FromStreamFn<T>, A...>;
  };
  template <typename T, typename... A>
  static constexpr bool FromStreamFnImplemented =
    FromStreamFnImplemented_<void, T, A...>::Value;

  template <typename V, typename T, typename... A>
  struct FromStreamFnImplementedInClass_ {
    static constexpr bool Value = false;
  };
  template <typename T, typename... A>
  struct FromStreamFnImplementedInClass_<std::void_t<decltype(typename T::FromStreamFn())>, T, A...> {
    static constexpr bool Value =
      FromStreamFnOperatorImplemented<T, typename T::FromStreamFn, A...>;
  };
  template <typename T, typename... A>
  static constexpr bool FromStreamFnImplementedInClass =
    FromStreamFnImplementedInClass_<void, T, A...>::Value;

  template <typename V, typename T, typename F, typename... A>
  struct ToStreamFnOperatorImplemented_ {
    static constexpr bool Value = false;
  };
  template <typename T, typename F, typename... A>
  struct ToStreamFnOperatorImplemented_<std::void_t<decltype(std::declval<F>()(
    std::declval<Stream &>(),
    std::declval<const T &>(),
    std::declval<std::conditional_t<std::is_fundamental_v<T>, A, const A &>>()...
  ))>, T, F, A...> {
    static constexpr bool Value = true;
  };
  template <typename T, typename F, typename... A>
  static constexpr bool ToStreamFnOperatorImplemented =
    ToStreamFnOperatorImplemented_<void, T, F, A...>::Value;

  template <typename V, typename T, typename... A>
  struct ToStreamFnImplemented_ {
    static constexpr bool Value = false;
  };
  template <typename T, typename... A>
  struct ToStreamFnImplemented_<std::void_t<decltype(ToStreamFn<T>())>, T, A...> {
    static constexpr bool Value =
      ToStreamFnOperatorImplemented<T, ToStreamFn<T>, A...>;
  };
  template <typename T, typename... A>
  static constexpr bool ToStreamFnImplemented =
    ToStreamFnImplemented_<void, T, A...>::Value;

  template <typename V, typename T, typename... A>
  struct ToStreamFnImplementedInClass_ {
    static constexpr bool Value = false;
  };
  template <typename T, typename... A>
  struct ToStreamFnImplementedInClass_<std::void_t<decltype(typename T::ToStreamFn())>, T, A...> {
    static constexpr bool Value =
      ToStreamFnOperatorImplemented<T, typename T::ToStreamFn, A...>;
  };
  template <typename T, typename... A>
  static constexpr bool ToStreamFnImplementedInClass =
    ToStreamFnImplementedInClass_<void, T, A...>::Value;
public:
  Stream() = default;

  Stream(const std::filesystem::path &path)
    : buffer_(File::From(path)) {}

  Stream(std::vector<U8> data)
    : buffer_(std::move(data)) {}

  Stream(std::initializer_list<U8> data)
    : buffer_(std::move(data)) {}

  template <typename T>
  std::enable_if_t<detail::ByteSwapSupported<T>, T>
  Read() {
    if (Overflow(sizeof(T))) {
      throw Error("Stream::NotEnoughData");
    }
    return detail::SwapLE(Access<T>());
  }
  template <typename T, typename... A>
  std::enable_if_t<
    FromStreamFnImplemented<T, A...> ||
    FromStreamFnImplementedInClass<T, A...>,
    T>
  Read(A&&... arguments) {
    T output;
    return std::move(ReadTo(output, std::forward<A>(arguments)...));
  }
  template <typename T>
  std::enable_if_t<detail::ByteSwapSupported<T>, T&>
  ReadTo(T &output) {
    return output = Read<T>();
  }
  template <typename T, typename... A>
  std::enable_if_t<FromStreamFnImplemented<T, A...>, T&>
  ReadTo(T &output, A&&... arguments) {
    FromStreamFn<T>()(*this, output, std::forward<A>(arguments)...);
    return output;
  }
  template <typename T, typename... A>
  std::enable_if_t<FromStreamFnImplementedInClass<T, A...>, T&>
  ReadTo(T &output, A&&... arguments) {
    typename T::FromStreamFn()(*this, output, std::forward<A>(arguments)...);
    return output;
  }

  template <typename T>
  std::enable_if_t<detail::ByteSwapSupported<T>>
  Write(T value) {
    if (Overflow(sizeof(T))) {
      AdjustBuffer(sizeof(T));
    }
    Access<T>() = detail::SwapLE(value);
  }
  template <typename T, typename... A>
  std::enable_if_t<ToStreamFnImplemented<T, A...>>
  Write(const T &input, A&&... arguments) {
    ToStreamFn<T>()(*this, input, std::forward<A>(arguments)...);
  }
  template <typename T, typename... A>
  std::enable_if_t<ToStreamFnImplementedInClass<T, A...>>
  Write(const T &input, A&&... arguments) {
    typename T::ToStreamFn()(*this, input, std::forward<A>(arguments)...);
  }

  template <typename I>
  std::enable_if_t<
    !ToStreamFnImplemented        <typename I::value_type, I, I> &&
    !ToStreamFnImplementedInClass <typename I::value_type, I, I>>
  Read(I begin, I end) {
    auto distance = std::distance(begin, end);
    if (Overflow(distance)) {
      throw Error("Stream::NotEnoughData");
    }
    using Value = detail::IteratorValue<I>;
    if constexpr (
      std::is_same_v<Value, U8> ||
      std::is_same_v<Value, I8>) {
        auto current = reinterpret_cast<Value *>(buffer_.data() + offset_);
        std::copy(current, current + distance, begin);
        offset_ += distance;
    } else {
      std::for_each(begin, end, [this](auto &value) {
        ReadTo(value);
      });
    }
  }
  template <typename I>
  std::enable_if_t<detail::SupportedIterator<I>>
  Write(I begin, I end) {
    const auto distance = std::distance(begin, end);
    if (Overflow(distance)) {
      AdjustBuffer(distance);
    }
    using Value = detail::IteratorValue<I>;
    if constexpr (
      std::is_same_v<Value, U8> ||
      std::is_same_v<Value, I8>) {
        const auto current = Current<Value>();
        std::copy(begin, end, current);
        offset_ += distance;
    } else {
      std::for_each(begin, end, [this](const auto &value){
        Write(value);
      });
    }
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
  void SetPos(unsigned value) {
    offset_ = value;
  }
  unsigned Pos() const {
    return offset_;
  }
  unsigned Length() const {
    return buffer_.size();
  }
  void Dump(const std::filesystem::path &path) const {
    File::To(buffer_, path);
  }
  void Dump(std::vector<U8> &output) const {
    output = buffer_;
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
template <typename T>
struct FromStreamFn<std::vector<T>> {
  void operator()(Stream &stream, std::vector<T> &output, unsigned length) {
    output.resize(length);
    stream.Read(
      output.begin(),
      output.end()
    );
  }
}; // struct FromStreamFn<std::vector<T>>
template <typename T>
struct ToStreamFn<std::vector<T>> {
  void operator()(Stream &stream, const std::vector<T> &input) {
    stream.Write(
      input.begin(),
      input.end()
    );
  }
}; // struct ToStreamFn<std::vector<T>>
template <typename T>
struct FromStreamFn<std::basic_string<T>> {
  void operator()(Stream &stream, std::basic_string<T> &output, unsigned length) {
    output.resize(length);
    stream.Read(
      output.begin(),
      output.end()
    );
  }
}; // struct FromStreamFn<std::basic_string<T>>
template <typename T>
struct ToStreamFn<std::basic_string<T>> {
  void operator()(Stream &stream, const std::basic_string<T> &input) {
    stream.Write(
      input.begin(),
      input.end()
    );
  }
}; // struct ToStreamFn<std::basic_string<T>>
template <typename F, typename S>
struct FromStreamFn<std::pair<F, S>> {
  void operator()(Stream &stream, std::pair<F, S> &output) {
    stream.ReadTo(output.first);
    stream.ReadTo(output.second);
  }
}; // struct FromStreamFn<std::pair<F, S>>
template <typename F, typename S>
struct ToStreamFn<std::pair<F, S>> {
  void operator()(Stream &stream, const std::pair<F, S> &input) {
    stream.Write(input.first);
    stream.Write(input.second);
  }
}; // struct ToStreamFn<std::pair<F, S>>
template <typename K, typename V>
struct FromStreamFn<std::map<K, V>> {
  void operator()(Stream &stream, std::map<K, V> &output, unsigned length) {
    for (auto index = 0u;
              index < length;
              index++) {
      output[stream.Read<K>()] = stream.Read<V>();
    }
  }
}; // struct FromStreamFn<std::map<K, V>>
template <typename K, typename V>
struct ToStreamFn<std::map<K, V>> {
  void operator()(Stream &stream, const std::map<K, V> &input) {
    for (const auto &pair : input) {
      stream.Write(pair);
    }
  }
}; // struct ToStreamFn<std::map<K, V>>
}; // namespace PSD::llapi
