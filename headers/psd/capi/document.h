
#pragma once

#include <psd/capi/document/layer.h>
#include <psd/capi/document/group.h>
#include <psd/capi/error.h>

#ifdef __cplusplus
#include <psd/document.h>

namespace PSD::capi {
extern "C" {

#endif // __cplusplus

typedef struct psd_document psd_document;

psd_document *psd_document_new(void);
psd_document *psd_document_copy(const psd_document *document);

void psd_document_delete(psd_document *document);

psd_error psd_open(psd_document **document, const char *path);
psd_error psd_save(psd_document *document, const char *path);
psd_error psd_export(psd_document *document, unsigned char **output, unsigned *row_count, unsigned *column_count);
psd_error psd_decode(const char *path, unsigned char **output, unsigned *row_count, unsigned *column_count);

unsigned psd_document_get_row_count(const psd_document *document);
unsigned psd_document_get_column_count(const psd_document *document);

psd_error psd_document_push_layer(psd_document *document, psd_layer *layer);
psd_error psd_document_push_group(psd_document *document, psd_group *layer);

#ifdef __cplusplus
}
Document *DocumentCast(psd_document *document);
const Document *DocumentCast(const psd_document *document);
psd_document *DocumentCast(Document *document);
const psd_document *DocumentCast(const Document *document);
}
#endif // __cplusplus
