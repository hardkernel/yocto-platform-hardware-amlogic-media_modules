#ifndef __MM_DEC_DEBUG_PORT__
#define __MM_DEC_DEBUG_PORT__


typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long ulong;
typedef unsigned long long u64;

#define LPRINT0
#define LPRINT1(...)        printf(__VA_ARGS__)

#define ERRP(con, rt, p, ...) do {    \
    if (con) {                        \
        LPRINT##p(__VA_ARGS__);       \
        rt;                           \
    }                                 \
} while(0)


#define PR_DBG(pr_level, ...) do {             \
    if ((pr_level & _in_debug) || (!pr_level)) \
        printf(__VA_ARGS__);                   \
} while(0)


enum vformat_e {
	VFORMAT_MPEG12 = 0,
	VFORMAT_MPEG4,
	VFORMAT_H264,
	VFORMAT_MJPEG,
	VFORMAT_REAL,
	VFORMAT_JPEG,
	VFORMAT_VC1,
	VFORMAT_AVS,
	VFORMAT_YUV,		/* Use SW decoder */
	VFORMAT_H264MVC,
	VFORMAT_H264_4K2K,
	VFORMAT_HEVC,
	VFORMAT_H264_ENC,
	VFORMAT_JPEG_ENC,
	VFORMAT_VP9,
	VFORMAT_AVS2,
	VFORMAT_AV1,
	VFORMAT_AVS3,
	VFORMAT_MAX
};

//compat with version v0.x
struct port_data_packet_v0 {
	int header;
	int id;
	int type;
	u32 data_size;
	u32 crc;
	u32 private_data_size;
	char private[0];   //private to PADING_SIZE end
};


/* version v1.0 it must be same with drivers defined */
struct vdec_dbg_ex_usr {
	int header;
	int id;       //the packet count
	int type;     //0: local buf, 1: not local buf but virt, 2: phys buf direct
	u32 data_size;
	u32 crc;
	u32 offset;
	u32 addr_h;
	u32 addr_l;
	char file[64];     /* store to file name */
	u32 reserved[32];
};

/* type for read data inside */
#define DEV_READ_LOCAL  0
#define DEV_READ_VIRT   1
#define DEV_READ_PHYS   2
#define DEV_READ_MMAP   3


#define _VDP_  'P'
#define VDEC_EXPORT_PACKET           _IOR((_VDP_), 0x02, struct vdec_dbg_ex_usr)
#define VDEC_EXPORT_CONFIG           _IOW((_VDP_), 0x03, int)
#define VDEC_EXPORT_NOTIFY           _IOW((_VDP_), 0x04, int)
#define VDEC_EXPORT_LOCAL_BUF_RST    _IOW((_VDP_), 0x0a, int)   //to compat with version 0
#define VDEC_EXPORT_VERSION          _IOR((_VDP_), 0x0f, int)

#define PACKET_HEADER  (0xaa55aa55)
#define PADING_SIZE 1024

enum data_type{
	TYPE_INFO,
	TYPE_YUV,
	TYPE_CRC,
	TYPE_ES,
	TYPE_AUX,
	TYPE_SIZE,
	TYPE_MAX
};

struct port_vdec_info {
	int id;
	int format;
	int double_write;
	int stream_w;
	int stream_h;
	int dw_w;
	int dw_h;
	int plane_num;
	int bitdepth;
	int is_interlace;
	u32 reserved[32];
};

#endif /* __MM_DEC_DEBUG_PORT__ */

