
#pragma once

#include "psd/error.h"
#include <map>
#include <psd/llapi/stream.h>
#include <psd/export.h>
#include <type_traits>

namespace PSD::llapi {
//
enum class Compression : U16 {
  None    = 0,
  Default = 1,
}; // enum class Compression
template<>
struct FromStreamFn<Compression> {
  void operator()(Stream &stream, Compression &output) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<Compression> &>(output));
    if (output != Compression::None &&
        output != Compression::Default) throw Error("PSD::Error: UnsupportedCompression");
  }
}; // FromStreamFn<Compression>
template <>
struct ToStreamFn<Compression> {
  void operator()(Stream &stream, Compression input) {
    stream.Write(static_cast<std::underlying_type_t<Compression>>(input));
  }
}; // ToStreamFn<Compression>

PSD_EXPORT std::vector<U8>
Decompress(const std::vector<U8> &input, unsigned row_count, unsigned column_count);

PSD_EXPORT std::vector<U8>
Compress(const std::vector<U8> &input, unsigned row_count, unsigned column_count);

class ChannelData {
  struct FromStreamFn {
    void operator()(Stream &stream, ChannelData &output, const std::map<I16, U32> &channel_info) {
      for (auto [channel, length] : channel_info) {
        stream.ReadTo(output.data[channel].first);
        stream.ReadTo(output.data[channel].second, length - 2);
      }
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const ChannelData &input) {
      for (const auto &[channel, data] : input.data) {
        stream.Write(data);
      }
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  ChannelData() = default;
  bool operator==(const ChannelData &other) const {
    return data == other.data;
  }
  bool operator!=(const ChannelData &other) const {
    return !operator==(other);
  }
  std::map<I16, std::pair<Compression, std::vector<U8>>> data;
  unsigned Length() const {
    auto output = 0u;
    for (const auto &[channel, data] : data) {
      output += 2 + data.second.size();
    }
    return output;
  }
  void Decompress(unsigned row_count, unsigned column_count) {
    for (auto &[channel, pair] : data) {
      auto &[compression, data] = pair;
      switch (compression) {
        case Compression::None    : {break;}
        case Compression::Default : data = llapi::Decompress(data, row_count, column_count);
      }
      compression = Compression::None;
    }
  }
  void Compress(Compression ocompression, unsigned row_count, unsigned column_count) {
    for (auto &[channel, pair] : data) {
      auto &[compression, data] = pair;
      switch (ocompression) {
        case Compression::None    : {break;}
        case Compression::Default : throw Error("Unsupported");
      }
      compression = ocompression;
    }
  }
}; // class ChannelData
}; // namespace PSD::llapi
