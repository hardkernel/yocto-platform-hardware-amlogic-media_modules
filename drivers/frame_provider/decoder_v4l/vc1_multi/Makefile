MODULE_NAME = amvdec_mvc1_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_VC1_MULTI) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vvc1_multi.o

PR_FMT = "vc1"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
