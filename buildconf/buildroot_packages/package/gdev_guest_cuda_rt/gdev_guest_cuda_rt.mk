################################################################################
#
# GDEV_GUEST_CUDA_RT
# builds cuda with runtime api library
#
################################################################################
GDEV_GUEST_CUDA_RT_VERSION = 1.0
GDEV_GUEST_CUDA_RT_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../src/gpu_driver/gdev-guest
GDEV_GUEST_CUDA_RT_SITE_METHOD = local
GDEV_GUEST_CUDA_RT_INSTALL_STAGING = YES
GDEV_GUEST_CUDA_RT_INSTALL_TARGET = YES
GDEV_GUEST_CUDA_RT_DEPENDENCIES = libdrm boost gdev_guest host-bison host-flex

GDEV_GUEST_CUDA_RT_CONF_OPTS = -Ddriver=nouveau -Duser=ON -Druntime=ON -Dusched=OFF -Duse_as=OFF \
     -DCMAKE_BUILD_TYPE=Release -Dgdev_dir=$(STAGING_DIR)/usr/local/gdev/lib64 \
     -DCMAKE_INSTALL_PREFIX=/usr/local -Dinstall_name="ucuda_rt"  -DSTAGIND_DIR=$(STAGING_DIR) -Dbuild_cuda_only=ON


$(eval $(cmake-package))
$(eval $(host-cmake-package))

