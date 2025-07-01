
#include <psd/capi/document.h>

namespace PSD::Capi {
//

Document<> *DocumentCast(psd_document *document) {
  return reinterpret_cast<Document<> *>(document);
}
const Document<> *DocumentCast(const psd_document *document) {
  return reinterpret_cast<const Document<> *>(document);
}
psd_document *DocumentCast(Document<> *document) {
  return reinterpret_cast<psd_document *>(document);
}
const psd_document *DocumentCast(const Document<> *document) {
  return reinterpret_cast<const psd_document *>(document);
}

extern "C" {
//

psd_document *psd_document_create(unsigned row_count, unsigned column_count) {
  return DocumentCast(new Document<>(row_count, column_count));
}
psd_document *psd_document_create_empty(void) {
  return DocumentCast(new Document<>());
}
psd_document *psd_document_copy(const psd_document *document) {
  return DocumentCast(new Document<>(*DocumentCast(document)));
}
void psd_document_delete(psd_document *document) {
  delete DocumentCast(document);
}
psd_error psd_document_open(psd_document *document, const char *path) {
  return Detail::HandleError([&](){
    DocumentCast(document)->Open(std::string(path));
  });
}
psd_error psd_document_save(psd_document *document, const char *path) {
  return Detail::HandleError([&](){
    DocumentCast(document)->Save(std::string(path));
  });
}
unsigned psd_document_get_row_count(const psd_document *document) {
  return DocumentCast(document)->GetRCount();
}
unsigned psd_document_get_column_count(const psd_document *document) {
  return DocumentCast(document)->GetCCount();
}

psd_error psd_document_push_layer(psd_document *document, psd_layer *layer) {
  return Detail::HandleError([&](){
    DocumentCast(document)->Push(std::move(*LayerCast(layer)));
    psd_layer_delete(layer);
  });
}
psd_error psd_document_push_group(psd_document *document, psd_group *group) {
  return Detail::HandleError([&](){
    DocumentCast(document)->Push(std::move(*GroupCast(group)));
    psd_group_delete(group);
  });
}
} // extern "C"
} // PSD::Capi
