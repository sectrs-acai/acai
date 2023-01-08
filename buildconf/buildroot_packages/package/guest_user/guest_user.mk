################################################################################
#
# build_user
#
################################################################################

GUEST_USER_VERSION = 1.0
GUEST_USER_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../src/fvp_escape_guest_user
GUEST_USER_SITE_METHOD = local
GUEST_USER_INSTALL_STAGING = YES

define GUEST_USER_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

define GUEST_USER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0655 $(@D)/*.out '$(TARGET_DIR)'
endef

$(eval $(generic-package))
