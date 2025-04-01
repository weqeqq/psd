
#pragma once

#include <iostream>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <codecvt>
#include <locale>
#include <utility>
#include <vector>
#include <tuple>
#include <map>
#include <unordered_map>
#include <stdexcept>
#include <array>
#include <image/convertor/endianness.h>

namespace PSD {

class Stream;

template <typename Te>
class Reader {
public:

  Reader(Stream &) {
    static_assert(false);
  }

}; // Reader

#define PSD_REGISTER_READER_FOR_TYPE(TypeName, ClassName) \
  template <>                                             \
  class Reader<TypeName> : public ClassName::Reader {     \
  public:                                                 \
    Reader(Stream &stream) : ClassName::Reader(stream) {} \
  }

#define PSD_REGISTER_READER_FOR_BUFFER(ClassName)                                                         \
  template <Depth::Tp DepthV, Color::Tp ColorV>                                                           \
  class Reader<ClassName<DepthV, ColorV>> : public ClassName<DepthV, ColorV>::Reader {                    \
  public:                                                                                                 \
    explicit Reader(Stream &stream) : ClassName<DepthV, ColorV>::Reader(stream) {} \
  }

#define PSD_REGISTER_READER(ClassName) \
  PSD_REGISTER_READER_FOR_TYPE(ClassName, ClassName)

template <typename Te>
class Writer {
public:

  Writer(Stream &, const Te &) {
    static_assert(false);
  }

}; // Writer 

#define PSD_REGISTER_WRITER_FOR_TYPE(TypeName, ClassName)                               \
  template <>                                                                           \
  class Writer<TypeName> : public ClassName::Writer {                                   \
  public:                                                                               \
    explicit Writer(Stream &stream, const TypeName &input) : ClassName::Writer(stream, input) {} \
  }

#define PSD_REGISTER_WRITER_FOR_BUFFER(ClassName)                                                         \
  template <Depth::Tp DepthV, Color::Tp ColorV>                                                           \
  class Writer<ClassName<DepthV, ColorV>> : public ClassName<DepthV, ColorV>::Writer {                    \
  public:                                                                                                 \
    explicit Writer(Stream &stream, const ClassName<DepthV, ColorV> &input) : ClassName<DepthV, ColorV>::Writer(stream, input) {} \
  }

#define PSD_REGISTER_WRITER(ClassName) \
  PSD_REGISTER_WRITER_FOR_TYPE(ClassName, ClassName)

template <typename... As>
static std::tuple<As&&...> CrTuple(As&&... args) {
  return std::tuple<As&&...>(std::forward<As>(args)...);
};

class Stream {
private:

  template <typename Te>
  struct IsStdTpS {
    static constexpr bool Value = std::is_floating_point_v<Te> || std::is_integral_v<Te>; 
  };
  template <typename Te>
  static constexpr bool IsStdTp = IsStdTpS<Te>::Value;

  template <typename Te, typename = void, typename... As>
  struct HasReaderImplSe {
    static constexpr bool Value = false;
  };
  template <typename Te, typename... As>
  static constexpr bool HasReaderImpl = HasReaderImplSe<Te, void, As...>::Value;

  template <typename Te, typename... As>
  struct CanBeReadedSe {
    static constexpr bool Value = IsStdTp<Te> || HasReaderImpl<Te, As...>;
  };
  template <typename Te, typename... As>
  static constexpr bool CanBeReaded = CanBeReadedSe<Te, As...>::Value;

  template <typename Te, typename = void, typename... As>
  struct HasWriterImplSe {
    static constexpr bool Value = false;
  };
  template <typename Te, typename... As>
  static constexpr bool HasWriterImpl = HasWriterImplSe<Te, void, As...>::Value;

  template <typename Te, typename... As>
  struct CanBeWrittenSe {
    static constexpr bool Value = IsStdTp<Te> || HasWriterImpl<Te>;
  };
  template <typename Te, typename... As>
  static constexpr bool CanBeWritten = CanBeWrittenSe<Te, As...>::Value;

  template <typename Te>
  struct IsVectorSe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool IsVector = IsVectorSe<Te>::Value;

