#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <getopt.h>

#include <sys/mman.h>
#include <poll.h>
#include <sys/ioctl.h>

#include <pthread.h>

#include "vdec_debug_port.h"

/* max stream buffer size */
#define MALLOC_BUF_SIZE (1024 * 1024 * 16)

#define VDBG_POLL_TIMEOUT 1000

#define MAX_INSTANCE_NUM 9

int dev_status;
int _in_debug;

static char file_path[128] = {0};

int code_version[2];

static const char short_opt[] = "c:p:d:h";

static const struct option long_opt[] = {
	{ "config", required_argument, NULL, 'c' },
	{ "path",   required_argument, NULL, 'p' },
	{ "debug",   no_argument,      NULL, 'd' },
	{ "help",   no_argument,       NULL, 'h' },
	{ 0, 0, 0, 0 }
};


static void signal_handler(int signum)
{
	dev_status = -1;
	printf("exit signal %d\n", signum);
}

int debug_port_save_file_new(char *filename, char *buf, u32 size)
{
	int fd;
	int ret = 0;
	int retry = 0;

	if ((buf == NULL) || (size == 0))
		return 0;

	fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (fd < 0) {
		printf("%s, create %s failed\n", __func__, filename);
		return -1;
	}

	do {
		ret += write(fd, buf + ret, size);
		if (++retry > ((size << 4) + 1))
			break;

	} while (ret < size);

	close(fd);

	if (ret < size) {
		printf("%s, write size less than required, %d < %d\n", __func__, ret, size);
	}
#ifdef DBG_PORT_DEBUG
	else if (ret == size) {
		printf("save to file %s success, write size 0x%x\n", filename, ret);
	}
#endif
	return 0;
}

static u32 debug_port_write_append_sync(char *file, char *buf, u32 size)
{
	int fd, ret;

	fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0644);
	if (fd < 0) {
		printf("open %s failed\n", file);
		return 0;
	}

	ret = write(fd, buf, size);
	if (ret != size)
		printf("%s, write size %d less than %d\n", __func__, ret, size);
	close(fd);

	return ret;
}


static int debug_port_save_file_append(int fd, char *buf, u32 size)
{
	int ret = 0;
	int retry = 0;

	lseek(fd, 0, SEEK_END);

	do {
		ret += write(fd, buf + ret, size);
		if (++retry > ((size << 4) + 1))
			break;

	} while (ret < size);

	if (ret < size) {
		printf("%s, write size less than required, %d < %d\n", __func__, ret, size);
	}

	return 0;
}

