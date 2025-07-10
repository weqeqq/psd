
#include "psd/document.h"
#include <psd/capi/document.h>

namespace PSD::capi {
//

Document *DocumentCast(psd_document *document) {
  return reinterpret_cast<Document *>(document);
}
const Document *DocumentCast(const psd_document *document) {
  return reinterpret_cast<const Document *>(document);
}
psd_document *DocumentCast(Document *document) {
  return reinterpret_cast<psd_document *>(document);
}
const psd_document *DocumentCast(const Document *document) {
  return reinterpret_cast<const psd_document *>(document);
}

extern "C" {
//

psd_document *psd_document_new(void) {
  return DocumentCast(new Document());
}
psd_document *psd_document_clone(const psd_document *document) {
  return DocumentCast(new Document(*DocumentCast(document)));
}
void psd_document_delete(psd_document *document) {
  delete DocumentCast(document);
}

psd_error psd_open(psd_document **document, const char *path) {
  return detail::HandleError([&](){
    *document = DocumentCast(new Document(PSD::Open(std::filesystem::path(path))));
  });
}
psd_error psd_save(psd_document *document, const char *path) {
  return detail::HandleError([&](){
    PSD::Save(*DocumentCast(document), std::filesystem::path(path));
  });
}
psd_error psd_export(psd_document *document, unsigned char **output) {
  return detail::HandleError([&](){
    auto image = PSD::Export(*DocumentCast(document));
    *output = static_cast<unsigned char *>(malloc(image.Count()));
    std::copy(image.begin(), image.end(), *output);
  });
}
unsigned psd_document_get_row_count(const psd_document *document) {
  return DocumentCast(document)->RowCount();
}
unsigned psd_document_get_column_count(const psd_document *document) {
  return DocumentCast(document)->ColumnCount();
}

psd_error psd_document_push_layer(psd_document *document, psd_layer *layer) {
  return detail::HandleError([&](){
    DocumentCast(document)->Push(std::move(*LayerCast(layer)));
    psd_layer_delete(layer);
  });
}
psd_error psd_document_push_group(psd_document *document, psd_group *group) {
  return detail::HandleError([&](){
    DocumentCast(document)->Push(std::move(*GroupCast(group)));
    psd_group_delete(group);
  });
}
} // extern "C"
} // PSD::capi
