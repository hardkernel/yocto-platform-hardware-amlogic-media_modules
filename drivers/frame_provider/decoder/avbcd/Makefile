MODULE_NAME = amvdec_avbcd
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AVBCD) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += avbcd.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