int vdec_dbg_get_data_v0(int dev)
{
	int i, j;
	int dump_fd[MAX_INSTANCE_NUM][TYPE_MAX];
	int sum_pkt[MAX_INSTANCE_NUM][TYPE_MAX];

	char file_str[32] = "name-0-0";
	char last_str[32] = {0};
	char file_name[256] = {0};
	const char *file_ext[TYPE_MAX] = {"info", "yuv", "crc", "es", "aux", "fsz"};

	u32 no_data_wait_time = 0;
	struct pollfd pfd;
	int ret;
	char *buf;
	u32 buf_size;

	buf = (char *)malloc(MALLOC_BUF_SIZE);
	if (buf == NULL) {
		printf("malloc failed\n");
		return -1;
	}
	buf_size = MALLOC_BUF_SIZE;

	memset(dump_fd, 0xff, sizeof(dump_fd));
	memset(sum_pkt, 0, sizeof(sum_pkt));

	pfd.fd     = dev;
	pfd.events = POLLIN | POLLERR;

	do {
		if (dev_status < 0) {
			ret = dev_status;
			break;
		}
		ret = poll(&pfd, 1, VDBG_POLL_TIMEOUT);
		if (ret < 0) {
			break;
		} else if (ret == 0) {
			if (++no_data_wait_time > 10) {
				printf("vdec debug waiting data (v%d) ...\n", code_version[0]);
				no_data_wait_time = 0;
				//break;
			}

			continue;
		}
		no_data_wait_time = 0;

		ret = read(dev, buf, buf_size);
		if (ret > 0) {
			struct port_data_packet_v0 pkt;

			memcpy(&pkt, buf, sizeof(pkt));

			if ((pkt.data_size > 0) &&
				((pkt.header != PACKET_HEADER) ||
				(pkt.id >= MAX_INSTANCE_NUM) ||
				(pkt.type >= TYPE_MAX))) {

				ret = ioctl(dev, VDEC_EXPORT_LOCAL_BUF_RST);
				if (ret < 0)
					printf("VDBG_IOC_BUF_RESET failed, message: %s\n", strerror(errno));
				continue;
			}
			/*
			printf("pkt[%d][%d](%d): 0x%x, size 0x%x\n",
				pkt.id, pkt.type, sum_pkt[pkt.id][pkt.type], pkt.header, pkt.data_size);
			*/
			/* coverity[tainted_data:SUPPRESS] */
			printf("pkt[%d][%d](%d): size 0x%x\n",
				pkt.id, pkt.type, sum_pkt[pkt.id][pkt.type], pkt.data_size);

			/*
			 * if pkt.type > 4, buffer will be reset and
			 * continue to reread buf.
			 */
			/* coverity[OVERRUN:SUPPRESS] */
			sum_pkt[pkt.id][pkt.type]++;

			if (pkt.type == TYPE_INFO) {
				struct port_vdec_info info;

				memcpy(&info, buf + sizeof(pkt), sizeof(info));

				printf("info[%d]: \n", info.id);
				printf("format  : %d\n", info.format);
				printf("width   : %d\n", info.dw_w);
				printf("height  : %d\n", info.dw_h);

				memset(file_str, 0, sizeof(file_str));
				snprintf(file_str, sizeof(file_str), "%d_%d_%dx%d",
					pkt.id, info.format, info.dw_w, info.dw_h);
				continue;
			} else {
				int fd = dump_fd[pkt.id][pkt.type];

				if (fd > 0) {
#if 0
					//create a new file when file str changed ?
					if (strncmp(file_str, last_str, sizeof(file_str))) {
						printf("close old file %d\n", fd);
						close(fd);
						fd = -1;
						dump_fd[pkt.id][pkt.type] = -1;
					}
#endif
				} else if (fd <= 0) {
					memset(file_name, 0, sizeof(file_name));
					/* coverity[OVERRUN:SUPPRESS] */
					snprintf(file_name, sizeof(file_name), "%s/%s.%s",
						file_path, file_str, file_ext[pkt.type]);
					memcpy(last_str, file_str, sizeof(file_str));

					fd = open(file_name, O_RDWR | O_CREAT | O_APPEND, 0644); //O_TRUNC
					if (fd < 0) {
						printf("create yuv %s failed, err %d\n", file_name, errno);
						continue;
					}
					/* coverity[OVERRUN:SUPPRESS] */
					dump_fd[pkt.id][pkt.type] = fd;

					printf("create file %s success\n", file_name);
				}

				/* coverity[tainted_data:SUPPRESS] */
				debug_port_save_file_append(fd, buf + sizeof(pkt), pkt.data_size);
			}
		}
	} while(1);

	for (i = 0; i < MAX_INSTANCE_NUM; i++) {
		for (j = 0; j < TYPE_MAX; j++) {
			if (dump_fd[i][j] > 0) {
				close(dump_fd[i][j]);
				dump_fd[i][j] = -1;
				printf("instance %d, type %d, total %d packets received\n", i, j, sum_pkt[i][j]);
			}
		}
	}

	free(buf);

	return 0;
}

int vdec_dbg_get_exported_data(int dev)
{
	char *buf;
	struct pollfd pfd;
	int ret, no_data = 15;
	struct vdec_dbg_ex_usr info;
	char ffpath[256];
	void *vaddr;

	buf = (char *)malloc(MALLOC_BUF_SIZE);
	if (buf == NULL) {
		printf("malloc failed\n");
		return -1;
	}

	memset(ffpath, 0, sizeof(ffpath));

	pfd.fd     = dev;
	pfd.events = POLLIN | POLLERR;

	do {
		if (dev_status < 0) {
			ret = dev_status;
			break;
		}
		ret = poll(&pfd, 1, VDBG_POLL_TIMEOUT);
		if (ret < 0) {
			break;
		} else if (ret == 0) {
			if (++no_data > 15) {
				printf("vdec debug waiting data (v%d.%d) ...\n", code_version[0], code_version[1]);
				no_data = 0;
			}
			continue;
		}
		no_data = 0;

		ret = ioctl(dev, VDEC_EXPORT_PACKET, &info);
		if (ret < 0) {
			printf("ioctl get data failed, errno %d\n", errno);
			continue;
		}

		PR_DBG(1, "=> id %d, size %x, file %s\n", info.id, info.data_size, info.file);

		if (!strcmp(info.file, "EXIT")) {
			dev_status = -1;
			printf("driver removing, exit\n");
			break;
		}

		if (info.type == DEV_READ_MMAP && info.data_size) {
			vaddr = mmap(NULL, info.data_size,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				dev, 0);
			if (vaddr == MAP_FAILED) {
				printf("mmap failed\n");
			} else {
				snprintf(ffpath, sizeof(ffpath),
					"%s/%s", file_path, info.file);

				debug_port_write_append_sync(ffpath, vaddr, info.data_size);

				munmap(vaddr, info.data_size);

				continue;
			}
		}

		ret = read(dev, buf, info.data_size);
		if (ret) {
			snprintf(ffpath, sizeof(ffpath),
				"%s/%s", file_path, info.file);

			debug_port_write_append_sync(ffpath, buf, info.data_size);
		}

	} while(1);

	free(buf);

	return 0;
}