  template <typename Te>
  struct IsArraySe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool IsArray = IsArraySe<Te>::Value;

  template <typename Te>
  struct IsPairSe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool IsPair = IsPairSe<Te>::Value;

  template <typename Te>
  struct IsTupleSe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool IsTuple = IsTupleSe<Te>::Value;

  template <typename Te, typename = void>
  struct HasHashImplSe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool HasHashImpl = HasHashImplSe<Te>::Value;

  template <typename Te>
  struct IsMapSe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool IsMap = IsMapSe<Te>::Value;

  template <typename Te>
  struct IsUnorderedMapSe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool IsUnorderedMap = IsUnorderedMapSe<Te>::Value;

  template <typename Te>
  struct IsStringSe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool IsString = IsStringSe<Te>::Value;

  template <typename Te>
  struct IsCharSe {
    static constexpr bool Value = false;
  };
  template <typename Te>
  static constexpr bool IsChar = IsCharSe<Te>::Value;

public:

  Stream() = default;
  Stream(std::vector<std::uint8_t> buffer) : buffer_(std::move(buffer)) {}
  Stream(const char *path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
      throw std::runtime_error("failed");
    } 
    file.seekg(0, std::ios::end);
    buffer_.resize(file.tellg());
    file.seekg(0, std::ios::beg);

    file.read(reinterpret_cast<char *>(buffer_.data()), buffer_.size());
    file.close();
  }
  Stream(const std::string &path) : Stream(path.c_str()) {}

  std::uint64_t GetPos() const {
    return pos_;
  }
  std::uint64_t GetLength() const {
    return buffer_.size();
  }

  void IncPos() {
    pos_++;
  }
  void SetPos(std::uint64_t pos) {
    pos_ = pos;
  }
  void AdjustPos(std::uint64_t pos) {
    pos_ += pos;
  }
  void Rewind() {
    pos_ = 0;
  }

  template <typename OTe>
  std::enable_if_t<IsStdTp<OTe>, OTe> Read() {
    OTe output = *reinterpret_cast<OTe *>(buffer_.data() + pos_);
    AdjustPos(sizeof output);
    if constexpr (Image::Endianness::Determine() == Image::Endianness::Big) {
      return output;
    } else {
      return Image::EndiannessConvertor(output).template Convert<Image::Endianness::Big>();
    }
  }
  template <typename OTe, void *Unused = nullptr, typename... As>
  std::enable_if_t<HasReaderImpl<OTe, As...>, OTe>
  Read(std::tuple<As...> tuple = std::tuple()) {
    OTe output;
    std::apply([&](auto&&... tuple_args){
      output = Reader<OTe>(*this).Read(std::forward<decltype(tuple_args)>(tuple_args)...);
    }, tuple);
    return output;
  }
  template <typename OTe>
  std::enable_if_t<IsArray<OTe> || IsPair<OTe> || IsTuple<OTe>, OTe>
  Read() {
    OTe output;
    std::apply([&](auto&... args){ 
      ((args = Read<std::decay_t<decltype(args)>>()), ...); 
    }, output);
    return output;
  }
  template <typename OTe, std::size_t Se> 
  std::enable_if_t<CanBeReaded<OTe>, std::array<OTe, Se>> 
  Read() {
    return Read<std::array<OTe, Se>>();
  }
  template <typename FTe, typename STe> 
  std::enable_if_t<CanBeReaded<FTe> && CanBeReaded<STe>, std::pair<FTe, STe>>
  Read() {
    return Read<std::pair<FTe, STe>>();
  }
  template <typename FTe, typename STe, typename... Or>
  std::enable_if_t<CanBeReaded<FTe> && CanBeReaded<STe> && ((CanBeReaded<Or>), ...), std::tuple<FTe, STe, Or...>>
  Read() {
    return Read<std::tuple<FTe, STe, Or...>>();
  }
  template <typename OTe>
  std::enable_if_t<IsVector<OTe>, OTe>
  Read(std::uint64_t length) {
    OTe output(length);
    for (auto &value : output) {
      value = Read<typename OTe::value_type>();
    }
    return output;
  }
  template <typename VTe>
  std::enable_if_t<CanBeReaded<VTe> && !IsChar<VTe>, std::vector<VTe>>
  Read(std::uint64_t length) {
    return Read<std::vector<VTe>>(length);
  }
  template <typename OTe>
  std::enable_if_t<IsMap<OTe> || IsUnorderedMap<OTe>, OTe> 
  Read(std::uint64_t length) {
    OTe output;
    for (auto index = 0u; 
              index < length;
              index++) {
      output.insert(Read<typename OTe::key_type, typename OTe::mapped_type>());
    }
    return output;
  }
  template <typename KTe, typename VTe>
  std::enable_if_t<CanBeReaded<KTe> && CanBeReaded<VTe> && !HasHashImpl<KTe>, std::map<KTe, VTe>>
  Read(std::uint64_t length) {
    return Read<std::map<KTe, VTe>>(length);
  }
  template <typename KTe, typename VTe>
  std::enable_if_t<CanBeReaded<KTe> && CanBeReaded<VTe> && HasHashImpl<KTe>, std::unordered_map<KTe, VTe>>
  Read(std::uint64_t length) {
    return Read<std::unordered_map<KTe, VTe>>(length);
  }
  template <typename OTe>
  std::enable_if_t<IsString<OTe> && CanBeReaded<typename OTe::value_type>, std::string>
  Read(std::uint64_t length) {
    OTe output(length, 0);
    for (auto &value : output) {
      value = Read<typename OTe::value_type>();
    }
    if constexpr (!std::is_same_v<typename OTe::value_type, char>) {
      return std::wstring_convert<std::codecvt_utf8<
        typename OTe::value_type>, 
        typename OTe::value_type>().to_bytes(output);
    } else {
      return output;
    };
  }
  template <typename CTe>
  std::enable_if_t<IsChar<CTe>, std::string> 
  Read(std::uint64_t length) {
    return Read<std::basic_string<CTe>>(length);
  }

