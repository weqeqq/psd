
#pragma once

#include "psd/document/detail/group_processor.h"
#include "psd/document/detail/root_converter.h"
#include "psd/llapi/structure/header.h"
#include "psd/llapi/structure/info/layer_info/channel_data.h"
#include "psd/llapi/structure/resource_info.h"
#include <iterator>

namespace PSD {
//
using Compression = llapi::Compression;
using Color       = llapi::Color;
using Depth       = llapi::Depth;
class Document {
  Document(detail::Root root) : root_(root) {}

  friend class ExportFn;
  friend class OpenFn;
  friend class SaveFn;
public:
  Document() = default;

  bool operator==(const Document &other) const {
    return root_ == other.root_;
  }
  bool operator!=(const Document &other) const {
    return !operator==(other);
  }
  std::shared_ptr<Entry> operator[](unsigned index) {
    return root_[index];
  }
  std::shared_ptr<const Entry> operator[](unsigned index) const {
    return root_[index];
  }
  template <typename T>
  void Push(T &&entry) {
    root_.Push(std::forward<T>(entry));
  }
  auto begin()        { return root_.begin(); }
  auto begin() const  { return root_.begin(); }
  auto end()          { return root_.end();   }
  auto end() const    { return root_.end();   }

  unsigned RowCount()
  const { return root_.Bottom() - root_.Top(); }
  unsigned ColumnCount()
  const { return root_.Right() - root_.Left(); }

  void SetCompression(llapi::Compression compr) {
    compression_ = compr;
  }
  void SetCompressionLevel(unsigned level = 6) {
    compression_level_ = level;
  }
  void SetColor(llapi::Color color) {
    color_ = color;
  }

  llapi::Compression Compression() const {
    return compression_;
  }
  unsigned CompressionLevel() const {
    return compression_level_;
  }
  llapi::Color Color() const {
    return color_;
  }

  void ToggleRendering() {
    rendering_enabled_ = !rendering_enabled_;
  }
  bool RenderingEnabled() const {
    return rendering_enabled_;
  }

private:
  detail::Root root_;
  llapi::Compression compression_ = llapi::Compression::None;
  unsigned compression_level_ = 6;
  llapi::Color color_ = llapi::Color::Rgb;

  bool rendering_enabled_ = false;

}; // class Document
class DocumentCreator {
public:
  DocumentCreator() = default;
  DocumentCreator(class Document document)
    : document_(std::move(document)) {}

  template <typename T>
  DocumentCreator &Push(T &&input) {
    document_.Push(std::forward<T>(input));
    return *this;
  }
  DocumentCreator &Color(llapi::Color input) {
    document_.SetColor(input);
    return *this;
  }
  DocumentCreator &Compression(llapi::Compression input) {
    document_.SetCompression(input);
    return *this;
  }
  class Document Document() {
    return std::exchange(document_, PSD::Document());
  }
private:
  class Document document_;
};
class OpenFn {
public:
  Document operator()(const std::filesystem::path &path) const {
    return CreateDocument(
      llapi::ConvertColor(
        llapi::ConvertDepth(
          llapi::Decompress(llapi::StructureFrom(path)),
          Depth::Eight
        ),
        Color::Rgb
      )
    );
  }
private:
  Document CreateDocument(llapi::Structure input) const {
    return DocumentCreator(detail::ConvertRoot(input.info.layer_info))
      .Color(input.header.color)
      .Document();
  }
}; // class OpenFn
inline constexpr auto Open = OpenFn();

class ExportFn {
public:
  ::Image::Buffer<> operator()(const Document &input) const {
    return detail::ProcessGroup(input.root_);
  }
  ::Image::Buffer<> operator()(const std::filesystem::path &path) const {
    return operator()(Open(path));
  }
}; // class ExportFn
inline constexpr ExportFn Export = ExportFn();

class SaveFn {
  auto CreateHeader(
    const Document &input
  ) const {
    return llapi::Header(input.RowCount(), input.ColumnCount());
  }
  auto CreateResourceInfo(
    const Document &input
  ) const {
    return llapi::ResourceInfo();
  }
  auto CreateInfo(
    const Document &input
  ) const {
    return llapi::Info(detail::ConvertRoot(input.root_));
  }
  auto CreateImage(
    const Document &input
  ) const {
    if (input.rendering_enabled_) {
      return ProcessImage(detail::ProcessGroup(input.root_));
    } else {
      return llapi::Image(std::vector<llapi::U8>(
        input.RowCount() * input.ColumnCount() * 3, 0x00
      ));
    }
  }
public:
  void operator()(const Document &input, const std::filesystem::path &path) const {
    llapi::DumpStructure(
      llapi::Compress(
        llapi::ConvertColor(
          llapi::Structure(
            CreateHeader       (input),
            CreateResourceInfo (input),
            CreateInfo         (input),
            CreateImage        (input)
          ),
          input.color_
        ),
        input.compression_,
        input.compression_level_
      ),
      path
    );
  }
private:
  llapi::Image ProcessImage(const ::Image::Buffer<> &input) const {
    llapi::Image output = llapi::Image(std::vector<llapi::U8>(input.Length() * input.ChannelCount()));
    for (auto channel = 0u;
              channel < input.ChannelCount()-1;
              channel++) {
      for (auto index = 0u;
                index < input.Length();
                index++) {
        output.data[input.Length() * channel + index] = input[index][channel];
      }
    }
    return output;
  }
}; // class SaveFn

inline constexpr auto Save = SaveFn();

namespace detail {
class DecodeToFn {
public:
  template <typename I>
  void operator()(I output, I end, unsigned channel_count, unsigned length, const llapi::Image &image) const {
    if (std::distance(output, end) < (channel_count * length)) {
      throw Error("PSD::Error: DecodeError");
    }
    for (auto channel = 0u;
              channel < channel_count;
              channel++) {
      for (auto index = 0u;
                index < length;
                index++) {
        *(output + (index * channel_count) + channel) = image.data[length * channel + index];
      }
    }
  }
}; // class DecodeToFn
inline constexpr auto DecodeTo = DecodeToFn();
} // namespace detail
class DecodeFn {
public:
  ::Image::Buffer<> operator()(const std::filesystem::path &path) const {
    llapi::Stream stream(path);
    auto header = stream.Read<llapi::Header>();
    stream     += stream.Read<llapi::U32>(); // skip color info
    stream     += stream.Read<llapi::U32>(); // skip resource info
    stream     += stream.Read<llapi::U32>(); // skip info
    auto image  = stream.Read<llapi::Image>();
    image.Decompress(header);
    ::Image::Buffer<> output(
      header.row_count,
      header.column_count,
      header.channel_count
    );
    detail::DecodeTo(
      output.begin(),
      output.end(),
      output.ChannelCount(),
      output.Length(),
      image
    );
    return output;
  }
}; // class DecodeFn

inline constexpr auto Decode = DecodeFn();

}; // namespace PSD
