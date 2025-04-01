
#pragma once

#include <memory>
#include <type_traits>
#include <cstdint>

namespace PSD {

class Element {
public:

  using Pointer      = std::shared_ptr<Element>;
  using ConstPointer = std::shared_ptr<Element const>;

  virtual ~Element() = default;

  template <typename ElementT, typename... ArgumentsT>
  static std::shared_ptr<Element> Create(ArgumentsT&&... arguments) {

    static_assert(std::is_base_of_v<Element, ElementT>);

    return Pointer(new ElementT(
      std::forward<ArgumentsT>(arguments)...
    ));
  }

  virtual bool IsGroup() const = 0;
  virtual bool IsLayer() const = 0;

  virtual Pointer Clone() const = 0;
  virtual bool    Compare(ConstPointer) const = 0;

  virtual std::uint64_t GetTop()    const = 0; 
  virtual std::uint64_t GetLeft()   const = 0;
  virtual std::uint64_t GetBottom() const = 0;
  virtual std::uint64_t GetRight()  const = 0;

protected:

  template <typename ElementT>
  static Pointer Clone(const ElementT *element) {
    return Create<ElementT>(*element);
  }
  template <typename ElementT>
  static bool Compare(const ElementT *element, ConstPointer other) {
    return *element == 
           *std::static_pointer_cast<const ElementT>(other);
  }
};
};
