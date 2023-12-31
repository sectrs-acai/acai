SCANMEM_VERSION = 1.0
SCANMEM_SITE = $(BR2_EXTERNAL_ARMCCA_PATH)/../../ext/scanmem
SCANMEM_SITE_METHOD = local
SCANMEM_INSTALL_STAGING = YES
SCANMEM_INSTALL_TARGET = YES
SCANMEM_AUTORECONF = YES

SCANMEM_CONF_OPTS = -v --disable-procmem --enable-static --enable-shared

SCANMEM_DEPENDENCIES = readline libtool
HOST_SCANMEM_DEPENDENCIES = host-readline host-libtool host-intltool

define SCANMEM_RUN_AUTOGEN
	cd $(@D) && PATH=$(BR_PATH) ./autogen.sh
endef

SCANMEM_PRE_CONFIGURE_HOOKS += SCANMEM_RUN_AUTOGEN
HOST_SCANMEM_PRE_CONFIGURE_HOOKS += SCANMEM_RUN_AUTOGEN

$(eval $(autotools-package))
$(eval $(host-autotools-package))
