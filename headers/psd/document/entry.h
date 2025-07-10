
#pragma once

#include <psd/llapi/structure/info/layer_info/layer_data.h>
#include <memory>
namespace PSD {
//

using Coordinates = llapi::Coordinates;

class Entry {
public:
  virtual ~Entry() = default;

  virtual bool IsGroup() const = 0;
  virtual bool IsLayer() const = 0;

  virtual bool Compare(
    const std::shared_ptr<Entry> &other
  ) const = 0;
  virtual std::shared_ptr<Entry>
  Clone() const = 0;

  virtual unsigned Top()    const = 0;
  virtual unsigned Left()   const = 0;
  virtual unsigned Bottom() const = 0;
  virtual unsigned Right()  const = 0;
}; // class Entry
template <typename T>
class EntryFor : public Entry {
public:
  virtual bool Compare(
    const std::shared_ptr<Entry> &other
  ) const override final {
    return std::static_pointer_cast<T>(other)->operator==(Self());
  }
  virtual std::shared_ptr<Entry>
  Clone() const override final {
    return std::static_pointer_cast<Entry>(std::make_shared<T>(Self()));
  }
private:
  const T &Self() const {
    return *reinterpret_cast<const T *>(this);
  }
}; // class EntryFor
}; // namespace PSD::llapi
