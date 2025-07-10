#pragma once

#include <psd/capi/document/layer.h>
#include <psd/capi/error.h>

#ifdef __cplusplus
#include <psd/document/group.h>

namespace PSD::capi {

extern "C" {
#endif // __cplusplus

typedef struct psd_group psd_group;

psd_group *psd_group_new(const char *name);
void psd_group_delete(psd_group *group);
psd_group *psd_group_clone(const psd_group *group);

psd_error psd_group_push_layer(psd_group *group, psd_layer *other);
psd_error psd_group_push_group(psd_group *group, psd_group *other);

const char *psd_group_get_name(const psd_group *group);
void psd_group_set_name(psd_group *group, const char *name);

int psd_group_empty(const psd_group *group);

#ifdef __cplusplus
}
Group *GroupCast(psd_group *group);
const Group *GroupCast(const psd_group *group);

psd_group *GroupCast(Group *group);
const psd_group *GroupCast(const Group *group);
}
#endif
