
#pragma once

#define PSD_DEBUG

#include <psd/structure/resource_info_element.h>

#include <iomanip>
#include <memory>

namespace PSD {

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
