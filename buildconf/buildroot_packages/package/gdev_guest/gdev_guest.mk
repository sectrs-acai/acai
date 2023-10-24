################################################################################
#
# GDEV_GUEST
# build libgdev.so with kernel ioctl mode for fvp
#
################################################################################
GDEV_GUEST_VERSION = 1.0
GDEV_GUEST_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../src/gpu_driver/gdev-guest
GDEV_GUEST_SITE_METHOD = local
GDEV_GUEST_INSTALL_STAGING = YES
GDEV_GUEST_INSTALL_TARGET = YES
GDEV_GUEST_DEPENDENCIES = libdrm boost

define GDEV_GUEST_BUILD_CMDS
	# gdev
	$(MAKE) $(TARGET_CONFIGURE_OPTS)  -C $(@D)/lib/kernel clean
	$(MAKE) $(TARGET_CONFIGURE_OPTS)  -C $(@D)/lib/kernel all
endef

define GDEV_GUEST_INSTALL_TARGET_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) GDEVDIR="$(TARGET_DIR)/usr/local/gdev" -C $(@D)/lib/kernel install
endef

define GDEV_GUEST_INSTALL_STAGING_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) GDEVDIR="$(STAGING_DIR)/usr/local/gdev" -C $(@D)/lib/kernel install
endef


$(eval $(generic-package))
$(eval $(host-generic-package))

