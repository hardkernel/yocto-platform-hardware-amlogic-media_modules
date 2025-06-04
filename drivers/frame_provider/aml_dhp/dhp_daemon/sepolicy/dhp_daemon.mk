# dhp_daemon
PRODUCT_PACKAGES += \
    dhp_daemon_sepolicy

# new policy for selinux
ifeq ($(TARGET_SELINUX_LEGACY), false)

BOARD_SEPOLICY_DIRS += common/common*/driver_modules/media_modules/drivers/frame_provider/aml_dhp/dhp_daemon/sepolicy

endif
