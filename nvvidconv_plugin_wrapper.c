// SPDX-License-Identifier: MIT
// Copyright (c) 2021, OpenEmbedded for Tegra Project

#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <libv4l-plugin.h>

struct wrapper_plugin {
    void *vidconv_library;
    void *vidconv_plugin_ctx;
    struct libv4l_dev_ops *vidconv_dev_ops;
    bool stream_is_on;
};

static void *
plugin_init (int fd)
{
    struct wrapper_plugin *ctx;
    void *handle;
    char buf[PATH_MAX], pathbuf[PATH_MAX], *err;
    ssize_t n;

    handle = dlopen("libv4l2.so.0", RTLD_NOLOAD|RTLD_NOW);
    if (handle == NULL) {
        fprintf(stderr, "dlopen failed for $ORIGIN lookup: %s\n", dlerror());
        return NULL;
    }
    if (dlinfo(handle, RTLD_DI_ORIGIN, buf) < 0) {
        fprintf(stderr, "cannot identify $ORIGIN for locating nvvidconv plugin\n");
        dlclose(handle);
        return NULL;
    }
    dlclose(handle);
    n = snprintf(pathbuf, sizeof(pathbuf)-1, "%s/libv4l/plugins-wrapped/libv4l2_nvvidconv.so", buf);
    if (n < 0) {
        fprintf(stderr, "could not format path name to wrapped plugin\n");
        return NULL;
    }
    pathbuf[n] = '\0';
    ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        perror("allocating plugin context");
        return NULL;
    }
    ctx->vidconv_library = dlopen(pathbuf, RTLD_LAZY);
    if (!ctx->vidconv_library) {
        fprintf(stderr, "dlopen failed for %s: %s\n", pathbuf, dlerror());
        free(ctx);
        return NULL;
    }
    dlerror();
    ctx->vidconv_dev_ops = (struct libv4l_dev_ops *) dlsym(ctx->vidconv_library, "libv4l2_plugin");
    err = dlerror();
    if (err) {
        fprintf(stderr, "error loading nvvidconv plugin: %s\n", err);
        goto error_depart;
    }
    if (ctx->vidconv_dev_ops->init == NULL ||
        ctx->vidconv_dev_ops->close == NULL ||
        ctx->vidconv_dev_ops->ioctl == NULL) {
        fprintf(stderr, "nvvidconv plugin missing ops functions?\n");
        goto error_depart;
    }
    ctx->vidconv_plugin_ctx = ctx->vidconv_dev_ops->init(fd);
    if (ctx->vidconv_plugin_ctx == NULL) {
        fprintf(stderr, "nvvidconv plugin init returned NULL\n");
        goto error_depart;
    }
    return ctx;
error_depart:
    dlclose(ctx->vidconv_library);
    free(ctx);
    return NULL;

} // plugin_init


static void
plugin_close (void *data)
{
    struct wrapper_plugin *ctx = data;

    if (ctx == NULL)
        return;

    if (ctx->vidconv_dev_ops->close != NULL)
        ctx->vidconv_dev_ops->close(ctx->vidconv_plugin_ctx);
    dlclose(ctx->vidconv_library);
    free(ctx);

} // plugin_close


static int
plugin_ioctl (void *data, int fd, unsigned long int cmd, void *arg)
{
    struct wrapper_plugin *ctx = data;
    int ret;

    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (cmd == VIDIOC_STREAMON) {
        if (ctx->stream_is_on)
            return 0;
    } else if (cmd == VIDIOC_STREAMOFF) {
        if (!ctx->stream_is_on)
            return 0;
    }
    ret = ctx->vidconv_dev_ops->ioctl(ctx->vidconv_plugin_ctx, fd, cmd, arg);
    if (ret < 0)
        return ret;
    if (cmd == VIDIOC_STREAMON) {
        ctx->stream_is_on = true;
    } else if (cmd == VIDIOC_STREAMOFF) {
        ctx->stream_is_on = false;
    }
    return ret;

} // plugin_ioctl


const struct libv4l_dev_ops libv4l2_plugin = {
        .init = plugin_init,
        .close = plugin_close,
        .ioctl = plugin_ioctl,
};
