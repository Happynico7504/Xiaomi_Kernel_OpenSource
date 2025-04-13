 drm_mode_config_init(&fbkms->drm);

    fbkms->mode.clock = 71000;
    fbkms->mode.hdisplay = 720;
    fbkms->mode.vdisplay = 1600;
    fbkms->mode.vrefresh = 60;
    fbkms->mode.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC;
    fbkms->mode.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    strcpy(fbkms->mode.name, "720x1600");
