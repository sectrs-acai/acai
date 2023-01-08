#
# TODO: make patch or fork ptedit
#
PTEDITOR_VERSION = 1.0
PTEDITOR_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../ext/pteditor
PTEDITOR_MODULE_SUBDIRS = module
PTEDITOR_SITE_METHOD = local
PTEDITOR_INSTALL_STAGING = YES
PTEDITOR_INSTALL_TARGET = YES

THIS_DIR=$(BR2_EXTERNAL_ARMCCA_PATH)/package/pteditor

define PTEDITOR_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/pteditor

    $(INSTALL) -D -m 0644 $(@D)/ptedit_header.h $(STAGING_DIR)/usr/include/pteditor/ptedit_header.h
	$(INSTALL) -D -m 0644 $(@D)/ptedit.h $(STAGING_DIR)/usr/include/pteditor/ptedit.h
	$(INSTALL) -D -m 0655 $(@D)/libptedit.so* $(STAGING_DIR)/usr/lib
endef

define PTEDITOR_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0655 $(@D)/module/*.ko '$(TARGET_DIR)'
    $(INSTALL) -D -m 0655 $(@D)/libptedit.so* $(TARGET_DIR)/usr/lib
endef

define PTEDITOR_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) header lib.ptedit

	# $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(THIS_DIR) all
endef


define PTEDITOR_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0655 $(@D)/*.out '$(TARGET_DIR)' || true
	$(INSTALL) -D -m 0655 $(@D)/*.o '$(TARGET_DIR)' || true
endef


$(eval $(kernel-module))
$(eval $(generic-package))
$(eval $(host-generic-package))
