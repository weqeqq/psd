
#pragma once

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>
#include <psd/core/unicode_convertor.h>

#include <string>
#include <cstdint>

namespace PSD {

class UnicodeString {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  explicit UnicodeString() = default;

  template <typename CharT>
  UnicodeString(const std::basic_string<CharT> &string) 
    : string_(UnicodeConvertor(string).template Convert<char32_t>()) {}

  template <typename CharT>
  UnicodeString(const CharT *string) 
    : UnicodeString(std::basic_string<CharT>(string)) {}

  template <typename CharT>
  UnicodeString &operator=(const std::basic_string<CharT> &string) {
    string_ = UnicodeConvertor(string).template Convert<char32_t>();
    return *this;
  }
  template <typename CharT> 
  UnicodeString &operator=(const CharT *string) {
    return operator=(std::basic_string<CharT>(string));
  }
  friend std::ostream &operator<<(std::ostream &stream, const UnicodeString &input);

  using Iterator = std::basic_string<char32_t>::iterator;
  using ConstIterator = std::basic_string<char32_t>::const_iterator;

  friend Iterator begin(UnicodeString &input);
  friend ConstIterator begin(const UnicodeString &input);

  friend Iterator end(UnicodeString &input);
  friend ConstIterator end(const UnicodeString &input);

  bool operator==(const UnicodeString &other) const {
    return string_ == other.string_;
  }
  bool operator!=(const UnicodeString &other) const {
    return !operator==(other);
  }
  template <typename CharT>
  bool operator==(const CharT *other) const {
    return operator==(std::basic_string<CharT>(other));
  }
  template <typename CharT>
  bool operator!=(const CharT *other) const {
    return !operator==(other);
  }

  char32_t &operator[](std::uint64_t index) {
    return string_[index];
  }
  char32_t operator[](std::uint64_t index) const {
    return string_[index];
  }

  template <typename CharT>
  std::enable_if_t<
    std::is_same_v<CharT, char>     ||
    std::is_same_v<CharT, char16_t> || 
    std::is_same_v<CharT, char32_t> 
  > Push(CharT character) {
    string_.push_back(character);
  }

private:
  std::basic_string<char32_t> string_;

}; // UnicodeString

std::ostream &operator<<(std::ostream &stream, const UnicodeString &input) {
  stream << UnicodeConvertor(input.string_).Convert<char>();
  return stream;
}

UnicodeString::Iterator begin(UnicodeString &input) {
  return input.string_.begin();
}
UnicodeString::ConstIterator begin(const UnicodeString &input) {
  return input.string_.begin();
}

UnicodeString::Iterator end(UnicodeString &input) {
  return input.string_.end();
}
UnicodeString::ConstIterator end(const UnicodeString &input) {
  return input.string_.end();
}

class UnicodeString::LengthCalculator {
public:

  explicit LengthCalculator(const UnicodeString &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return 4 + (UnicodeConvertor(input_.string_).Convert<char16_t>().size() * sizeof(char16_t));
  }

private:
  const UnicodeString &input_;
  
}; // UnicodeString::LengthCalculator
PSD_REGISTER_LENGTH_CALCULATOR(UnicodeString);

class UnicodeString::Reader {
public:

  explicit Reader(Stream &stream) : stream_(stream) {}

  UnicodeString Read() {

    std::uint32_t               length = stream_.Read<std::uint32_t>();
    std::basic_string<char16_t> string;

    string.reserve(length);

    for (auto index = 0u; 
              index < length; 
              index++) {
      char16_t character = stream_.Read<char16_t>();

      if (character >= 0xD800 && character <= 0xDBFF) {
        char16_t low_surrogate = stream_.Read<char16_t>();

        if (low_surrogate >= 0xDC00 && low_surrogate <= 0xDFFF) {
          string.push_back(character);
          string.push_back(low_surrogate);
        } else {
          throw std::runtime_error("Invalid UTF-16 surrogate pair");
        }
      } else {
        string.push_back(character);
      }
    }
    return UnicodeString(string);
  }
private:
  Stream &stream_;

}; // UnicodeString::Reader
PSD_REGISTER_READER(UnicodeString);

class UnicodeString::Writer {
public:

  explicit Writer(Stream &stream, const UnicodeString &input) : stream_(stream), input_(input) {} 

  void Write() {
    stream_.Write<std::uint32_t>(input_.string_.size());
    for (const auto &character : UnicodeConvertor(input_.string_).Convert<char16_t>()) {
      stream_.Write(character);
    }
  }
private:
  Stream &stream_;
  const UnicodeString &input_;

}; // UnicodeString::Writer
PSD_REGISTER_WRITER(UnicodeString);

}; // PSD

