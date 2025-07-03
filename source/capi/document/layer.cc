
#include <psd/capi/document/layer.h>

namespace PSD::Capi {
//
Layer<> *LayerCast(psd_layer *layer) {
  return reinterpret_cast<Layer<> *>(layer);
}
const Layer<> *LayerCast(const psd_layer *layer) {
  return reinterpret_cast<const Layer<> *>(layer);
}
psd_layer *LayerCast(Layer<> *layer) {
  return reinterpret_cast<psd_layer *>(layer);
}
const psd_layer *LayerCast(const Layer<> *layer) {
  return reinterpret_cast<const psd_layer *>(layer);
}
namespace {
//
Image::AlphaBuffer<> ConstructImage(
  const unsigned char *buffer, unsigned row_count, unsigned column_count
) {
  Image::AlphaBuffer<> output(row_count, column_count);
  for (auto index = 0u; index < row_count * column_count; ++index) {
    output[index][0] = buffer[index * 4 + 0];
    output[index][1] = buffer[index * 4 + 1];
    output[index][2] = buffer[index * 4 + 2];
    output[index][3] = buffer[index * 4 + 3];
  }
  return output;
}
} // namespace
extern "C" {
//
psd_layer *psd_layer_new(const char *name) {
  return LayerCast(new Layer<>(std::string(name)));
}
void psd_layer_delete(psd_layer *layer) {
  delete LayerCast(layer);
}
psd_layer *psd_layer_copy(const psd_layer *layer) {
  return LayerCast(new Layer<>(*LayerCast(layer)));
}

psd_error psd_layer_set_image(
  psd_layer *layer, const unsigned char *buffer, unsigned row_count, unsigned column_count
) {
  return Detail::HandleError([&](){
    LayerCast(layer)->SetImage(ConstructImage(
      buffer,
      row_count,
      column_count
    ));
  });
}
unsigned char *psd_layer_get_image(const psd_layer *layer) {
  try {
    auto &image = LayerCast(layer)->GetBuffer();
    auto *buffer = static_cast<unsigned char *>(malloc(image.GetLength() * 4));

    for (auto index = 0u; index < image.GetLength(); ++index) {
      buffer[index * 4 + 0] = image[index][0];
      buffer[index * 4 + 1] = image[index][1];
      buffer[index * 4 + 2] = image[index][2];
      buffer[index * 4 + 3] = image[index][3];
    }
    return buffer;
  } catch(...) {
    return nullptr;
  }
}

unsigned psd_layer_get_row_count(const psd_layer *layer) {
  return LayerCast(layer)->GetRowCount();
}
unsigned psd_layer_get_column_count(const psd_layer *layer) {
  return LayerCast(layer)->GetColumnCount();
}
void psd_layer_set_offset(psd_layer *layer, unsigned x_offset, unsigned y_offset) {
  LayerCast(layer)->SetOffset(x_offset, y_offset);
}
unsigned psd_layer_get_top(const psd_layer *layer) {
  return LayerCast(layer)->GetTop();
}
unsigned psd_layer_get_left(const psd_layer *layer) {
  return LayerCast(layer)->GetLeft();
}
unsigned psd_layer_get_bottom(const psd_layer *layer) {
  return LayerCast(layer)->GetBottom();
}
unsigned psd_layer_get_right(const psd_layer *layer) {
  return LayerCast(layer)->GetRight();
}
const char *psd_layer_get_name(const psd_layer *layer) {
  return LayerCast(layer)->GetName().c_str();
}
void psd_layer_set_name(psd_layer *layer, const char *name) {
  LayerCast(layer)->SetName(std::string(name));
}
}
}
