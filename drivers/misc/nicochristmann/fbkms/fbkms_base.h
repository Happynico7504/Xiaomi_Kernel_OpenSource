int fbkms_get_modes(struct drm_connector *connector)
{
    struct drm_display_mode *mode;

    pr_info("fbkms_get_modes: called\n");

    if (!connector || !connector->dev) {
        pr_err("fbkms_get_modes: invalid connector or dev\n");
        return 0;
    }

    mode = drm_mode_duplicate(connector->dev, &fbkms_output_config);
    if (!mode) {
        pr_err("fbkms_get_modes: failed to duplicate mode\n");
        return 0;
    }

    drm_mode_set_name(mode);  // falls .name nicht korrekt gesetzt war
    drm_mode_probed_add(connector, mode);

    pr_info("fbkms_get_modes: added mode %s\n", mode->name);

    return 1;
}

static enum drm_connector_status fbkms_detect(struct drm_connector *connector, bool force)
{
    return connector_status_connected;
}

static const struct drm_encoder_funcs fbkms_encoder_funcs = {
    .destroy = drm_encoder_cleanup,
};

const struct drm_connector_funcs fbkms_conn_funcs = {
    .reset = drm_atomic_helper_connector_reset,
    .fill_modes = drm_helper_probe_single_connector_modes,
    .detect = fbkms_detect,
    .destroy = drm_connector_cleanup,
    .atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
    .atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

const struct drm_connector_helper_funcs fbkms_conn_helper_funcs = {
    .get_modes = fbkms_get_modes,
};

static void fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
                              struct drm_crtc_state *crtc_state,
                              struct drm_plane_state *plane_state)
{

    if (!pipe) {
        pr_err("fbkms: pipe is NULL!\n");
        return;
    }
    if (!pipe->crtc.dev) {
        pr_err("fbkms: crtc.dev is NULL!\n");
        return;
    }
    if (!plane_state) {
        pr_err("fbkms: plane_state is NULL!\n");
        return;
    }
    struct drm_device *dev = pipe->crtc.dev;
    struct fbkms_device *fbkms = container_of(dev, struct fbkms_device, drm);
    struct drm_framebuffer *fb;
    struct drm_gem_cma_object *cma_obj;
    void *src;

    fb = plane_state->fb;

    if (!fb || !fb->obj[0]) {
        pr_err("fbkms: framebuffer or object is NULL!\n");
        return;
    }

    cma_obj = to_drm_gem_cma_obj(fb->obj[0]);
    src = cma_obj->vaddr;

    if (!src || !fbkms->fb || !fbkms->fb->screen_base) {
        pr_err("fbkms: invalid src or screen_base!\n");
        return;
    }

    memcpy(fbkms->fb->screen_base, src, fb->height * fb->pitches[0]);
    pr_info("fbkms: pipe enabled\n");
}

static void fbkms_pipe_disable(struct drm_simple_display_pipe *pipe)
{
    pr_info("fbkms pipe disabled\n");
}

static const struct drm_simple_display_pipe_funcs fbkms_pipe_funcs = {
    .enable = fbkms_pipe_enable,
    .disable = fbkms_pipe_disable,
};

static const uint32_t fbkms_formats[] = {
    DRM_FORMAT_RGBA8888,
};

static struct drm_driver fbkms_driver = {
    .driver_features = DRIVER_MODESET | DRIVER_GEM,
    .name = "fbkms",
    .desc = "Framebuffer KMS",
    .date = "20250625",
    .gem_free_object_unlocked = drm_gem_cma_free_object,
    .dumb_create = drm_gem_cma_dumb_create,
    .dumb_destroy = drm_gem_dumb_destroy,
};

