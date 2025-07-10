
#pragma once

#include <algorithm>
#include <memory>
#include <psd/document/entry.h>

namespace PSD {
//
class Group : public EntryFor<Group> {
  auto CloneData() const {
    std::vector<std::shared_ptr<Entry>> output = data_;
    for (auto &entry : output) {
      entry = entry->Clone();
    }
    return output;
  }
public:
  Group() = default;
  Group(std::string name) : name_(name) {}

  Group(const Group &other)
    : name_(other.name_)
    , data_(other.CloneData()) {}

  Group &operator=(const Group &other) {
    if (this != &other) {
      name_ = other.name_;
      data_ = other.CloneData();
    }
    return *this;
  }

  bool operator==(const Group &other) const {
    auto compare = [](const auto &left, const auto &right) {
      return left->Compare(right);
    };
    return std::equal(
      data_.begin(),
      data_.end(),
      other.data_.begin(),
      compare
    );
  }
  bool operator!=(const Group &other) const {
    return !operator==(other);
  }

  bool Empty() const { return data_.empty(); }
  auto begin()       { return data_.begin(); }
  auto begin() const { return data_.begin(); }
  auto end()         { return data_.end();   }
  auto end() const   { return data_.end();   }

  bool IsLayer() const override final { return false; }
  bool IsGroup() const override final { return true; }

  unsigned Top() const override final {
    if (Empty()) return 0;
    return (*std::min_element(begin(), end(), [](const auto &entry, const auto &smallest) {
      return entry->Top() < smallest->Top();
    }))->Top();
  }
  unsigned Left() const override final {
    if (Empty()) return 0;
    return (*std::min_element(begin(), end(), [](const auto &entry, const auto &smallest) {
      return entry->Left() < smallest->Left();
    }))->Left();
  }
  unsigned Bottom() const override final {
    if (Empty()) return 0;
    return (*std::max_element(begin(), end(), [](const auto &largest, const auto &entry) {
      return largest->Bottom() < entry->Bottom();
    }))->Bottom();
  }
  unsigned Right() const override final {
    if (Empty()) return 0;
    return (*std::max_element(begin(), end(), [](const auto &largest, const auto &entry) {
      return largest->Right() < entry->Right();
    }))->Right();
  }
  unsigned Length() const {
    return data_.size();
  }
  unsigned Count() const {
    return Length();
  }
  unsigned LayerCount() const {
    return std::count_if(begin(), end(), [](const auto &entry) {
      return entry->IsLayer();
    });
  }
  unsigned GroupCount() const {
    return std::count_if(begin(), end(), [](const auto &entry) {
      return entry->IsGroup();
    });
  }
  template <typename T>
  void Push(T &&entry) {
    data_.push_back(std::static_pointer_cast<Entry>(
      std::make_shared<std::decay_t<T>>(std::forward<T>(entry))
    ));
  }
  std::shared_ptr<Entry> operator[](unsigned index) {
    return data_[index];
  }
  std::shared_ptr<const Entry> operator[](unsigned index) const {
    return data_[index];
  }
  void SetName(std::string name) {
    name_ = std::move(name);
  }
  std::string &Name() {
    return name_;
  }
  const std::string &Name() const {
    return name_;
  }
private:
  std::string name_;
  std::vector<std::shared_ptr<Entry>> data_;
}; // class Group
inline Group &GroupCast(std::shared_ptr<Entry> input) {
  return *std::static_pointer_cast<Group>(input);
}
inline const Group &GroupCast(std::shared_ptr<const Entry> input) {
  return *std::static_pointer_cast<const Group>(input);
}
}; // namespace PSD
