
#pragma once

#include <psd/core/length_calculator.h>
#include <psd/core/stream.h>

namespace PSD {

class Blend {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  enum Tp : std::uint32_t {
    PassThrough = 0x70617373,
    Normal      = 0x6E6F726D,
    Dissolve    = 0x64697373
  };

  using UnderlyingTp = std::underlying_type_t<Tp>;

  template <Tp Value>
  static constexpr UnderlyingTp UnderlyingValue = static_cast<UnderlyingTp>(Value);

};

class Blend::LengthCalculator {
public:
  LengthCalculator(const Blend::Tp &) {}

  std::uint64_t Calculate() const { 
    return 4; 
  }

}; // Blend::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_TYPE(Blend::Tp, Blend);

class Blend::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  Blend::Tp Read() {
    return static_cast<Blend::Tp>(stream_.Read<UnderlyingTp>());
  }

private:
  Stream &stream_;

}; // Blend::Reader

PSD_REGISTER_READER_FOR_TYPE(Blend::Tp, Blend);

class Blend::Writer {
public:

  Writer(Stream &stream, Blend::Tp blend) : stream_(stream), blend_(blend) {}

  void Write() {
    stream_.Write(static_cast<UnderlyingTp>(blend_));
  }

private:
  Stream &stream_;
  Blend::Tp blend_;

}; // Blend::Writer 

PSD_REGISTER_WRITER_FOR_TYPE(Blend::Tp, Blend);

}; // PSD
