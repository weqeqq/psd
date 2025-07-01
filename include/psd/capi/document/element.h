
#pragma once

#include <psd/capi/error.h>

#ifdef __cplusplus
namespace PSD::Capi {
extern "C" {

#endif // __cplusplus

typedef struct psd_element psd_element;

// todo

bool psd_element_is_group(psd_element *element);
bool psd_element_is_layer(psd_element *element);

#ifdef __cplusplus
}
}
#endif // __cplusplus
