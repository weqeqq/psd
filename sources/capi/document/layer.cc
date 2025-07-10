
#include <psd/capi/document/layer.h>

namespace PSD::capi {
//
Layer *LayerCast(psd_layer *layer) {
  return reinterpret_cast<Layer *>(layer);
}
const Layer *LayerCast(const psd_layer *layer) {
  return reinterpret_cast<const Layer *>(layer);
}
psd_layer *LayerCast(Layer *layer) {
  return reinterpret_cast<psd_layer *>(layer);
}
const psd_layer *LayerCast(const Layer *layer) {
  return reinterpret_cast<const psd_layer *>(layer);
}
extern "C" {
//
psd_layer *psd_layer_new(const char *name) {
  return LayerCast(new Layer(std::string(name)));
}
void psd_layer_delete(psd_layer *layer) {
  delete LayerCast(layer);
}
psd_layer *psd_layer_clone(const psd_layer *layer) {
  return LayerCast(new Layer(*LayerCast(layer)));
}

psd_error psd_layer_set_image(
  psd_layer *layer, const unsigned char *buffer, unsigned row_count, unsigned column_count
) {
  return detail::HandleError([&](){
    LayerCast(layer)->SetImage(
      ::Image::Buffer(std::vector<llapi::U8>(
        buffer,
        buffer + row_count * column_count * 4), row_count, column_count)
    );
  });
}
const unsigned char *psd_layer_get_image(const psd_layer *layer) {
  return LayerCast(layer)->Image().Data();
}

unsigned psd_layer_get_row_count(const psd_layer *layer) {
  return LayerCast(layer)->Image().RowCount();
}
unsigned psd_layer_get_column_count(const psd_layer *layer) {
  return LayerCast(layer)->Image().ColumnCount();
}
void psd_layer_set_offset(psd_layer *layer, unsigned x_offset, unsigned y_offset) {
  LayerCast(layer)->SetOffset(x_offset, y_offset);
}
unsigned psd_layer_get_top(const psd_layer *layer) {
  return LayerCast(layer)->Top();
}
unsigned psd_layer_get_left(const psd_layer *layer) {
  return LayerCast(layer)->Left();
}
unsigned psd_layer_get_bottom(const psd_layer *layer) {
  return LayerCast(layer)->Bottom();
}
unsigned psd_layer_get_right(const psd_layer *layer) {
  return LayerCast(layer)->Right();
}
const char *psd_layer_get_name(const psd_layer *layer) {
  return LayerCast(layer)->Name().c_str();
}
void psd_layer_set_name(psd_layer *layer, const char *name) {
  LayerCast(layer)->SetName(std::string(name));
}
}
}
