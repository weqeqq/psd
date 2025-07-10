
#include <psd/capi/document/group.h>

namespace PSD::capi {
//
Group *GroupCast(psd_group *group) {
  return reinterpret_cast<Group *>(group);
}
const Group *GroupCast(const psd_group *group) {
  return reinterpret_cast<const Group *>(group);
}
psd_group *GroupCast(Group *group) {
  return reinterpret_cast<psd_group *>(group);
}
const psd_group *GroupCast(const Group *group) {
  return reinterpret_cast<const psd_group *>(group);
}
extern "C" {
//
psd_group *psd_group_new(const char *name) {
  try {
    return GroupCast(new Group(std::string(name)));
  } catch(...) {
    return nullptr;
  }
}
void psd_group_delete(psd_group *group) {
  delete GroupCast(group);
}
psd_group *psd_group_clone(const psd_group *group) {
  return GroupCast(new Group(*GroupCast(group)));
}
psd_error psd_group_push_group(psd_group *group, psd_group *other) {
  return detail::HandleError([&](){
    GroupCast(group)->Push(std::move(*GroupCast(other)));
    psd_group_delete(other);
  });
}
psd_error psd_group_push_layer(psd_group *group, psd_layer *other) {
  return detail::HandleError([&](){
    GroupCast(group)->Push(std::move(*LayerCast(other)));
    psd_layer_delete(other);
  });
}
const char *psd_group_get_name(const psd_group *group) {
  return GroupCast(group)->Name().c_str();
}
void psd_group_set_name(psd_group *group, const char *name) {
  GroupCast(group)->SetName(std::string(name));
}
int psd_group_empty(const psd_group *group) {
  return GroupCast(group)->Empty();
}
}
}
