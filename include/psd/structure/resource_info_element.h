
#pragma once

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>

#include <memory>

namespace PSD {

class ResourceElementID {
public:
  enum Tp {
    ResolutionInfo,
    LayerState,
  };
};

class ResourceElementHeader {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  using SignatureTp = std::array<std::uint8_t, 4>;

  explicit ResourceElementHeader() = default;

  std::uint16_t id;
  std::uint64_t content_length;

}; // ResourceElementHeader

class ResourceElementHeader::LengthCalculator {
public:

  LengthCalculator(const ResourceElementHeader &) {}

  constexpr std::uint64_t Calculate() const {
    return 12;
  }

}; // ResourceElementHeader::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(ResourceElementHeader);

class ResourceElementHeader::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  ResourceElementHeader Read() {
    ResourceElementHeader output;

    ReadSignature();
    output.id = stream_.Read<std::uint16_t>();
    ReadName();
    output.content_length = stream_.Read<std::uint32_t>();

    return output;
  }

private:
  Stream &stream_;

  void ReadSignature() {
    if (stream_.Read<SignatureTp>() != SignatureTp { 0x38, 0x42, 0x49, 0x4D }) {
      throw std::runtime_error("InvalidSignature");
    }
  }
  void ReadName() {
    std::uint8_t length = stream_.Read<std::uint8_t>();
    stream_.AdjustPos(length 
      ? length + 0 
      : length + 1);
  }

}; // ResourceElementHeader::Reader 

PSD_REGISTER_READER(ResourceElementHeader);

class ResourceElementHeader::Writer {
public:
  Writer(Stream &stream, const ResourceElementHeader &input) 
    : stream_(stream)
    , input_(input) {}

  void Write() {
    stream_.Write(SignatureTp { 0x38, 0x42, 0x49, 0x4D });
    stream_.Write(input_.id);
    stream_.Write<std::uint16_t>(0);
    stream_.Write<std::uint32_t>(input_.content_length);
  }

private:
  Stream &stream_;
  const ResourceElementHeader &input_;

}; // ResourceElementHeader::Writer 

PSD_REGISTER_WRITER(ResourceElementHeader);

class ResourceElement {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  using Ptr = std::shared_ptr<ResourceElement>;
  using Tp = std::shared_ptr<ResourceElement>;

  virtual ~ResourceElement() = default;

  virtual std::uint16_t GetID()            const = 0;
  virtual std::uint64_t GetContentLength() const = 0;

  virtual bool Compare(const Ptr &other) const = 0;
  virtual Ptr  Clone() const = 0;

private:
  virtual void ReadContent  (Stream &stream, const ResourceElementHeader &header) = 0;
  virtual void WriteContent (Stream &stream) const = 0;
  virtual void WriteHeader  (Stream &stream) const = 0;

}; // ResourceElement 

class ResourceElement::LengthCalculator {
public:
  LengthCalculator(const ResourceElement::Tp &input) : input_(input) {}

  std::uint64_t Calculate() const {

    std::uint64_t actual_length = input_->GetContentLength();
    for (; actual_length % 2; 
           actual_length++);

    return PSD::LengthCalculator(ResourceElementHeader()).Calculate() + actual_length;
  }

private:
  ResourceElement::Tp input_;

}; // ResourceElement::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_TYPE(ResourceElement::Tp, ResourceElement);

class ResourceElement::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  ResourceElement::Ptr Read(const ResourceElementHeader &header, ResourceElement::Ptr ptr) {
    ptr->ReadContent(stream_, header); 

    if (header.content_length % 2) {
      stream_.IncPos();
    }
    return ptr;
  }
  
private:
  Stream &stream_;

}; // ResourceElement::Reader

PSD_REGISTER_READER_FOR_TYPE(ResourceElement::Tp, ResourceElement);

class ResourceElement::Writer {
public:
  Writer(Stream &stream, ResourceElement::Tp input) 
    : stream_(stream)
    , input_(input) {}

  void Write() {
    input_->WriteHeader  (stream_);
    input_->WriteContent (stream_);

    if (input_->GetContentLength() % 2) {
      stream_.Write<std::uint8_t>(0);
    }
  }

private:
  Stream             &stream_;
  ResourceElement::Tp input_;

}; // ResourceElement::Writer 

PSD_REGISTER_WRITER_FOR_TYPE(ResourceElement::Tp, ResourceElement);

#ifdef PSD_DEBUG

class DefaultResourceElement : public ResourceElement {
public:
  explicit DefaultResourceElement() = default;

  bool operator==(const DefaultResourceElement &other) const {
    if (id_ != other.id_) {
      return false;
    }
    return content_ == other.content_;
  }
  bool operator!=(const DefaultResourceElement &other) const {
    return !operator==(other);
  }

  std::uint16_t GetID() const override {
    return id_;
  }
  std::uint64_t GetContentLength() const override {
    return content_.size();
  }

  bool Compare(const Ptr &other) const override {
    return *std::static_pointer_cast<DefaultResourceElement>(other) == *this;
  }
  Ptr Clone() const override {
    return std::make_shared<DefaultResourceElement>(*this);
  }

private:

  void ReadContent(Stream &stream, const ResourceElementHeader &header) override {
    id_      = header.id;
    content_ = stream.Read<std::uint8_t>(header.content_length);
  }
  void WriteHeader(Stream &stream) const override {
    ResourceElementHeader header;
    header.id             = GetID();
    header.content_length = GetContentLength(); 
    stream.Write(header);
  }
  void WriteContent(Stream &stream) const override {
    stream.Write(content_);
  }

private:

  std::uint16_t             id_;
  std::vector<std::uint8_t> content_;
}; 

#endif
  
}; // PSD
