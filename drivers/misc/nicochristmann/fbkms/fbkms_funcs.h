
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
