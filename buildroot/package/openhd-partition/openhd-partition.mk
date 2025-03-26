OPENHD_PARTITION_SITE = $(TOPDIR)/package/openhd-partition
OPENHD_PARTITION_VERSION = 1.0
OPENHD_PARTITION_INSTALL_TARGET = YES

define OPENHD_PARTITION_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/overlay/etc/init.d/S99openhd_partition $(TARGET_DIR)/etc/init.d/S99openhd_partition
endef

$(eval $(generic-package))
