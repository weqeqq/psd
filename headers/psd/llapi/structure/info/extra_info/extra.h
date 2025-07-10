
#pragma once

#include <iomanip>
#include <iostream>
#include <memory>
#include <psd/llapi/stream.h>

namespace PSD::llapi {
//
enum class ExtraID : U32 {
  SectionDivider = 0x6C736374,
}; // enum class ExtraID
template <>
struct FromStreamFn<ExtraID> {
  void operator()(Stream &stream, ExtraID &output) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<ExtraID> &>(output));
  }
}; // struct FromStreamFn<ExtraID>
template <>
struct ToStreamFn<ExtraID> {
  void operator()(Stream &stream, ExtraID input) {
    stream.Write(static_cast<std::underlying_type_t<ExtraID>>(input));
  }
}; // struct ToStreamFn<ExtraID>
template <ExtraID I>
struct UseExtraID {
  static constexpr ExtraID Value = I;
}; // struct UseExtraID
template <typename T>
struct ExtraFromID_; // struct ExtraFromID_

template <typename T>
static constexpr ExtraID ExtraFromID = ExtraFromID_<T>::Value;
class Extra {
  friend FromStreamFn<std::shared_ptr<Extra>>;
  friend ToStreamFn<std::shared_ptr<Extra>>;
public:
  virtual ~Extra() = default;
  virtual ExtraID ID() const = 0;
  virtual U32 Length() const = 0;
  virtual bool Compare(
    const std::shared_ptr<Extra> &other
  ) const = 0;
  virtual std::shared_ptr<Extra>
  Clone() const = 0;
protected:
  virtual U32 ContentLength() const = 0;
  virtual void ToStream(Stream &stream) const = 0;
}; // class Extra
template <>
struct FromStreamFn<std::shared_ptr<Extra>> {
  void operator()(Stream &stream, std::shared_ptr<Extra> &output, std::shared_ptr<Extra> extra) {
    if (extra->ContentLength() % 2) {
      stream++;
    }
    output = extra;
  }
}; // struct FromStreamFn<std::shared_ptr<Extra>>
template <>
struct ToStreamFn<std::shared_ptr<Extra>> {
  void operator()(Stream &stream, std::shared_ptr<Extra> input) {
    input->ToStream(stream);
  }
}; // struct ToStreamFn<std::shared_ptr<Extra>>
class ExtraHeader {
  struct FromStreamFn {
    void operator()(Stream &stream, ExtraHeader &output) {
      auto signature = stream.Read<U32>();
      if (signature != 0x3842494D &&
          signature != 0x38423634) throw Error("PSD::Error: ExtraHeaderSignatureError");
      stream.ReadTo(output.id);
      stream.ReadTo(output.content_length);
    }
  }; // struct FromStreamFn;
  struct ToStreamFn {
    void operator()(Stream &stream, const ExtraHeader &input) {
      stream.Write(U32(0x3842494D));
      stream.Write(input.id);
      stream.Write(input.content_length);
    }
  }; // struct ToStreamFn
  friend Stream;
  auto Comparable() const {
    return std::tie(id, content_length);
  }
public:
  ExtraHeader() = default;
  bool operator==(const ExtraHeader &other) const {
    return Comparable() == other.Comparable();
  }
  bool operator!=(const ExtraHeader &other) const {
    return !operator==(other);
  }
  ExtraID id; U32 content_length;
  constexpr U32 Length() const {
    return 12;
  }
}; // class ExtraHeader
namespace detail {
//
template <ExtraID I>
struct UseExtraID {
  static constexpr ExtraID Value = I;
}; // struct UseExtraID
template <typename T>
struct ExtraToID : UseExtraID<static_cast<ExtraID>(-1)> {}; // struct ExtraToID
}; // namespace detail
template <typename T>
static constexpr ExtraID ExtraToID = detail::ExtraToID<T>::Value;
template <typename T>
class ExtraFor : public Extra {
public:
  ExtraID ID() const override final {
    return ExtraToID<T>;
  }
  U32 Length() const override final {
    return ExtraHeader().Length() + Padding(ContentLength());
  }
  bool Compare(
    const std::shared_ptr<Extra> &other
  ) const override final {
    return std::static_pointer_cast<T>(other)->operator==(Self());
  }
  std::shared_ptr<Extra>
  Clone() const override final {
    return std::make_shared<T>(Self());
  }
protected:
  void ToStream(Stream &stream) const override final {
    stream.Write(ExtraHeader{ExtraToID<T>, ContentLength()});
    stream.Write(Self());
    if (ContentLength() % 2) {
      stream.Write(U8(0));
    }
  }
private:
  const T &Self() const {
    return *reinterpret_cast<const T *>(this);
  }
  static U32 Padding(U32 input) {
    return input % 2 ? input + 1 : input;
  }
}; // class ExtraFor
}; // namespace PSD::llapi
