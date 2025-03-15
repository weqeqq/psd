
#pragma once

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>

namespace PSD {

class ColorInfo {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  ColorInfo() = default;

  bool operator==(const ColorInfo &other) const {
    return data_ == other.data_;
  }
  bool operator!=(const ColorInfo &other) const {
    return !operator==(other);
  }

private:
  std::uint64_t CalculateContentLength() const {
    return data_.size();
  }

  std::vector<std::uint8_t> data_;
  
}; // ColorInfo

class ColorInfo::LengthCalculator {
public:

  LengthCalculator(const ColorInfo &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return input_.CalculateContentLength() + sizeof(std::uint32_t);
  }

private:
  const ColorInfo &input_;

}; // ColorInfo::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(ColorInfo);

class ColorInfo::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  ColorInfo Read() {
    ColorInfo output;

    output.data_ = stream_.Read<std::uint8_t>(stream_.Read<std::uint32_t>());

    return output;
  }

private:
  Stream &stream_;
  
}; // ColorInfo::Reader

PSD_REGISTER_READER(ColorInfo);

class ColorInfo::Writer {
public:

  Writer(Stream &stream, const ColorInfo &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(std::uint32_t(
      input_.CalculateContentLength()
    ));
    stream_.Write(input_.data_);
  }

private:
  Stream &stream_;
  const ColorInfo &input_;

}; // ColorInfo::Writer 

PSD_REGISTER_WRITER(ColorInfo);
  
} // PSD

