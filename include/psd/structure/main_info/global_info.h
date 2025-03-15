
#pragma once

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>

namespace PSD {

class GlobalInfo {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  explicit GlobalInfo() = default;

  bool operator==(const GlobalInfo &) const {
    return true;
  }
  bool operator!=(const GlobalInfo &) const {
    return false;
  }

private:
  constexpr std::uint64_t CalculateContentLength() const {
    return 0;
  }

}; // GlobalInfo

class GlobalInfo::LengthCalculator {
public:
  explicit LengthCalculator(const GlobalInfo &input) : input_(input) {}

  constexpr std::uint64_t Calculate() const {
    return input_.CalculateContentLength() + 4;
  }

private:
  const GlobalInfo &input_;

}; // GlobalInfo::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(GlobalInfo);

class GlobalInfo::Reader {
public:
  explicit Reader(Stream &stream) : stream_(stream) {}

  GlobalInfo Read() {
    stream_.AdjustPos(stream_.Read<std::uint32_t>());

    return GlobalInfo();
  }

private:
  Stream &stream_;

}; // GlobalInfo::Reader

PSD_REGISTER_READER(GlobalInfo);

class GlobalInfo::Writer {
public:
  explicit Writer(Stream &stream, const GlobalInfo &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(std::uint32_t(
      input_.CalculateContentLength()
    ));
  }

private:
  Stream &stream_;
  const GlobalInfo &input_;

}; // GlobalInfo::Writer

PSD_REGISTER_WRITER(GlobalInfo);

}; // PSD

