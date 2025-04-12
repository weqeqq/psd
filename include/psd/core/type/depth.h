
#pragma once

#include <psd/core/stream.h>
#include <psd/core/type/error.h>
#include <image/core/depth.h>

namespace PSD {

class DepthIOError : public Error {
public:
  struct UndefinedDepth;

protected:
  DepthIOError(const std::string &msg) : Error(msg) {}

}; // DepthIOError

struct DepthIOError::UndefinedDepth : DepthIOError {
  UndefinedDepth() : DepthIOError("UndefinedDepth") {}

}; // DepthIOError::UndefinedDepth

class Depth : public Image::Depth {
public:
  class Reader;
  class Writer;

}; // Depth

class Depth::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  Depth::Tp Read() {
    switch (stream_.Read<std::uint16_t>()) {
      case 1  : return Depth::One;
      case 8  : return Depth::Eight;
      case 16 : return Depth::Sixteen;
      case 32 : return Depth::ThirtyTwo;
      default: throw DepthIOError::UndefinedDepth();
    }
  }

private:
  Stream &stream_;
};
template <>
class Reader<Depth::Tp> : public Depth::Reader {
public:
  Reader(Stream &stream) : Depth::Reader(stream) {}
};
class Depth::Writer {
public:

  Writer(Stream &stream, Depth::Tp depth) 
    : stream_ (stream)
    , depth_  (depth) {}

  void Write() {
    std::uint16_t value;
    switch (depth_) {
      case Depth::One       : value = 0;  break;
      case Depth::Eight     : value = 8;  break;
      case Depth::Sixteen   : value = 16; break;
      case Depth::ThirtyTwo : value = 32; break;
    }
    stream_.Write(value);
  }
private:
  Stream   &stream_;
  Depth::Tp depth_;
}; 
template <>
class Writer<Depth::Tp> : public Depth::Writer {
public:
  Writer(Stream &stream, Depth::Tp depth) : Depth::Writer(stream, depth) {}
}; 
static constexpr Depth::Tp DefDepth = Depth::Eight;

};