static int fbkms_probe(struct platform_device *pdev)
{
    struct fbkms_device *fbkms;
    struct fb_info *info = NULL;
    int ret;

    pr_info("fbkms: probe started\n");

    // Speicher allozieren
    fbkms = devm_kzalloc(&pdev->dev, sizeof(*fbkms), GFP_KERNEL);
    if (!fbkms) {
        dev_err(&pdev->dev, "Failed to allocate device struct\n");
        return -1;
    }
    
    // Ein valides fb Gerät suchen
    for (int i = 0; i < FB_MAX; i++) {
        if (registered_fb[i] && registered_fb[i]->screen_base) {
            info = registered_fb[i];
            break;
        }
    }

    if (!info) {
        dev_err(&pdev->dev, "No valid framebuffer device found\n");
        return -1;
    }
    
    fbkms->fb = info;
    fbkms->drm.dev = &pdev->dev;
    platform_set_drvdata(pdev, fbkms);

    // DRM-Gerät initialisieren
    ret = drm_dev_init(&fbkms->drm, &fbkms_driver, &pdev->dev);
    if (ret) {
        dev_err(&pdev->dev, "drm_dev_init failed\n");
        return -1;
    }
    
    // KMS-Konfig initialisieren
    drm_mode_config_init(&fbkms->drm);
    pr_info("fbkms: drm_mode_config_init done\n");

    // Display Pipe zuerst initialisieren – erzeugt intern encoder!
    ret = drm_simple_display_pipe_init(&fbkms->drm,
        &fbkms->pipe,
        &fbkms_pipe_funcs,
        fbkms_formats, ARRAY_SIZE(fbkms_formats),
        NULL, // connector separat
        NULL);
    if (ret) {
        dev_err(&pdev->dev, "drm_simple_display_pipe_init failed (%d)\n");
        goto err_config;
    }

    fbkms->pipe.crtc.dev = &fbkms->drm;

    // Connector initialisieren
    ret = drm_connector_init(&fbkms->drm,
                             &fbkms->connector,
                             &fbkms_conn_funcs,
                             DRM_MODE_CONNECTOR_Unknown);
    if (ret) {
        dev_err(&pdev->dev, "drm_connector_init failed (%d)\n");
        goto err_pipe;
    }

    drm_connector_helper_add(&fbkms->connector, &fbkms_conn_helper_funcs);

    // Encoder anhängen
    ret = drm_connector_attach_encoder(&fbkms->connector, &fbkms->pipe.encoder);
    if (ret) {
        dev_err(&pdev->dev, "drm_connector_attach_encoder failed (%d)\n", ret);
        goto err_pipe;
    }

    // Zusätzliche Connector-Infos
    fbkms->connector.dpms = DRM_MODE_DPMS_ON;
    fbkms->connector.display_info.width_mm = 68;
    fbkms->connector.display_info.height_mm = 122;
    fbkms->connector.polled = DRM_CONNECTOR_POLL_CONNECT;

    // Gerät registrieren
    ret = drm_dev_register(&fbkms->drm, 0);
    if (ret) {
        dev_err(&pdev->dev, "drm_dev_register failed (%d)\n", ret);
        goto err_pipe;
    }

    // Polling aktivieren (für Hotplug etc.)
    drm_kms_helper_poll_init(&fbkms->drm);

    dev_info(&pdev->dev, "fbkms driver registered successfully\n");
    return 0;

err_pipe:
    drm_mode_config_cleanup(&fbkms->drm);
err_config:
    drm_dev_put(&fbkms->drm);
    return ret;
}
    
static int fbkms_remove(struct platform_device *pdev)
{
    struct fbkms_device *fbkms = platform_get_drvdata(pdev);
    if (!fbkms) {
        dev_warn(&pdev->dev, "fbkms: remove called but no drvdata set\n");
        return -ENODEV;
    }

    pr_info("fbkms: remove called\n");

    // Polling beenden
    drm_kms_helper_poll_fini(&fbkms->drm);
    pr_info("fbkms: polling shutdown done\n");

    // DRM Gerät deregistrieren
    drm_dev_unregister(&fbkms->drm);
    pr_info("fbkms: drm_dev_unregister done\n");

    // KMS-Konfiguration freigeben
    drm_mode_config_cleanup(&fbkms->drm);
    pr_info("fbkms: drm_mode_config_cleanup done\n");

    // Referenzzählung verringern, Gerät wird ggf. freigegeben
    drm_dev_put(&fbkms->drm);
    pr_info("fbkms: drm_dev_put done\n");

    return 0;
}

static struct platform_driver fbkms_platform_driver = {
    .probe = fbkms_probe,
    .remove = fbkms_remove,
    .driver = {
        .name = "fbkms",
    },
};
