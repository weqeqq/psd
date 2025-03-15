
#include <psd/core/compressor.h>

namespace PSD {

class RLECompressor {
public:

  RLECompressor(const std::vector<std::uint8_t> &input) : input_(input) {}

  std::vector<std::uint8_t> Compress(
    std::uint64_t row_count, std::uint64_t column_count
  ) const {
    std::vector<std::uint8_t>              output(row_count * sizeof(std::uint16_t));
    std::vector<std::vector<std::uint8_t>> data_list(row_count);

    auto input_iterator  = input_.begin();
    auto output_iterator = output.begin();
    for (auto &data : data_list) {
      data = Compress(
        input_iterator,
        input_iterator + column_count
      );
      input_iterator += column_count;
      union {
        std::uint16_t value;
        std::uint8_t  array[sizeof(value)];
      } count;
      count.value = Image::ConvertEndianness<Image::Endianness::Big>(
        std::uint16_t(data.size())
      );
      *output_iterator++ = count.array[0];
      *output_iterator++ = count.array[1];
    }
    for (auto &data : data_list) {
      output.insert(output.end(), data.begin(), data.end());
    }
    return output;
  }

private:
  const std::vector<std::uint8_t> &input_;

  class RLE;

  std::vector<std::uint8_t> Compress(
    std::vector<std::uint8_t>::const_iterator iterator,
    std::vector<std::uint8_t>::const_iterator end
  ) const {
    std::vector<std::uint8_t> output;
    output.reserve(end - iterator);

    while (iterator != end) {
      std::uint8_t value = *iterator++;
      std::int16_t count = 1;

      while (iterator != end && *iterator == value) {
        count++;
        iterator++;

        if (count == 128) {
          break;
        }
      }
      if (count > 1 && count != 2) {
        output.push_back(-(count - 1));
        output.push_back(value);
      } else {

        while (true) {

          while (iterator != end && *iterator != *(iterator - 1)) {
            count++;
            iterator++;

            if (count == 128) {
              break;
            }
          }

          if (count == 128) {
            if (iterator != end && *iterator == *(iterator - 1)) {
              if (iterator + 1 == end) {
                break;
              }
              if (*iterator != *(iterator + 1)) {
                break;
              }
              iterator--;
              count--;
            }
            break;
          }

          if (iterator != end && *iterator == *(iterator - 1)) {
            if (count == 127) {
              if (iterator + 1 == end) {
                break;
              }
              if (*iterator == *(iterator + 1)) {
                iterator--;
                count--;
                break;
              }
              if (*iterator != *(iterator + 1)) {
                iterator++;
                count++;
                break;
              }
            }

            if (iterator + 1 == end) {
              iterator++;
              count++;
              break;
            }
            if (*iterator != *(iterator + 1)) {
              iterator++;
              count++;
              continue;
            }
            count--;
            iterator--;
          }
          break; 
        }
        output.push_back(count - 1);
        output.insert(output.end(), iterator - count, iterator);
      }
    }
    return output;
  }

}; // Compressor

std::vector<std::uint8_t> Compressor<std::vector<std::uint8_t>>::CompressRLE(
  std::uint64_t row_count_, std::uint64_t column_count_
) const {
  return RLECompressor(input_).Compress(row_count_, column_count_);
}

}; // PSD::_Compressor
