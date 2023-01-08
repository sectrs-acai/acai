FVP_ESCAPE_HOST_VERSION = 1.0
FVP_ESCAPE_HOST_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../src/fvp_escape_host
FVP_ESCAPE_HOST_SITE_METHOD = local
FVP_ESCAPE_HOST_INSTALL_STAGING = YES
FVP_ESCAPE_HOST_INSTALL_TARGET = YES
FVP_ESCAPE_HOST_DEPENDENCIES = pteditor scanmem host-pteditor host-scanmem

define FVP_ESCAPE_HOST_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

define FVP_ESCAPE_HOST_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0655 $(@D)/*.out '$(TARGET_DIR)'
endef


$(eval $(generic-package))
$(eval $(host-generic-package))
