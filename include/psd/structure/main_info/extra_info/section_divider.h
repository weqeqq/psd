
#pragma once

#include <psd/structure/main_info/extra_info_element.h>
#include <psd/core/type/blending.h>

namespace PSD {

class DividerType {
public:

  class Reader;
  class Writer;

  enum Tp : std::uint32_t {
    Layer        = 0,
    OpenFolder   = 1,
    ClosedFolder = 2,
    Hidden       = 3,
    
  }; 
  using UnderlyingTp = std::underlying_type<Tp>::type;

  template <Tp Value>
  static constexpr UnderlyingTp UnderlyingValue = static_cast<UnderlyingTp>(Value);
};
class DividerTypeError : public Error {
public:
  class UndefinedValue;

protected:
  explicit DividerTypeError(const std::string &msg) : Error(msg) {}
}; 
class DividerTypeError::UndefinedValue : public DividerTypeError {
public:
  explicit UndefinedValue(DividerType::UnderlyingTp input) : DividerTypeError(
    "Undefined DividerType Value: " + std::to_string(input) + "\n"
    "Expected one of:    \n" 
    " - Layer        (0) \n" 
    " - OpenFolder   (1) \n"
    " - ClosedFolder (2) \n"
    " - Hidden       (3)"
  ) {}
};
class DividerType::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  DividerType::Tp Read() {
    auto value = stream_.Read<UnderlyingTp>();
    switch (value) {
      case UnderlyingValue<Layer>        : return Layer;
      case UnderlyingValue<OpenFolder>   : return OpenFolder;
      case UnderlyingValue<ClosedFolder> : return ClosedFolder;
      case UnderlyingValue<Hidden>       : return Hidden;
      default: {
        throw DividerTypeError::UndefinedValue(value);
      }
    }
  }
private:
  Stream &stream_;
};
PSD_REGISTER_READER_FOR_TYPE(DividerType::Tp, DividerType);
class DividerType::Writer {
public:

  explicit Writer(Stream &stream, DividerType::Tp input) : stream_(stream), input_(input) {} 

  void Write() {
    if (input_ != DividerType::Layer        && 
        input_ != DividerType::OpenFolder   && 
        input_ != DividerType::ClosedFolder &&
        input_ != DividerType::Hidden) {
      throw DividerTypeError::UndefinedValue(input_);
    }
    stream_.Write(static_cast<UnderlyingTp>(input_));
  }
private:
  Stream &stream_;
  DividerType::Tp input_;
}; 
PSD_REGISTER_WRITER_FOR_TYPE(DividerType::Tp, DividerType);

class DividerSubType {
public:

  class Reader;
  class Writer;

  enum Tp : std::uint32_t {
    Normal     = 0,
    SceneGroup = 1,
    NotPresent = 2,

  }; 
  using UnderlyingTp = std::underlying_type<Tp>::type;

  template <Tp Value>
  static constexpr UnderlyingTp UnderlyingValue = static_cast<UnderlyingTp>(Value);
}; 
class DividerSubTypeError : public Error {
public:
  class UndefinedValue;

protected:
  explicit DividerSubTypeError(const std::string &msg) : Error(msg) {}
};
class DividerSubTypeError::UndefinedValue : DividerSubTypeError { 
public:

  explicit UndefinedValue(DividerSubType::UnderlyingTp input) : DividerSubTypeError(
    "Undefined DividerSubType Value: " + std::to_string(input) + "\n"
    "Expected one of:     \n" 
    " - Normal        (0) \n" 
    " - SceneGroup    (1)"
  ) {}
};
class DividerSubType::Reader {
public:
  explicit Reader(Stream &stream) : stream_(stream) {}

