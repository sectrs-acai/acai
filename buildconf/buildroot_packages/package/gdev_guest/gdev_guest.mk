################################################################################
#
# GDEV_GUEST
# builds fh enabled gdev frontend for aarch64
#
################################################################################
GDEV_GUEST_VERSION = 1.0
GDEV_GUEST_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../src/gpu/gdev-guest
GDEV_GUEST_SITE_METHOD = local
GDEV_GUEST_INSTALL_STAGING = YES
GDEV_GUEST_INSTALL_TARGET = YES
GDEV_GUEST_DEPENDENCIES = libdrm

define GDEV_GUEST_BUILD_CMDS
	# gdev
	$(MAKE) $(TARGET_CONFIGURE_OPTS)  -C $(@D)/lib/kernel all

	# cuda
	rm -rf $(@D)/cuda/build-cuda
	mkdir -p $(@D)/cuda/build-cuda
	cd $(@D)/cuda/build-cuda && ../configure --disable-runtime
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/cuda/build-cuda
	cd $(@D)/cuda/build-cuda && ln -s --relative libucuda.so.1.0.0 libucuda.so || true
	cd $(@D)/cuda/build-cuda && ln -s --relative libucuda.so.1.0.0 libucuda.so.1 || true
endef

define GDEV_GUEST_INSTALL_TARGET_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) GDEVDIR="$(TARGET_DIR)/usr/local/gdev" -C $(@D)/lib/kernel install
	$(INSTALL) -D -m 0655 $(@D)/cuda/build-cuda/libucu* "$(TARGET_DIR)/usr/local/gdev/lib64"
endef

define GDEV_GUEST_INSTALL_STAGING_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) GDEVDIR="$(STAGING_DIR)/usr/local/gdev" -C $(@D)/lib/kernel install
	$(INSTALL) -D -m 0655 $(@D)/cuda/build-cuda/libucu* "$(STAGING_DIR)/usr/local/gdev/lib64/"
endef


$(eval $(generic-package))
$(eval $(host-generic-package))
