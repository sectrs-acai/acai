GUEST_KERNEL_MODULE_VERSION = 1.0
GUEST_KERNEL_MODULE_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../src/fvp_escape_experiments/fvp_escape_guest_kernel
GUEST_KERNEL_MODULE_SITE_METHOD = local
GUEST_KERNEL_MODULE_INSTALL_STAGING = YES

define GUEST_KERNEL_MODULE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0655 $(@D)/*.ko '$(TARGET_DIR)'
endef

$(eval $(kernel-module))
$(eval $(generic-package))