  DividerSubType::Tp Read() {
    auto value = stream_.Read<UnderlyingTp>();
    switch (value) {
      case UnderlyingValue<Normal>     : return Normal;
      case UnderlyingValue<SceneGroup> : return SceneGroup;
      default: {
        throw DividerSubTypeError::UndefinedValue(value);
      }
    }
  }
private:
  Stream &stream_;
}; 
PSD_REGISTER_READER_FOR_TYPE(DividerSubType::Tp, DividerSubType);
class DividerSubType::Writer {
public:
  explicit Writer(Stream &stream, DividerSubType::Tp input) : stream_(stream), input_(input) {}

  void Write() {
    if (input_ != DividerSubType::Normal && 
        input_ != DividerSubType::SceneGroup) {
      throw DividerSubTypeError::UndefinedValue(input_);
    }
    stream_.Write(static_cast<UnderlyingTp>(input_));
  }
private:
  Stream &stream_;
  DividerSubType::Tp input_;
}; 
PSD_REGISTER_WRITER_FOR_TYPE(DividerSubType::Tp, DividerSubType);

class SectionDividerError : public Error {
public:
  class InvalidBlendingSignature;

protected:
  explicit SectionDividerError(const std::string &msg) : Error(msg) {}
}; 
class SectionDividerError::InvalidBlendingSignature : public SectionDividerError {
public:
  explicit InvalidBlendingSignature(Blending::SignatureTp signature) : SectionDividerError(CreateMsg(signature)) {}
private:
  std::string CreateMsg(Blending::SignatureTp signature) {
    std::stringstream sstream;
    sstream << "[ "; 
    sstream << std::hex 
            << std::setw(2) 
            << std::setfill('0') 
            << std::uppercase;
    sstream << unsigned(signature[0]) << ", "
            << unsigned(signature[1]) << ", "
            << unsigned(signature[2]) << ", "
            << unsigned(signature[3]) << " ]" << std::endl;
    return "Invalid blending signature\n" 
           "Readed   : " + sstream.str() +
           "Expected : [ 38, 42, 49, 4D ]";
  }
}; 
class SectionDivider : public ExtraInfoElement {
public:

  static constexpr std::uint32_t ID = 0x6C736374;

  explicit SectionDivider() = default;
  explicit SectionDivider(DividerType::Tp type) : type(type) {}

  bool operator==(const SectionDivider &other) const {
    return type == other.type;
  }
  bool operator!=(const SectionDivider &other) const {
    return !operator==(other);
  }

  std::uint32_t GetID() const override { 
    return ID; 
  }
  std::uint64_t GetContentLength() const override { 
    if (type == DividerType::Layer || type == DividerType::Hidden) {
      return sizeof type;
    } 
    std::uint64_t output = sizeof(type) + sizeof(Blending::SignatureTp) + sizeof(blending);
    if (subtype != DividerSubType::NotPresent) {
      return output + sizeof subtype;
    }
    return output;
  }

  void ReadContent(Stream &stream, const Header &header) override {
    type = stream.Read<decltype(type)>();
    if (header.content_length < 12) {
      return;
    }
    auto signature = stream.Read<Blending::SignatureTp>();
    if (signature != Blending::CorrectSignature) {
      throw SectionDividerError::InvalidBlendingSignature(signature);
    }
    blending = stream.Read<decltype(blending)>();
    if (header.content_length < 16) {
      return;
    }
    subtype = stream.Read<decltype(subtype)>();
  }
  void WriteContent(Stream &stream) const override {
    stream.Write(type);
    if (type == DividerType::Layer || 
        type == DividerType::Hidden) {
      return;
    }
    stream.Write(Blending::CorrectSignature);
    stream.Write(blending);
    if (subtype == DividerSubType::NotPresent) {
      return;
    }
    stream.Write(subtype);
  }

  PSD_EXTRA_INFO_ELEMENT_IMPLEMENT_CLONE   (SectionDivider);
  PSD_EXTRA_INFO_ELEMENT_IMPLEMENT_COMPARE (SectionDivider);

  DividerType   ::Tp type     = DividerType   ::Layer;
  Blending      ::Tp blending = Blending      ::PassThrough;
  DividerSubType::Tp subtype  = DividerSubType::NotPresent;
};
};