void usage(void)
{
	printf("Usage:\n");
	printf("vdec_debug -p <path> -d <dbg_level>\n");
	printf("  -p, --path, create or specify the dump file path\n");
	printf("  -d, --debug, print more log for debug\n");
	printf("  -h, --help,  usage\n");
	printf("\n");
}

static int mkdir_p(const char *path, mode_t mode)
{
	char tmp[512];
	char *p = NULL;
	size_t len;

	if (!path) return -1;

	len = strlen(path);
	if (len >= sizeof(tmp)) {
		errno = ENAMETOOLONG;
		return -1;
	}

	strcpy(tmp, path);

	if (tmp[len - 1] == '/')
		tmp[len - 1] = '\0';

	for (p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			if (mkdir(tmp, mode) != 0) {
				if (errno != EEXIST) {
					return -1;
				}
			}
			*p = '/';
		}
	}

	if (mkdir(tmp, mode) != 0) {
		if (errno != EEXIST) {
			return -1;
		}
	}

	return 0;
}

void file_path_set(void)
{
	if (file_path[0] == 0)
		strcpy(file_path, "/data/tmp/");

	if (access(file_path, F_OK) == 0)
		printf("access path ok %s\n", file_path);
	else {
		if (mkdir_p(file_path, 0755) != 0) {
			printf("mkdir failed %s\n", file_path);
			return;
		}
		printf("create path ok %s\n", file_path);
	}
}

static int parse_para(int argc, char *argv[])
{
	memset(file_path, 0, sizeof(file_path));

	while (1) {
		int idx;
		int c;

		c = getopt_long(argc, argv, short_opt, long_opt, &idx);

		if (-1 == c)
			break;

		switch (c) {
			case 0: /* getopt_long() flag */
				break;
			case 'c':
				printf("todo: config\n");
				break;

			case 'p':
				strncpy(file_path, optarg, sizeof(file_path) - 1);
				printf("config file path %s\n", file_path);
				break;
			case 'd':
				_in_debug = 1;
				break;
			case 'h':
				usage();
				return -1;
			default:
				usage();
			return -1;
		}
	}
	return 0;
}

static int vdec_debug_init(int *ver)
{
	int fd;
	int version[2];
	int ret;

	fd = open("/dev/vdec_debug", O_RDWR);
	if (fd < 0) {
		fd = open("/sys/kernel/debug/vdec_profile/debug_port", O_RDWR);
		if (fd < 0) {
			printf("open device failed, error %d\n", errno);
			return -1;
		}
	}

	ret = ioctl(fd, VDEC_EXPORT_VERSION, version);
	if (ret < 0) {
		version[0] = 0;
		version[1] = 0;
		printf("ret %d, can get version, errno %d\n", ret, errno);
	}
	ver[0] = version[0];
	ver[1] = version[1];

	printf("VDEC_DEBUG DRIVER VERSION %d.%d\n", version[0], version[1]);

	dev_status = 0;
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);

	return fd;
}

int main(int argc, char *argv[])
{
	int dev = -1;

	if (parse_para(argc, argv))
		return -1;

	file_path_set();

	dev = vdec_debug_init(code_version);
	if (dev < 0)
		return -1;

	if (code_version[0] == 1)
		vdec_dbg_get_exported_data(dev);
	else
		vdec_dbg_get_data_v0(dev);

	close(dev);

	printf("exited\n");

	return 0;
}

