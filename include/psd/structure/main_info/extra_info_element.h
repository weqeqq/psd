
#define PSD_DEBUG

#pragma once

#include <psd/core/stream.h>
#include <psd/core/type/error.h>
#include <psd/core/length_calculator.h>

#include <iomanip>
#include <memory>

namespace PSD {

class ExtraInfoElementIOError : public Error {
public:
  struct InvalidSignature;
  struct UndefinedID;

protected:
  explicit ExtraInfoElementIOError(const std::string &msg) : Error(msg) {}

}; // ExtraInfoElementIOError

class ExtraInfoElementIOError::InvalidSignature : public ExtraInfoElementIOError {
public:
  explicit InvalidSignature() : ExtraInfoElementIOError("Invalid signature") {}

}; // ExtraInfoElementIOError::InvalidSignature

class ExtraInfoElementIOError::UndefinedID : public ExtraInfoElementIOError {
public:
  explicit UndefinedID() : ExtraInfoElementIOError("Undefined ID") {}

}; // ExtraInfoElementIOError::UndefinedID

class ExtraInfoElement {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  class Header;

  virtual ~ExtraInfoElement() = default;

  using Tp = std::shared_ptr<ExtraInfoElement>;

  virtual std::uint32_t GetID()            const = 0;
  virtual std::uint64_t GetContentLength() const = 0;

  virtual bool Compare(const Tp &other) const = 0;
  virtual Tp   Clone() const = 0;

private:
  virtual void ReadContent  (Stream &stream, const Header &header) = 0;
  virtual void WriteContent (Stream &stream) const = 0;

}; // ExtraInfoElement

class ExtraInfoElement::Header {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  using SignatureTp = std::array<std::uint8_t, 4>;

  explicit Header() = default;

  explicit Header(std::uint32_t id, std::uint32_t content_length) 
    : id(id)
    , content_length(content_length) {}

  std::uint32_t id;
  std::uint32_t content_length;

}; // ExtraInfoElement::Header

class ExtraInfoElement::Header::LengthCalculator {
public:

  explicit LengthCalculator(const Header &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return 12;
  }

private:
  const Header &input_;

}; // ExtraInfoElement::Header::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(ExtraInfoElement::Header);

class ExtraInfoElement::Header::Reader {
public:

  explicit Reader(Stream &stream) : stream_(stream) {}

  Header Read() {
    Header output;

    ReadSignature();
    output.id             = stream_.Read<std::uint32_t>();
    output.content_length = stream_.Read<std::uint32_t>();

    return output;
  }

private:
  Stream &stream_;

  void ReadSignature() {
    auto signature = stream_.Read<SignatureTp>();

    if (signature != SignatureTp { 0x38, 0x42, 0x49, 0x4D } &&
        signature != SignatureTp { 0x38, 0x42, 0x36, 0x34 }) {
      throw ExtraInfoElementIOError::InvalidSignature();
    }
  }

}; // ExtraInfoElement::Header::Reader

PSD_REGISTER_READER(ExtraInfoElement::Header);

class ExtraInfoElement::Header::Writer {
public:

  explicit Writer(Stream &stream, const Header &input) : stream_(stream), input_(input) {}

  void Write() {
    WriteSignature();
    stream_.Write(input_.id);
    stream_.Write(input_.content_length);
  }

private:
  Stream &stream_;
  const Header &input_;

  void WriteSignature() {
    stream_.Write<SignatureTp>({ 0x38, 0x42, 0x49, 0x4D });
  }

}; // ExtraInfoElement::Header::Writer

PSD_REGISTER_WRITER(ExtraInfoElement::Header);

class ExtraInfoElement::LengthCalculator {
public:

  explicit LengthCalculator(ExtraInfoElement::Tp input) : input_(input) {}

  std::uint64_t Calculate() const {

    std::uint64_t actual_length = input_->GetContentLength();
    for (; actual_length % 2; 
           actual_length++);

    return PSD::LengthCalculator(Header(
      input_->GetID(),
      input_->GetContentLength()
    )).Calculate() + actual_length;
  }

private:
  ExtraInfoElement::Tp input_;

}; // ExtraInfoElement::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_TYPE(ExtraInfoElement::Tp, ExtraInfoElement);

class ExtraInfoElement::Reader {
public:

  explicit Reader(Stream &stream) : stream_(stream) {}

  ExtraInfoElement::Tp Read(const Header &header, ExtraInfoElement::Tp ptr) {
    ptr->ReadContent(stream_, header);

    if (header.content_length % 2) {
      stream_.IncPos();
    }
    return ptr;
  }

private:
  Stream &stream_;

}; // ExtraInfoElement::Reader

PSD_REGISTER_READER_FOR_TYPE(ExtraInfoElement::Tp, ExtraInfoElement);

class ExtraInfoElement::Writer {
public:

  explicit Writer(Stream &stream, ExtraInfoElement::Tp input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(Header(input_->GetID(), input_->GetContentLength()));
    input_->WriteContent(stream_);
    for (auto length = input_->GetContentLength();
              length % 2;
              length++) {
      stream_.Write<std::uint8_t>(0);
    }
  }

private:
  Stream &stream_;
  ExtraInfoElement::Tp input_;

}; // ExtraInfoElement::Writer 

PSD_REGISTER_WRITER_FOR_TYPE(ExtraInfoElement::Tp, ExtraInfoElement);

#ifdef PSD_DEBUG

class DefaultExtraInfoElement : public ExtraInfoElement {
public:

  explicit DefaultExtraInfoElement() = default;

  bool operator==(const DefaultExtraInfoElement &other) const {
    if (id_ == other.id_) {
      return content_ == other.content_;
    } else {
      return false;
    }
  }
  bool operator!=(const DefaultExtraInfoElement &other) const {
    return !operator==(other);
  }

  std::uint32_t GetID() const override {
    return id_;
  }
  std::uint64_t GetContentLength() const override {
    return content_.size();
  }
  void ReadContent(Stream &stream, const Header &header) override {
    id_      = header.id;
    content_ = stream.Read<std::uint8_t>(header.content_length);
  }
  void WriteContent(Stream &stream) const override {
    stream.Write(content_);
  }

  bool Compare(const ExtraInfoElement::Tp &other) const override {
    return *std::static_pointer_cast<DefaultExtraInfoElement>(other) == *this;
  }
  ExtraInfoElement::Tp Clone() const override {
    return std::make_shared<DefaultExtraInfoElement>(*this);
  }

private:
  std::uint32_t id_;
  std::vector<std::uint8_t> content_;

}; // DefaultExtraInfoElement

#endif

#define PSD_EXTRA_INFO_ELEMENT_IMPLEMENT_CLONE(ClassName)          \
  bool Compare(const ExtraInfoElement::Tp &other) const override { \
    return *std::static_pointer_cast<ClassName>(other) == *this;   \
  }
#define PSD_EXTRA_INFO_ELEMENT_IMPLEMENT_COMPARE(ClassName) \
  ExtraInfoElement::Tp Clone() const override {             \
    return std::make_shared<ClassName>(*this);              \
  }

}; // PSD
