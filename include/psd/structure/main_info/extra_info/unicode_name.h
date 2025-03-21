
#pragma once

#include <psd/structure/main_info/extra_info_element.h>
#include <psd/core/unicode_string.h>

namespace PSD {

class UnicodeName : public ExtraInfoElement  {
public:
  static constexpr std::uint32_t ID = 0x6C756E69;

  explicit UnicodeName() = default;
  explicit UnicodeName(const std::string &name) : name(name) {}

  bool operator==(const UnicodeName &other) const {
    return name == other.name;
  }
  bool operator!=(const UnicodeName &other) const {
    return !operator==(other);
  }

  std::uint32_t GetID()            const override { return ID; }
  std::uint64_t GetContentLength() const override { return PSD::LengthCalculator(name).Calculate(); }

  void ReadContent(Stream &stream, const Header &header) override {
    name = stream.Read<decltype(name)>();
  }
  void WriteContent(Stream &stream) const override {
    stream.Write(name);
  }
  PSD_EXTRA_INFO_ELEMENT_IMPLEMENT_CLONE   (UnicodeName);
  PSD_EXTRA_INFO_ELEMENT_IMPLEMENT_COMPARE (UnicodeName);

  UnicodeString name;

}; // UnicodeName
}; // PSD
