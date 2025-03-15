
#pragma once

#define PSD_DEBUG

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>

#include <iomanip>
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
    return PSD::LengthCalculator(ResourceElementHeader()).Calculate() + input_->GetContentLength();
  }

private:
  ResourceElement::Tp input_;

}; // ResourceElement::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_TYPE(ResourceElement::Tp, ResourceElement);

class ResourceElement::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  ResourceElement::Ptr Read(const ResourceElementHeader &header, ResourceElement::Ptr ptr) {
    ptr->ReadContent(stream_, header); return ptr;
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
    if (header.content_length % 2) {
      stream.IncPos();
    }
  }
  void WriteHeader(Stream &stream) const override {
    ResourceElementHeader header;
    header.id             = GetID();
    header.content_length = GetContentLength(); 
    stream.Write(header);
  }
  void WriteContent(Stream &stream) const override {
    stream.Write(content_);
    if (GetContentLength() % 2) {
      stream.Write<std::uint8_t>(0);
    }
  }

private:

  std::uint16_t             id_;
  std::vector<std::uint8_t> content_;
}; 

#endif

class ResourceInfo {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  template <typename ResourceInfoT>
  class IteratorBaseClass;

  using Iterator      = IteratorBaseClass<ResourceInfo>;
  using ConstIterator = IteratorBaseClass<const ResourceInfo>;

  friend Iterator begin(ResourceInfo &input);
  friend ConstIterator begin(const ResourceInfo &input);

  friend Iterator end(ResourceInfo &input);
  friend ConstIterator end(const ResourceInfo &input);

  explicit ResourceInfo() = default;

  #ifdef PSD_DEBUG
  ResourceInfo(const ResourceInfo &other) 
    : insertion_order_(other.insertion_order_)
    , data_(other.CopyData()) {}
  #else
  ResourceInfo(const ResourceInfo &other) 
    : data_(other.CopyData()) {}
  #endif

  ResourceInfo &operator=(const ResourceInfo &other) {
    if (this != &other) {
      #if defined(PSD_DEBUG)
      insertion_order_ = other.insertion_order_;
      #endif
      data_ = other.CopyData();
    }
    return *this;
  }

  bool operator==(const ResourceInfo &other) const {
    auto compare = [](const auto &left, const auto &right) {
      return left.second->Compare(right.second);
    };
    return std::equal(
      this->data_.begin(),
      this->data_.end(),
      other.data_.begin(),
      compare
    );
  }
  bool operator!=(const ResourceInfo &other) const {
    return !operator==(other);
  }

  ResourceElement::Ptr Get(ResourceElementID::Tp id) {
    return data_.at(id);
  }
  const ResourceElement::Ptr Get(ResourceElementID::Tp id) const {
    return data_.at(id);
  }

  template <typename ResourceElementTe>
  ResourceElementTe &Get() {
    return std::static_pointer_cast<ResourceElementTe>(Get(ResourceElementTe::ID));
  }
  template <typename ResourceElementTe>
  const ResourceElementTe &Get() const {
    return std::static_pointer_cast<ResourceElementTe>(Get(ResourceElementTe::ID));
  }

  void Set(ResourceElement::Ptr ptr) {
    #ifdef PSD_DEBUG
    insertion_order_.push_back(
      static_cast<ResourceElementID::Tp>(ptr->GetID())
    );
    #endif
    data_[static_cast<ResourceElementID::Tp>(ptr->GetID())] = ptr;
  }
  /*template <typename ResourceElementTe>*/
  /*void Set(ResourceElementTe element) {*/
  /*  data_[ResourceElementTe::ID] = std::make_shared<ResourceElementTe>(std::move(element));*/
  /*} */

private:
  #ifdef PSD_DEBUG
  std::vector<ResourceElementID::Tp> insertion_order_;
  #endif
  std::unordered_map<ResourceElementID::Tp, ResourceElement::Ptr> data_;

  std::unordered_map<ResourceElementID::Tp, ResourceElement::Ptr> CopyData() const {
    std::unordered_map<ResourceElementID::Tp, ResourceElement::Ptr> output = data_;

    for (auto &[id, ptr] : output) {
      ptr = ptr->Clone();
    }
    return output;
  }

  std::uint64_t CalculateContentLength() const {
    std::uint64_t output = 0;

    for (const auto &[element_id, element_ptr] : data_) {
      output += PSD::LengthCalculator(element_ptr).Calculate();
    }
    return output;
  }

}; // ResourceInfo 

template <typename ResourceInfoT>
class ResourceInfo::IteratorBaseClass {
public:

  static constexpr bool IsConst = std::is_const_v<ResourceInfoT>;