  template <typename ITe>
  std::enable_if_t<IsStdTp<ITe>> 
  Write(ITe input) {
    if constexpr (Image::Endianness::Determine() == Image::Endianness::Big) {
      for (auto index = 0u;
                index < sizeof input;
                index++) {
        buffer_.push_back(reinterpret_cast<std::uint8_t *>(&input)[index]);
      }
    } else {
      for (auto index = std::int64_t(sizeof input - 1);
                index >= 0;
                index--) {
        buffer_.push_back(reinterpret_cast<std::uint8_t *>(&input)[index]);
      }
    }
    AdjustPos(sizeof input);
  }
  template <typename ITe, typename... As>
  std::enable_if_t<HasWriterImpl<ITe, As...>>
  Write(const ITe &input, std::tuple<As...> tuple = {}) {
    std::apply([&](auto&&... tuple_args) {
      Writer<ITe>(*this, input).Write(std::forward<decltype(tuple_args)>(tuple_args)...);
    }, tuple);
  }
  template <typename ITe>
  std::enable_if_t<IsArray<ITe> || IsPair<ITe> || IsTuple<ITe>> 
  Write(const ITe &input) {
    std::apply([&](const auto&... args) {
      (Write(args), ...);
    }, input);
  }
  template <typename VTe, std::uint64_t Se> 
  std::enable_if_t<CanBeWritten<VTe>>
  Write(const std::array<VTe, Se> &input) {
    Write<std::array<VTe, Se>>(input);
  }
  template <typename FTe, typename STe>
  std::enable_if_t<CanBeWritten<FTe> && CanBeWritten<STe>>
  Write(const std::pair<FTe, STe> &input) {
    Write<std::pair<FTe, STe>>(input);
  }
  template <typename FTe, typename STe, typename... OTe>
  std::enable_if_t<CanBeWritten<FTe> && CanBeWritten<STe> && ((CanBeWritten<OTe>), ...)>
  Write(const std::tuple<FTe, STe, OTe...> &input) {
    Write<std::tuple<FTe, STe, OTe...>>(input);
  }
  template <typename ITe>
  std::enable_if_t<IsVector<ITe> || IsMap<ITe> || IsUnorderedMap<ITe>>
  Write(const ITe &input) {
    for (const auto &value : input) {
      Write(value);
    }
  }
  template <typename VTe> 
  std::enable_if_t<CanBeWritten<VTe> && !IsChar<VTe>>
  Write(const std::vector<VTe> &input) {
    Write<std::vector<VTe>>(input);
  }
  template <typename KTe, typename VTe>
  std::enable_if_t<CanBeWritten<KTe> && CanBeWritten<VTe> && !HasHashImpl<KTe>>
  Write(const std::map<KTe, VTe> &input) {
    Write<std::map<KTe, VTe>>(input);
  }
  template <typename KTe, typename VTe>
  std::enable_if_t<CanBeWritten<KTe> && CanBeWritten<VTe> && HasHashImpl<KTe>>
  Write(const std::unordered_map<KTe, VTe> &input) {
    Write<std::unordered_map<KTe, VTe>>(input);
  }
  template <typename ITe>
  std::enable_if_t<IsString<ITe>>
  Write(const std::string &input) {
    ITe string;

    if constexpr (!std::is_same<typename ITe::value_type, char>::value) {
      string = std::wstring_convert<std::codecvt_utf8<
        typename ITe::value_type>,
        typename ITe::value_type>().from_bytes(input);
    } else {
      string = input;
    }
    for (const auto &character : string) Write(character);
  }
  template <typename CTe>
  std::enable_if_t<IsChar<CTe>>
  Write(const std::string &input) {
    Write<std::basic_string<CTe>>(input);
  }

