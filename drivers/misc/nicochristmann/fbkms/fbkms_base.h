int fbkms_get_modes(struct drm_connector *connector, struct drm_display_mode *output_mode);
int fbkms_get_modes_wrapper(struct drm_connector *connector);

int fbkms_get_modes(struct drm_connector *connector, struct drm_display_mode *output_mode)
{
    if (!connector) {
    pr_err("fbkms_get_modes: NULL connector!\n");
    return 0;
    }

    if (!connector->dev) {
    pr_err("fbkms_get_modes: NULL connector dev!\n");
    return 0;
    }

    if (!output_mode) {
    pr_err("fbkms_get_modes: NULL mode!\n");
    return 0;
    }

    drm_mode_debug_printmodeline(output_mode);

    pr_info("output_mode name: %s\n", output_mode->name);

    pr_info("adding mode to connector\n");
    drm_mode_probed_add(connector, output_mode);
    pr_info("mode successfully added to connector\n");

    return 1;
}

int fbkms_get_modes_wrapper(struct drm_connector *connector)
{
    pr_info("prepare for mode loading\n");
    return fbkms_get_modes(connector, &fbkms_output_config);
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
    .get_modes = fbkms_get_modes_wrapper,
};

static void fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
                              struct drm_crtc_state *crtc_state,
                              struct drm_plane_state *plane_state)
{
  
    struct fbkms_device *fbkms = container_of(pipe->crtc.dev, struct fbkms_device, drm);

    struct drm_framebuffer *fb = plane_state->fb;

    struct drm_gem_cma_object *cma_obj = to_drm_gem_cma_obj(fb->obj[0]);
    void *src = cma_obj->vaddr;

    memcpy(fbkms->fb->screen_base, src,
           fb->height * fb->pitches[0]);

    pr_info("fbkms pipe enabled\n");
  
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
    int ret;

    pr_info("fbkms: probe started\n");

    fbkms = devm_kzalloc(&pdev->dev, sizeof(*fbkms), GFP_KERNEL);
    if (!fbkms) {
        dev_err(&pdev->dev, "fbkms: failed to allocate device struct\n");
        return -ENOMEM;
    }

    struct fb_info *info = NULL;

    for (int i = 0; i < FB_MAX; i++) {
        if (registered_fb[i] && registered_fb[i]->screen_base) {
            info = registered_fb[i];
            break;
        }
    }

if (!info) {
    dev_err(&pdev->dev, "fbkms: no valid fb device found\n");
    return -ENODEV;
}

fbkms->fb = info;


    platform_set_drvdata(pdev, fbkms);
    pr_info("fbkms: platform data set\n");

    fbkms->drm.dev = &pdev->dev;
    ret = drm_dev_init(&fbkms->drm, &fbkms_driver, &pdev->dev);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_dev_init failed (%d)\n", ret);
        return ret;
    }
    pr_info("fbkms: drm_dev_init successful\n");

    if (!fbkms->drm.dev) {
      pr_err("fbkms: drm.dev is NULL!\n");
      return -EINVAL;
    }

    pr_info("fbkms: drm.dev = %px\n", &fbkms->drm);
    pr_info("fbkms: drm.dev.dev = %px\n", fbkms->drm.dev);

    drm_mode_config_init(&fbkms->drm);
    pr_info("fbkms: drm_mode_config_init done\n");

        ret = drm_connector_init(&fbkms->drm, &fbkms->connector,
                         &fbkms_conn_funcs, DRM_MODE_CONNECTOR_Unknown);
    if (ret) {
        dev_err(&pdev->dev, "failed to init connector (%d)\n", ret);
        goto err_pipe;
    }

    drm_connector_helper_add(&fbkms->connector, &fbkms_conn_helper_funcs);

    ret = drm_connector_attach_encoder(&fbkms->connector, &fbkms->pipe.encoder);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: attach_encoder failed (%d)\n", ret);
        return ret;
    }
    
    fbkms->connector.dpms = DRM_MODE_DPMS_ON;

    struct drm_connector *conn = &fbkms->connector;
    if (!conn) {
        dev_err(&pdev->dev, "fbkms: connector is NULL!\n");
        return -EINVAL;
    }
        conn->display_info.width_mm = 68;
        conn->display_info.height_mm = 122;
        conn->polled = DRM_CONNECTOR_POLL_CONNECT;

    ret = drm_simple_display_pipe_init(&fbkms->drm,
        &fbkms->pipe,
        &fbkms_pipe_funcs,
        fbkms_formats, ARRAY_SIZE(fbkms_formats),
        NULL,
        &fbkms->connector);
    
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_simple_display_pipe_init failed (%d)\n", ret);
        goto err_config;
    }
    pr_info("fbkms: display pipe init done\n");
    
    ret = drm_dev_register(&fbkms->drm, 0);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_dev_register failed (%d)\n", ret);
        goto err_pipe;
    }
    pr_info("fbkms: drm_dev_register success\n");

    drm_kms_helper_poll_init(&fbkms->drm);
    pr_info("fbkms: KMS poll init done\n");

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
    pr_info("fbkms: remove called\n");

    drm_kms_helper_poll_fini(&fbkms->drm);
    pr_info("fbkms: shutting down polling\n");

    drm_dev_unregister(&fbkms->drm);
    pr_info("fbkms: drm_dev_unregister done\n");

    drm_mode_config_cleanup(&fbkms->drm);
    pr_info("fbkms: drm_mode_config_cleanup done\n");

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
