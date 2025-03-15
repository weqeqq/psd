#pragma once

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>
#include <psd/core/type/error.h>

namespace PSD {

class Compression {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  Compression() = delete;

  enum Tp : std::uint16_t {
    RAW,
    RLE,
    ZIP,
    ZIPP

  }; // Tp

  using UnderlyingTp = std::underlying_type_t<Tp>;

  template <Tp Value>
  static constexpr UnderlyingTp UnderlyingValue = static_cast<UnderlyingTp>(Value); 

}; // Compression

class CompressionIOError : public Error {
public:

  struct UndefinedCompression;

protected:
  CompressionIOError(const std::string &msg) : Error(msg) {}

}; // CompressionIOError

struct CompressionIOError::UndefinedCompression : public CompressionIOError {
  UndefinedCompression() : CompressionIOError("Undefined Comperssion") {}

}; // CompressionIOError::UndefinedCompression

class Compression::LengthCalculator {
public:
  explicit LengthCalculator(Compression::Tp) {}

  constexpr std::uint64_t Calculate() const { 
    return 2; 
  }

}; // Compression::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_TYPE(Compression::Tp, Compression);

class Compression::Reader {
public:
  explicit Reader(Stream &stream) : stream_(stream) {}

  Compression::Tp Read() {
    switch (stream_.Read<UnderlyingTp>()) {
      case UnderlyingValue<Compression::RAW>  : return Compression::RAW;
      case UnderlyingValue<Compression::RLE>  : return Compression::RLE;
      case UnderlyingValue<Compression::ZIP>  : return Compression::ZIP;
      case UnderlyingValue<Compression::ZIPP> : return Compression::ZIPP;
      default: throw CompressionIOError::UndefinedCompression();
    }
  }

private:
  Stream &stream_;

}; // Compression::Reader

PSD_REGISTER_READER_FOR_TYPE(Compression::Tp, Compression);

class Compression::Writer {
public:

  explicit Writer(Stream &stream, Compression::Tp input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(static_cast<UnderlyingTp>(input_));
  }

private:
  Stream &stream_;
  Compression::Tp input_;

}; // Compression::Writer 

PSD_REGISTER_WRITER_FOR_TYPE(Compression::Tp, Compression);

}; // PSD
