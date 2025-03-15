
#include <psd/core/decompressor.h>

namespace PSD {

class RLEDecompressor {
public:

  RLEDecompressor(const std::vector<std::uint8_t> &input) : input_(input) {}

  std::vector<std::uint8_t> Decompress(std::uint64_t row_count, std::uint64_t column_count) const {
    auto input_iterator = input_.begin();

    std::vector<std::uint16_t> count_list(row_count);
    for (auto &count : count_list) {
      std::uint8_t *ccount = reinterpret_cast<std::uint8_t *>(&count);
      ccount[0] = *input_iterator++;
      ccount[1] = *input_iterator++;
      count = Image::ConvertEndianness<Image::Endianness::Big>(count);
    }
    std::vector<std::uint8_t> output(row_count * column_count); 
    auto output_iterator = output.begin();

    for (auto &count : count_list) {
      DecompressLine(
        input_iterator,
        input_iterator + count,
        output_iterator,
        output_iterator + column_count
      ); 
      input_iterator  += count;
      output_iterator += column_count;
    }
    return output;
  }

private:
  const std::vector<std::uint8_t> &input_;

  void DecompressLine(
    std::vector<std::uint8_t>::const_iterator input_iterator,
    std::vector<std::uint8_t>::const_iterator input_end,
    std::vector<std::uint8_t>::iterator       output_iterator,
    std::vector<std::uint8_t>::iterator       output_end
  ) const {

    while (input_iterator != input_end) {
      std::int16_t count = static_cast<std::int8_t>(*input_iterator++);  
      if (count < 0) {
        count = -count + 1;

        if (input_iterator == input_end) {
          throw std::runtime_error("Unexpected end of input");
        }
        std::int8_t value = *input_iterator++;
        for (; count > 0; count--) {
          *output_iterator++ = value;
        }
      } else {
        count++;
        for (; count > 0; count--) {
          if (output_iterator == output_end || input_iterator == input_end) {
            throw std::runtime_error("Unexpected end of input");
          }
          *output_iterator++ = *input_iterator++;
        }
      }
    }
  }

}; // Decompressor

std::vector<std::uint8_t> Decompressor<std::vector<std::uint8_t>>::DecompressRLE(
  std::uint64_t row_count, std::uint64_t column_count
) const {
  return RLEDecompressor(input_).Decompress(row_count, column_count);
}

}; // PSD::_Decompressor
