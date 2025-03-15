
#pragma once

#include <psd/core/stream.h>
#include <psd/core/type/error.h>

#include <image/core/color.h>

namespace PSD {

class ColorIOError : public Error {
public:
  struct UndefinedColor;

private:
  ColorIOError(const std::string &msg) : Error(msg) {};

}; // ColorIOError

struct ColorIOError::UndefinedColor : ColorIOError {
public:

  UndefinedColor() : ColorIOError("UndefinedColor") {}

}; // ColorIOError::UndefinedColor
 
class Color : public Image::Color {
public:
  class Reader;
  class Writer;
};

class Color::Reader {
public:

  Reader(Stream &stream) : stream_(stream) {}

  Color::Tp Read() {
    switch (stream_.Read<std::uint16_t>()) {
      case 0: return Color::Bitmap; 
      case 1: return Color::Grayscale;
      case 2: return Color::Indexed;
      case 3: return Color::RGB;
      case 4: return Color::CMYK;
      case 7: return Color::Multichannel;
      case 8: return Color::Duotone;
      case 9: return Color::Lab;
      default: throw ColorIOError::UndefinedColor();
    }
  }

private:
  Stream &stream_;

}; // Color::Reader

template <>
class Reader<Color::Tp> : public Color::Reader {
public:
  Reader(Stream &stream) : Color::Reader(stream) {}

}; // Reader<Color::Tp>

class Color::Writer {
public:

  Writer(Stream &stream, Color::Tp color) 
    : stream_ (stream)
    , color_  (color) {}

  void Write() {
    std::uint16_t value;
    switch (color_) {
      case Color::Bitmap       : value = 0; break;
      case Color::Grayscale    : value = 1; break;
      case Color::Indexed      : value = 2; break;
      case Color::RGB          : value = 3; break;
      case Color::CMYK         : value = 4; break;
      case Color::Multichannel : value = 7; break;
      case Color::Duotone      : value = 8; break;
      case Color::Lab          : value = 9; break;
    }
    stream_.Write(value);
  }

private:
  Stream   &stream_;
  Color::Tp color_;

}; // Color::Writer

template <>
class Writer<Color::Tp> : public Color::Writer {
public:

  Writer(Stream &stream, Color::Tp color) : Color::Writer(stream, color) {}

}; // Writer<Color::Tp>
}; // PSD
