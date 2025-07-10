
#pragma once

#include "psd/document/detail/group_processor.h"
#include "psd/document/detail/root_converter.h"
#include "psd/llapi/structure/header.h"
#include "psd/llapi/structure/info/layer_info/channel_data.h"

namespace PSD {
//
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

  unsigned RowCount() const {
    return root_.Bottom() - root_.Top();
  }
  unsigned ColumnCount() const {
    return root_.Right() - root_.Left();
  }

private:
  detail::Root root_;

}; // class Document


class OpenFn {
public:
  Document operator()(const std::filesystem::path &path) const {
    llapi::Structure structure; llapi::Stream stream(path);
    stream.ReadTo(structure.header);
    stream.ReadTo(structure.color_info);
    stream.ReadTo(structure.resource_info);
    stream.ReadTo(structure.info);
    structure.Decompress();
    return Document(
      detail::RootConverter<llapi::LayerInfo>()(structure.info.layer_info)
    );
  }
}; // class OpenFn

inline constexpr auto Open = OpenFn();

class ExportFn {
public:
  ::Image::Buffer<> operator()(const Document &input) const {
    return detail::GroupProcessor()(input.root_);
  }
  ::Image::Buffer<> operator()(const std::filesystem::path &path) const {
    return operator()(Open(path));
  }
}; // class ExportFn
inline constexpr ExportFn Export = ExportFn();

class SaveFn {
public:
  void operator()(const Document &document, const std::filesystem::path &path) const {
    llapi::Structure structure;
    structure.header = llapi::Header(document.RowCount(), document.ColumnCount());
    structure.info.layer_info = detail::RootConverter<detail::Root>()(document.root_);
    structure.image = ProcessImage(detail::GroupProcessor()(document.root_));
    structure.Compress(llapi::Compression::None);
    llapi::Stream stream;
    stream.Write(structure);
    stream.Dump(path);
  }
private:
  llapi::Image ProcessImage(const ::Image::Buffer<> &input) const {
    llapi::Image output = {llapi::Compression::None, std::vector<llapi::U8>(input.Length() * 3)};
    for (auto channel = 0u;
              channel < 3;
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

class DecodeFn {
public:
  ::Image::Buffer<> operator()(const std::filesystem::path &path) const {
    llapi::Stream stream(path);
    auto header = stream.Read<llapi::Header>();
    stream     += stream.Read<llapi::U32>(); // skip color info
    stream     += stream.Read<llapi::U32>(); // skip resource info
    stream     += stream.Read<llapi::U32>(); // skip info
    auto image  = stream.Read<llapi::Image>();
    image.Decompress(header.row_count, header.column_count);
    return ProcessImage(header, image);
  }
private:
  ::Image::Buffer<> ProcessImage(const llapi::Header &header, const llapi::Image &input) const {
    ::Image::Buffer<> output(std::vector<llapi::U8>(
      header.row_count    *
      header.column_count * 4,
      0xff
    ), header.row_count, header.column_count);
    for (auto channel = 0u;
              channel < output.ChannelCount()-1;
              channel++) {
      for (auto index = 0u;
                index < output.Length();
                index++) {
        output[index][channel] = input.data[output.Length() * channel + index];
      }
    }
    return output;
  }
}; // class DecodeFn

inline constexpr auto Decode = DecodeFn();

}; // namespace PSD
