
#pragma once

#include <psd/core/length_calculator.h>
#include <psd/core/type/error.h>
#include <psd/core/stream.h>

#include <image/processing/blend.h>

namespace PSD {

class Blending : public Image::Blending {
public:

  class Reader;
  class Writer;

  using SignatureTp = std::array<std::uint8_t, 4>;
  static constexpr SignatureTp CorrectSignature = { 0x38, 0x42, 0x49, 0x4D };
};
class BlendingError : Error {
public:
  class UndefinedValue;
  class InvalidSignature;

private:
  explicit BlendingError(const std::string &msg) : Error(msg) {}
};
class BlendingError::UndefinedValue : public BlendingError {
public:
  explicit UndefinedValue(std::uint32_t input) : BlendingError(
    "Undefined Blending Value: " + std::to_string(input) + "\n"
    "Expected one of:            \n"
    " - PassThrough (0x70617373) \n"
    " - Normal      (0x6E6F726D) \n"
    " - Dissolve    (0x64697373) \n"
    " - Darken      (0x6461726B) \n"
    " - Multiply    (0x6D756C20) \n"
    " - ColorBurn   (0x00000000) \n"
    " - LinearBurn  (0x00000000) \n"
    " - DarkerColor (0x00000000) \n"
    " - Lighten     (0x00000000)"
  ) {}
};
class Blending::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  Blending::Tp Read() {
    auto value = stream_.Read<std::uint32_t>();
    switch (value) {
      case 0x70617373: return PassThrough;
      case 0x6E6F726D: return Normal;
      case 0x64697373: return Dissolve;
      case 0x6461726B: return Darken;
      case 0x6D756C20: return Multiply;
      default: throw BlendingError::UndefinedValue(value);
    }
  }
private:
  Stream &stream_;
};
PSD_REGISTER_READER_FOR_TYPE(Blending::Tp, Blending);

class Blending::Writer {
public:

  Writer(Stream &stream, Blending::Tp blend) : stream_(stream), input_(blend) {}

  void Write() {
    switch (input_) {
      case PassThrough: stream_.Write(0x70617373); break;
      case Normal:      stream_.Write(0x6E6F726D); break;
      case Dissolve:    stream_.Write(0x64697373); break;
      case Darken:      stream_.Write(0x6461726B); break;
      case Multiply:    stream_.Write(0x6D756C20); break;
      default: throw BlendingError::UndefinedValue(input_);
    }
  }

private:
  Stream &stream_;
  Blending::Tp input_;

};
PSD_REGISTER_WRITER_FOR_TYPE(Blending::Tp, Blending);
};
