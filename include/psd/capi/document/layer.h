
#pragma once

#include <psd/capi/error.h>

#ifdef __cplusplus
#include <psd/document/layer.h>

namespace PSD::Capi {

extern "C" {
#endif // __cplusplus

typedef struct psd_layer psd_layer;

psd_layer *psd_layer_new(const char *name);
void psd_layer_delete(psd_layer *layer);

psd_layer *psd_layer_copy(const psd_layer *layer);

psd_error psd_layer_set_image(psd_layer *layer, const unsigned char *buffer, unsigned row_count, unsigned column_count);
unsigned char *psd_layer_get_image(const psd_layer *layer);

unsigned psd_layer_get_row_count(const psd_layer *layer);
unsigned psd_layer_get_column_count(const psd_layer *layer);

void psd_layer_set_offset(psd_layer *layer, unsigned x_offset, unsigned y_offset);
unsigned psd_layer_get_top(const psd_layer *layer);
unsigned psd_layer_get_left(const psd_layer *layer);
unsigned psd_layer_get_bottom(const psd_layer *layer);
unsigned psd_layer_get_right(const psd_layer *layer);

const char *psd_layer_get_name(const psd_layer *layer);
void psd_layer_set_name(psd_layer *layer, const char *name);

#ifdef __cplusplus
}
Layer<> *LayerCast(psd_layer *layer);

const Layer<> *LayerCast(const psd_layer *layer);

psd_layer *LayerCast(Layer<> *layer);

const psd_layer *LayerCast(const Layer<> *layer);
}
#endif // __cplusplus