  void To(const char *path) {
    std::ofstream file(path, std::ios::binary);

    if (!file) {
      throw std::runtime_error("err");
    }
    file.write(reinterpret_cast<const char *>(buffer_.data()), buffer_.size());
    file.close();
  }
  void To(const std::string &path) {
    To(path.c_str());
  }

private:
  std::uint64_t             pos_ = 0;
  std::vector<std::uint8_t> buffer_;

}; // Stream

template <typename Te, typename... As>
struct Stream::HasReaderImplSe<Te, std::void_t<decltype(std::declval<Reader<Te>>().Read(std::declval<As>()...))>, As...> {
  static constexpr bool Value = true;
};

template <typename Te, typename... As>
struct Stream::HasWriterImplSe<Te, std::void_t<decltype(std::declval<Writer<Te>>().Write(std::declval<As>()...))>, As...> {
  static constexpr bool Value = true;
};

template <typename VTe>
struct Stream::IsVectorSe<std::vector<VTe>> {
  static constexpr bool Value = true;
};

template <typename VTe, std::uint64_t Se> 
struct Stream::IsArraySe<std::array<VTe, Se>> {
  static constexpr bool Value = true;
};

template <typename FTe, typename STe>
struct Stream::IsPairSe<std::pair<FTe, STe>> {
  static constexpr bool Value = true;
};

template <typename... As> 
struct Stream::IsTupleSe<std::tuple<As...>> {
  static constexpr bool Value = true;
};

template <typename Te>
struct Stream::HasHashImplSe<Te, std::void_t<decltype(std::hash<Te>()(std::declval<Te>()))>> {
  static constexpr bool Value = true;
};

template <typename Ky, typename Ve>
struct Stream::IsMapSe<std::map<Ky, Ve>> {
  static constexpr bool Value = true;
};

template <typename Ky, typename Ve>
struct Stream::IsUnorderedMapSe<std::unordered_map<Ky, Ve>> {
  static constexpr bool Value = true;
};

template <typename Ve>
struct Stream::IsStringSe<std::basic_string<Ve>> {
  static constexpr bool Value = true;
};

template <>
struct Stream::IsCharSe<char> {
  static constexpr bool Value = true;
};

template <>
struct Stream::IsCharSe<char16_t> {
  static constexpr bool Value = true;
};

template <>
struct Stream::IsCharSe<char32_t> {
  static constexpr bool Value = true;
};

}; // PSD
