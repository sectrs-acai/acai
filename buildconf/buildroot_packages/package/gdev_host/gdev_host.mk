################################################################################
#
# GDEV_HOST
#
################################################################################

GDEV_HOST_VERSION = 1.0
GDEV_HOST_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../src/gpu/gdev-host
GDEV_HOST_SITE_METHOD = local
GDEV_HOST_INSTALL_STAGING = YES
GDEV_HOST_INSTALL_TARGET = YES
GDEV_HOST_CONF_OPTS = -Ddriver=nouveau -Duser=ON -Druntime=OFF -Dusched=OFF -Duse_as=OFF -DCMAKE_BUILD_TYPE=Release
GDEV_HOST_DEPENDENCIES = libdrm

$(eval $(cmake-package))
$(eval $(host-cmake-package))
