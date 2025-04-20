
#pragma once

#include <psd/core/length_calculator.h>
#include <psd/core/stream.h>
#include <psd/core/type/color.h>
#include <psd/core/type/depth.h>
#include <psd/core/type/version.h>

namespace PSD {

class Header {
public:
  Header() = default;

  using SignatureTp = std::array<std::uint8_t, 4>;

  class LengthCalculator;
  class Reader;
  class Writer;

  bool operator==(const Header &other) const {
    return version       == other.version       && 
           channel_count == other.channel_count &&
           row_count     == other.row_count     &&
           column_count  == other.column_count  && 
           depth         == other.depth         &&
           color         == other.color;
  }

  Version::Tp   version;
  std::uint16_t channel_count;
  std::uint32_t row_count;
  std::uint32_t column_count;
  Depth::Tp     depth;
  Color::Tp     color;

}; // Header;

class Header::LengthCalculator {
public:
  LengthCalculator(const Header &) {}

  constexpr std::uint64_t Calculate() const {
    return 26;
  }

}; // Header::LengthCalculator

template <>
class LengthCalculator<Header> : public Header::LengthCalculator {
public:
  LengthCalculator(const Header &input) : Header::LengthCalculator(input) {}

}; // LengthCalculator<Header>

class Header::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  Header Read() {
    Header output;

    ReadSignature();
    output.version = stream_.Read<Version::Tp>();
    ReadReserved();

    output.channel_count = stream_.Read<std::uint16_t>();
    output.row_count     = stream_.Read<std::uint32_t>();
    output.column_count  = stream_.Read<std::uint32_t>();
    output.depth        = stream_.Read<Depth::Tp>();
    output.color        = stream_.Read<Color::Tp>();

    return output;
  }

private:
  Stream &stream_;

  inline void ReadSignature() {
    if (stream_.Read<SignatureTp>() != SignatureTp({ 0x38, 0x42, 0x50, 0x53 })) {
      throw std::runtime_error("InvalidSignature");
    }
  }
  inline void ReadReserved() {
    stream_.AdjustPos(6);
  }
}; // Header::Reader

template <>
class Reader<Header> : public Header::Reader {
public:
  Reader(Stream &stream) : Header::Reader(stream) {}

}; // Reader<Header>

class Header::Writer {
public:

  Writer(Stream &stream, const Header &input) 
    : stream_ (stream)
    , input_  (input) {}

  void Write() {
    WriteSignature();
    stream_.Write(input_.version);
    WriteReserved();

    stream_.Write(input_.channel_count);
    stream_.Write(input_.row_count);
    stream_.Write(input_.column_count);
    stream_.Write(input_.depth);
    stream_.Write(input_.color);
  }

private:
  Stream &stream_;
  const Header &input_;

  inline void WriteSignature() {
    stream_.Write<SignatureTp>({ 0x38, 0x42, 0x50, 0x53 });
  }
  inline void WriteReserved() {
    stream_.Write<std::uint8_t, 6>({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
  }
}; 
template <>
class Writer<Header> : public Header::Writer {
public:
  Writer(Stream &stream, const Header &header) : Header::Writer(stream, header) {}
}; 
};

