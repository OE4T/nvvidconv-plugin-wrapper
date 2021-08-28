# nvvidconv-plugin-wrapper
Wrapper plugin for the libv4l2_nvvidconv plugin in L4T.

This wrapper plugins implements a workaround for the libv4l2_nvvidconv
V4L2 plugin, which can crash with a segmentation fault in `pthread_join()`
if is passed a VIDIOC_STREAMOFF ioctl when the stream has already been
turned off.

The wrapper normally passes through all plugin calls to libv4l2_nvvidconv,
but for VIDIOC_STREAMON and VIDIOC_STREAMOFF, it tracks the current
stream on/off state and skips the passthrough when it the requested
streaming state matches the current state.

See [this issue in meta-tegra](https://github.com/OE4T/meta-tegra/issues/785)
for more information.
