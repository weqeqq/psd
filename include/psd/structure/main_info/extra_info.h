#pragma once 

#include <psd/structure/main_info/extra_info_element.h>
#include <psd/structure/main_info/extra_info/unicode_name.h>
#include <psd/structure/main_info/extra_info/section_divider.h>

#include <iomanip>

namespace PSD {

class ExtraInfo {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  template <typename ExtraInfoT>
  class IteratorBaseClass;

  using Iterator      = IteratorBaseClass<ExtraInfo>;
  using ConstIterator = IteratorBaseClass<const ExtraInfo>;
  
  explicit ExtraInfo() = default;

  #ifdef PSD_DEBUG
  ExtraInfo(const ExtraInfo &other)
    : insertion_order_(other.insertion_order_)
    , data_(other.CopyData()) {}
  #else
  ExtraInfo(const ExtraInfo &other)
    : data_(other.CopyData()) {}
  #endif

  ExtraInfo &operator=(const ExtraInfo &other) {
    if (this != &other) {
      #ifdef PSD_DEBUG
      insertion_order_ = other.insertion_order_;
      #endif
      data_ = other.CopyData();
    }
    return *this;
  }
  bool operator==(const ExtraInfo &other) const {
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
  bool operator!=(const ExtraInfo &other) const {
    return !(*this == other);
}

  ExtraInfoElement::Tp Get(std::uint32_t id) {
    return data_.at(id);
  }
  const ExtraInfoElement::Tp Get(std::uint32_t id) const {
    return data_.at(id);
  }

  template <typename ExtraInfoElementT>
  ExtraInfoElementT &Get() {
    return *std::static_pointer_cast<ExtraInfoElementT>(Get(ExtraInfoElementT::ID));
  }
  template <typename ExtraInfoElementT>
  const ExtraInfoElementT &Get() const {
    return *std::static_pointer_cast<ExtraInfoElementT>(Get(ExtraInfoElementT::ID));
  }

  template <typename ElementT>
  void Set(ElementT element) {
    if constexpr (std::is_same_v<ElementT, ExtraInfoElement::Tp>) {
      #ifdef PSD_DEBUG 
      insertion_order_.push_back(element->GetID());
      #endif 
      data_[element->GetID()] = element;
    } else {
      Set(std::static_pointer_cast<ExtraInfoElement>(
        std::make_shared<ElementT>(std::move(element))
      ));
    }
  }
  template <typename ElementT, typename... ArgumentsT>
  void Emplace(ArgumentsT&&... arguments) {
    Set(ElementT(std::forward<ArgumentsT>(arguments)...));
  }

  void Clear() {
    #ifdef PSD_DEBUG
    insertion_order_.clear();
    #endif
    data_.clear();
  }

  friend Iterator begin(ExtraInfo &input);
  friend ConstIterator begin(const ExtraInfo &input);
  friend Iterator end(ExtraInfo &input);
  friend ConstIterator end(const ExtraInfo &input);

private:
  #ifdef PSD_DEBUG
  std::vector<std::uint32_t> insertion_order_;
  #endif
  std::unordered_map<std::uint32_t, ExtraInfoElement::Tp> data_;

  std::unordered_map<std::uint32_t, ExtraInfoElement::Tp> CopyData() const {
    std::unordered_map<std::uint32_t, ExtraInfoElement::Tp> output = data_; 
    
    for (auto &[id, ptr] : output) {
      ptr = ptr->Clone();
    }
    return output;
  }

}; // ExtraInfo

template <typename ExtraInfoT> 
class ExtraInfo::IteratorBaseClass {
public:

static constexpr bool IsConst = std::is_const_v<ExtraInfoT>;

  #ifdef PSD_DEBUG
  using InsertionOrderIteratorTp = std::conditional_t<IsConst,
    std::vector<std::uint32_t>::const_iterator,
    std::vector<std::uint32_t>::iterator>;
  #endif
  using DataIteratorTp = std::conditional_t<IsConst,
    std::unordered_map<std::uint32_t, ExtraInfoElement::Tp>::const_iterator,
    std::unordered_map<std::uint32_t, ExtraInfoElement::Tp>::iterator>;

  #ifdef PSD_DEBUG 
  IteratorBaseClass(
    ExtraInfoT &extra_info,
    InsertionOrderIteratorTp insertion_order_iterator
  ) : extra_info_(extra_info)
    , insertion_order_iterator_(insertion_order_iterator) {}
  #else
  IteratorBaseClass(DataIteratorTp data_iterator) : data_iterator_(data_iterator) {}
  #endif

