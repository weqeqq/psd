
#include <psd/core/compressor.h>

#include <xsimd/xsimd.hpp>

namespace PSD {

class RLECompressor {
public:
  using ByteSequence = std::vector<std::uint8_t>;

  explicit RLECompressor(const ByteSequence &input) : input_(input) {}

  ByteSequence Compress(
    std::uint64_t row_count, 
    std::uint64_t column_count
  ) {
    ByteSequence output(row_count * sizeof(std::uint16_t));

    auto input_iterator = input_.begin();

    output.reserve(output.size() + row_count * column_count);

    for (auto index = 0ul;
              index < row_count;
              index++) {
      union {
        std::uint16_t value;
        std::uint8_t  array[sizeof value];
      } count;
      auto output_end = output.end();
      InsertCompressed(
        output,
        input_iterator  - column_count,
        input_iterator += column_count
      );
      count.value = Image::ConvertEndianness<Image::Endianness::Big>(
        static_cast<std::uint16_t>(std::distance(
          output_end,
          output.end()
        ))
      );
      output[(index * sizeof count) + 0] = count.array[0];
      output[(index * sizeof count) + 1] = count.array[1];
    }
    return output;
  }

  using Batch = xsimd::batch<std::uint8_t>;

  static constexpr std::uint64_t BatchLength       = Batch::size;
  static constexpr std::uint64_t BatchDoubleLength = Batch::size * 2;

  void InsertCompressed(
    ByteSequence &output,
    ByteSequence::const_iterator iterator,
    ByteSequence::const_iterator end
  ) const {
    while (iterator != end) {
      std::uint8_t value = *iterator++;
      std::int16_t count = 1;

      while (iterator != end && count < 128) {
        std::uint64_t remaining = std::distance(iterator, end);

        if (remaining >= Batch::size && count + Batch::size < 128) {
          auto compare = xsimd::load_unaligned(&*iterator) == Batch(value);

          if (xsimd::all(compare)) {
            count    += Batch::size;
            iterator += Batch::size;

            continue;
          }
          int matching = __builtin_ctz(~compare.mask());
          count       += matching;
          iterator    += matching;
          break;

        } else {
          if (*iterator != value) {
            break;
          }
          count++;
          iterator++;
        }
      }
      if (count > 1 && count != 2) {
        output.push_back(-(count - 1));
        output.push_back(value);
      } else {

        while ((iterator + 0) != end &&
               (iterator + 1) != end && count < 128) {
          std::uint64_t remaining = std::distance(iterator, end);

          if (remaining >= BatchDoubleLength && count + BatchDoubleLength < 128) {
            auto compare = xsimd::load_unaligned(&*(iterator)) == 
                           xsimd::load_unaligned(&*(iterator + BatchLength));

            if (xsimd::none(compare)) {
              count    += BatchDoubleLength;
              iterator += BatchDoubleLength;
              continue;
            }
            auto non_matching = __builtin_ctz(compare.mask());
            count             += non_matching;
            iterator          += non_matching; 
            break;

          } else {
            if (*(iterator + 0) == 
                *(iterator + 1)) {
              break;
            }
            count++;
            iterator++;
          }
        }
        output.push_back(count - 1);
        output.insert(output.end(), iterator - count, iterator);
      }
    }
  }
private:
  const ByteSequence &input_;
};

std::vector<std::uint8_t> Compressor<std::vector<std::uint8_t>>::CompressRLE(
  std::uint64_t row_count_, std::uint64_t column_count_
) const {
  return RLECompressor(input_).Compress(row_count_, column_count_);
}

};
