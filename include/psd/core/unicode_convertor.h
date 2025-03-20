
#pragma once

#include <string>
#include <cstdint>
#include <locale>
#include <codecvt>

namespace PSD {

template <typename CharT>
class UnicodeConvertor {
public:

  explicit UnicodeConvertor(const std::basic_string<CharT> &input) : input_(input) {}

  template <typename OutputCharT>
  using OutputStringTp = std::conditional_t<std::is_same_v<OutputCharT, CharT>, 
    const std::basic_string<OutputCharT> &,
          std::basic_string<OutputCharT>
  >;
  template <typename OutputCharT>
  OutputStringTp<OutputCharT> Convert() const {
    if constexpr (std::is_same_v<OutputCharT, CharT>) {
      return input_;
    } else if constexpr (std::is_same_v<OutputCharT, char>) {
      return ConvertToByteSequence();
    } else if constexpr (std::is_same_v<OutputCharT, char16_t>) {
      return std::wstring_convert<std::codecvt_utf8_utf16<
        OutputCharT>, 
        OutputCharT>().from_bytes(ConvertToByteSequence());
    } else if constexpr (std::is_same_v<OutputCharT, char32_t>) {
      return std::wstring_convert<std::codecvt_utf8<
        OutputCharT>,
        OutputCharT>().from_bytes(ConvertToByteSequence());
    } else {
      static_assert(false);
    }
  }
private:
  const std::basic_string<CharT> &input_;

  using ByteSequenceTp = std::conditional_t<std::is_same_v<CharT, char>, 
    const std::basic_string<char> &,
          std::basic_string<char>
  >;
  ByteSequenceTp ConvertToByteSequence() const {
    if constexpr (std::is_same_v<CharT, char>) {
      return input_;
    } else if constexpr (std::is_same_v<CharT, char16_t>) {
      return std::wstring_convert<std::codecvt_utf8_utf16<
        CharT>, 
        CharT>().to_bytes(input_);
    } else if constexpr (std::is_same_v<CharT, char32_t>) {
      return std::wstring_convert<std::codecvt_utf8<
        CharT>, 
        CharT>().to_bytes(input_);
    } else {
      static_assert(false);
    }
  }
}; // UnicodeConvertor
  
}; // PSD

