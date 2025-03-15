
#pragma once

#include <psd/core/length_calculator.h>
#include <psd/core/stream.h>

namespace PSD {

class Rectangle {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  explicit Rectangle() = default;

  explicit Rectangle(
    std::uint32_t top,
    std::uint32_t left,
    std::uint32_t bottom,
    std::uint32_t right) 
    : top    (top)
    , left   (left)
    , bottom (bottom)
    , right  (right) {}

  bool operator==(const Rectangle &other) const {
    return 
    std::tie(this->top, this->left, this->bottom, this->right) ==
    std::tie(other.top, other.left, other.bottom, other.right);
  }
  bool operator!=(const Rectangle &other) const {
    return !operator==(other);
  }

  std::uint32_t top    = 0;
  std::uint32_t left   = 0;
  std::uint32_t bottom = 0;
  std::uint32_t right  = 0;

}; // Rectangle

class Rectangle::LengthCalculator {
public:
  LengthCalculator(const Rectangle &) {}

  constexpr std::uint64_t Calculate() const {
    return 4 * 4;
  }

}; // Rectangle::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(Rectangle);

class Rectangle::Reader {
public:
  Reader(Stream &stream) : stream_(stream) {}

  Rectangle Read() {
    auto rectangle = stream_.Read<std::uint32_t, 4>();
    return Rectangle(
      rectangle[0],
      rectangle[1],
      rectangle[2],
      rectangle[3]
    );
  }

private:
  Stream &stream_;

}; // Rectangle::Reader

PSD_REGISTER_READER(Rectangle);

class Rectangle::Writer {
public:

  Writer(Stream &stream, const Rectangle &rectangle) : stream_(stream), rectangle_(rectangle) {}

  void Write() {
    stream_.Write<std::uint32_t, 4>({
      rectangle_.top,
      rectangle_.left,
      rectangle_.bottom,
      rectangle_.right,
    });
  }

private:
  Stream &stream_;
  const Rectangle &rectangle_;

}; // Rectangle::Writer

PSD_REGISTER_WRITER(Rectangle);

} // PSD
