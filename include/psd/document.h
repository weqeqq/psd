
#pragma once

#include <psd/document/impl/convertor/root.h>
#include <psd/document/impl/processor/group.h>
#include <image/encoder.h>
#include <psd/structure.h>

namespace PSD {

template <Depth::Tp DepthV = Depth::Eight,
          Color::Tp ColorV = Color::RGB>
class Document {
public:
  static constexpr std::uint64_t Auto = std::numeric_limits<std::uint64_t>::max();

  explicit Document() = default;
  explicit Document(std::uint64_t row_count, std::uint64_t column_count)    
    : row_count_    (row_count)
    , column_count_ (column_count) {}

  bool operator==(const Document &other) const {
    return row_count_    == other.row_count_    && 
           column_count_ == other.column_count_ && 
           root_         == other.root_;
  }
  bool operator!=(const Document &other) const {
    return !operator==(other);
  }

  Element::Pointer operator[](std::uint64_t index) {
    return root_[index];
  }
  Element::Pointer operator[](std::uint64_t index) const {
    return root_[index];
  }

  using Iterator      = typename DocumentImpl::Root<DepthV, ColorV>::Iterator;
  using ConstIterator = typename DocumentImpl::Root<DepthV, ColorV>::ConstIterator;

  using RevIterator      = typename DocumentImpl::Root<DepthV, ColorV>::RevIterator;
  using ConstRevIterator = typename DocumentImpl::Root<DepthV, ColorV>::ConstRevIterator;

  auto Begin() {
    return root_.Begin();
  }
  auto Begin() const {
    return root_.Begin();
  }
  auto ConstBegin() const {
    return root_.ConstBegin();
  }
  auto RevBegin() {
    return root_.RevBegin();
  }
  auto RevBegin() const {
    return root_.RevBegin();
  }
  auto ConstRevBegin() const {
    return root_.ConstRevBegin();
  }
  auto End() {
    return root_.End();
  }
  auto End() const {
    return root_.End();
  }
  auto ConstEnd() const {
    return root_.ConstEnd();
  }
  auto RevEnd() {
    return root_.RevEnd();
  }
  auto RevEnd() const {
    return root_.RevEnd();
  }
  auto ConstRevEnd() const {
    return root_.ConstRevEnd();
  }

  std::uint64_t GetElementCount() const {
    return root_.GetElementCount();
  }
  std::uint64_t GetLayerCount() const {
    return root_.GetLayerCount();
  }
  std::uint64_t GetGroupCount() const {
    return root_.GetGroupCount();
  }

  template <typename ElementT>
  void Push(ElementT &&element) {
    root_.Push(std::forward<ElementT>(element));
  }

  bool HasGroups() const {
    return root_.HasGroups();
  }
  bool HasLayers() const {
    return root_.HasLayers();
  }

  decltype(auto) FrontElement() {
    return root_.FrontElement();
  } 
  decltype(auto) FrontElement() const {
    return root_.FrontElement();
  }
  decltype(auto) BackElement() {
    return root_.BackElement();
  }
  decltype(auto) BackElement() const {
    return root_.BackElement();
  }
  decltype(auto) FrontLayer() {
    return root_.FrontLayer();
  }
  decltype(auto) FrontLayer() const {
    return root_.FrontLayer();
  }
  decltype(auto) BackLayer() {
    return root_.BackLayer();
  }
  decltype(auto) BackLayer() const {
    return root_.BackLayer();
  }
  decltype(auto) FrontGroup() {
    return root_.FrontGroup();
  }
  decltype(auto) FrontGroup() const {
    return root_.FrontGroup();
  }
  decltype(auto) BackGroup() {
    return root_.BackGroup();
  }
  decltype(auto) BackGroup() const {
    return root_.BackGroup();
  }

  std::uint64_t GetRCount() const {
    return row_count_ == Auto 
      ? root_.FindRCount()
      : row_count_;
  }
  std::uint64_t GetCCount() const {
    return column_count_ == Auto 
      ? root_.FindCCount()
      : column_count_;
  }

  void Open(const std::string &path) {
    Input input(path);

    input.Decode();

    root_         = input.Convert();
    row_count_    = input.GetRCount();
    column_count_ = input.GetCCount();
  }
  void Save(const std::string &path) const {
    Output output(path);

    output.Convert   (root_);
    output.SetRCount (GetRCount());
    output.SetCCount (GetCCount());

    output.Encode();
  }
  void Export(const std::string &path) const {
    auto [buffer, alpha] = DocumentImpl::Processor(root_).Process();
    Image::Encoder(buffer, alpha).Write(path);
  }

private:
  class Input {
  public:
    explicit Input(const std::string &path) : path_(path) {}

    void Decode() {
      structure_ = Decompressor(
        Stream(path_).Read<CompressedStructure<DepthV, ColorV>>()
      ).Decompress();
    }
    auto Convert() const {
      return DocumentImpl::RootConvertor(
        structure_.main_info.layer_info
      ).Convert();
    }
    auto GetRCount() const {
      return structure_.header.row_count;
    }
    auto GetCCount() const {
      return structure_.header.column_count;
    }

  private:
    const std::string &path_;
    Structure<DepthV, ColorV> structure_;

  };
  class Output {
  public:
    explicit Output(const std::string &path) : path_(path) {}

    void Encode() {
      Stream stream;

      structure_.header.version       = Version::PSD;
      structure_.header.channel_count = Color::ChannelCount<ColorV>;
      structure_.header.depth         = DepthV;
      structure_.header.color         = ColorV;

      stream.Write(Compressor(structure_).Compress());
      stream.To(path_);
    }
    void Convert(const DocumentImpl::Root<DepthV, ColorV> &root) {
      structure_.main_info.layer_info = DocumentImpl::RootConvertor(root).Convert();
      structure_.final_image.buffer   = DocumentImpl::Processor(root).Process().buffer;
    }
    void SetRCount(std::uint64_t row_count) {
      structure_.header.row_count = row_count;
    }
    void SetCCount(std::uint64_t column_count) {
      structure_.header.column_count = column_count;
    }

  private:
    const std::string &path_;
    Structure<DepthV, ColorV> structure_;
  };

  DocumentImpl::Root<DepthV, ColorV> root_;

  std::uint64_t row_count_    = Auto;
  std::uint64_t column_count_ = Auto;
};
}; 