  auto operator*() const {
    #ifdef PSD_DEBUG
    return extra_info_.Get(*insertion_order_iterator_);
    #else
    return *data_iterator_;
    #endif
  }
  IteratorBaseClass<ExtraInfoT> &operator++() { 
    #ifdef PSD_DEBUG
    insertion_order_iterator_++;
    #else
    data_iterator_++;
    #endif
    return *this; 
  }
  IteratorBaseClass<ExtraInfoT> operator++(int) {
    auto tmp = *this;
    operator++();

    return tmp;
  }
  bool operator==(const IteratorBaseClass<ExtraInfoT> &other) const {
    #ifdef PSD_DEBUG
    return insertion_order_iterator_ == other.insertion_order_iterator_;
    #else
    return data_iterator_ == other.data_iterator_;
    #endif
  }
  bool operator!=(const IteratorBaseClass<ExtraInfoT> &other) const {
    return !operator==(other);
  }

private:
  #ifdef PSD_DEBUG
  ExtraInfoT &extra_info_;
  InsertionOrderIteratorTp insertion_order_iterator_;
  #else
  DataIteratorTp data_iterator_;
  #endif

}; // ExtraInfo::IteratorBaseClass

ExtraInfo::Iterator begin(ExtraInfo &input) {
  #ifdef PSD_DEBUG
  return ExtraInfo::Iterator(input, input.insertion_order_.begin());
  #else
  return ExtraInfo::Iterator(input.data_.begin());
  #endif
}
ExtraInfo::ConstIterator begin(const ExtraInfo &input) {
  #ifdef PSD_DEBUG
  return ExtraInfo::ConstIterator(input, input.insertion_order_.begin());
  #else
  return ExtraInfo::ConstIterator(input.data_.begin());
  #endif
}
ExtraInfo::Iterator end(ExtraInfo &input) {
  #ifdef PSD_DEBUG
  return ExtraInfo::Iterator(input, input.insertion_order_.end());
  #else
  return ExtraInfo::Iterator(input.data_.end());
  #endif
}
ExtraInfo::ConstIterator end(const ExtraInfo &input) {
  #ifdef PSD_DEBUG
  return ExtraInfo::ConstIterator(input, input.insertion_order_.end());
  #else
  return ExtraInfo::ConstIterator(input.data_.end());
  #endif
}

class ExtraInfo::LengthCalculator {
public:

  explicit LengthCalculator(const ExtraInfo &input) : input_(input) {}

  std::uint64_t Calculate() const {
    std::uint64_t output = 0;
    
    for (const auto &element : input_) {
      output += PSD::LengthCalculator(element).Calculate();
    }
    return output;
  }

private:
  const ExtraInfo &input_;

}; // ExtraInfo::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(ExtraInfo);

class ExtraInfo::Reader {
public:

  explicit Reader(Stream &stream) : stream_(stream) {}

  ExtraInfo Read(std::uint64_t length) {
    ExtraInfo output;
    std::uint64_t stop = length + stream_.GetPos();

    while (stream_.GetPos() < stop) {
      ExtraInfoElement::Header header; 

      if ((stop - stream_.GetPos()) < PSD::LengthCalculator(header).Calculate()) {
        stream_.AdjustPos(stop - stream_.GetPos());
        break;
      }
      header = stream_.Read<ExtraInfoElement::Header>();

      switch (header.id) {
        case UnicodeName::ID: {
          output.Set(stream_.Read<ExtraInfoElement::Tp>(CrTuple(
            header,
            std::make_shared<UnicodeName>()
          )));
          break;
        }
        case SectionDivider::ID: {
          output.Set(stream_.Read<ExtraInfoElement::Tp>(CrTuple(
            header,
            std::make_shared<SectionDivider>()
          )));
          break;
        }
        #ifdef PSD_DEBUG
        default: {
          output.Set(stream_.Read<ExtraInfoElement::Tp>(CrTuple(
            header,
            std::make_shared<DefaultExtraInfoElement>()
          )));
          break;
        }
        #endif
      }
    }

    return output;
  }

private:
  Stream &stream_;

}; // ExtraInfo::Reader

PSD_REGISTER_READER(ExtraInfo);

class ExtraInfo::Writer {
public:

  explicit Writer(Stream &stream, const ExtraInfo &input) : stream_(stream), input_(input) {}

  void Write() {
    for (const auto &ptr : input_) {
      stream_.Write(ptr);
    }
  }

private:
  Stream &stream_;
  const ExtraInfo &input_;

}; // ExtraInfo::Writer

PSD_REGISTER_WRITER(ExtraInfo);

}; // PSD