  #ifdef PSD_DEBUG
  using InsertionOrderIteratorTp = std::conditional_t<IsConst, 
    std::vector<ResourceElementID::Tp>::const_iterator,
    std::vector<ResourceElementID::Tp>::iterator>;
  #endif
  using DataIteratorTp = std::conditional_t<IsConst, 
    std::unordered_map<ResourceElementID::Tp, ResourceElement::Ptr>::const_iterator,
    std::unordered_map<ResourceElementID::Tp, ResourceElement::Ptr>::iterator>;

  #ifdef PSD_DEBUG
  IteratorBaseClass(
    ResourceInfoT           &resource_info,
    InsertionOrderIteratorTp insertion_order_iterator
  ) : resource_info_            (resource_info)
    , insertion_order_iterator_ (insertion_order_iterator) {}
  #else
  IteratorBaseClass(DataIteratorTp data_iterator) : data_iterator_(data_iterator) {}
  #endif

  auto operator*() const {
    #ifdef PSD_DEBUG
    return resource_info_.data_.at(*insertion_order_iterator_);
    #else 
    return *data_iterator_;
    #endif
  }
  IteratorBaseClass<ResourceInfoT> &operator++() {
    #ifdef PSD_DEBUG
    insertion_order_iterator_++;
    #else
    data_iterator_++;
    #endif
    return *this;
  }
  IteratorBaseClass<ResourceInfoT> operator++(int) {
    auto tmp = *this;
    operator++();
    return tmp;
  }
  bool operator==(const IteratorBaseClass<ResourceInfoT> &other) const {
    #ifdef PSD_DEBUG
    return insertion_order_iterator_ == other.insertion_order_iterator_;
    #else 
    return data_iterator_ == other.data_iterator_;
    #endif
  }
  bool operator!=(const IteratorBaseClass<ResourceInfoT> &other) const {
    return !operator==(other);
  }

private:
  #ifdef PSD_DEBUG 
  ResourceInfoT           &resource_info_;
  InsertionOrderIteratorTp insertion_order_iterator_;
  #else
  DataIteratorTp data_iterator_;
  #endif

}; // ResourceInfo::IteratorBaseClass

ResourceInfo::Iterator begin(ResourceInfo &input) {
  #ifdef PSD_DEBUG
  return ResourceInfo::Iterator(input, input.insertion_order_.begin());
  #else
  return ResourceInfo::Iterator(input.data_.begin());
  #endif
}
ResourceInfo::ConstIterator begin(const ResourceInfo &input) {
  #ifdef PSD_DEBUG
  return ResourceInfo::ConstIterator(input, input.insertion_order_.begin());
  #else
  return ResourceInfo::ConstIterator(input.data_.begin());
  #endif
}
ResourceInfo::Iterator end(ResourceInfo &input) {
  #ifdef PSD_DEBUG
  return ResourceInfo::Iterator(input, input.insertion_order_.end());
  #else
  return ResourceInfo::Iterator(input.data_.end());
  #endif
}
ResourceInfo::ConstIterator end(const ResourceInfo &input) {
  #ifdef PSD_DEBUG
  return ResourceInfo::ConstIterator(input, input.insertion_order_.end());
  #else
  return ResourceInfo::ConstIterator(input.data_.end());
  #endif
}

class ResourceInfo::LengthCalculator {
public:
  LengthCalculator(const ResourceInfo &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return input_.CalculateContentLength() + sizeof(std::uint32_t);
  }

private:
  const ResourceInfo &input_;

}; // ResourceInfo::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(ResourceInfo);

class ResourceInfo::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  ResourceInfo Read() {
    ResourceInfo output;
    std::uint64_t stop = stream_.Read<std::uint32_t>() + stream_.GetPos();

    while (stream_.GetPos() < stop) {
      ResourceElementHeader header = stream_.Read<ResourceElementHeader>(); 
      switch (header.id) {
        #ifdef PSD_DEBUG
        default: {
          output.Set(stream_.Read<ResourceElement::Tp>(CrTuple(
            header,
            std::make_shared<DefaultResourceElement>()
          )));
        }
        #endif
      }
    }

    return output;
  }

private:
  Stream &stream_;

}; // ResourceInfo::Reader 

PSD_REGISTER_READER(ResourceInfo);

class ResourceInfo::Writer {
public:

  explicit Writer(Stream &stream, const ResourceInfo &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(std::uint32_t(
      input_.CalculateContentLength()
    ));
    for (const auto &ptr : input_) {
      stream_.Write(ptr);
    }
  }

private:
  Stream &stream_;
  const ResourceInfo &input_;

}; // ResourceInfo::Writer

PSD_REGISTER_WRITER(ResourceInfo);

}; // PSD
