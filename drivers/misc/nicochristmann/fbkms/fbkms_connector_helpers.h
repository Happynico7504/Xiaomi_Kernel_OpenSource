#ifndef FBKMS_CONNECTOR_HELPERS_H
#define FBKMS_CONNECTOR_HELPERS_H

#include <drm/drm_connector.h>

int fbkms_get_modes(struct drm_connector *connector, struct drm_display_mode *preferred_mode);
int fbkms_get_modes_wrapper(struct drm_connector *connector);

extern const struct drm_connector_funcs fbkms_conn_funcs;
extern const struct drm_connector_helper_funcs fbkms_conn_helper_funcs;

#endif // FBKMS_CONNECTOR_HELPERS_H
