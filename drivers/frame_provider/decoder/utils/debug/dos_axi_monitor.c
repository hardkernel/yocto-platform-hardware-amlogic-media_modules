/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/interrupt.h>
#include "dos_axi_monitor.h"
#include "../../../../common/chips/decoder_cpu_ver_info.h"
#include "../../../../common/register/register.h"
#include "../../../../include/regs/dos_registers.h"


struct dos_axi_monitor_t dos_axi_monitor;

static struct dos_axi_monitor_t *get_dos_axi_mon(void)
{
	return &dos_axi_monitor;
}

irqreturn_t dec_axi_monitor_isr(void)
{
	u32 i, val, id, ign = 0;
	u32 pc, cur_pc;
	u64 addr = 0ULL;
	struct dos_axi_monitor_t *mon;

	cur_pc = READ_VREG(HEVC_MPC_E);
	pc = READ_VREG(PCLATCH);
	val = READ_VREG(HEVC_AXI_MON_B_ERR_ID);
	addr = READ_VREG(HEVC_AXI_MON_B_ERR_ADDR);

	id = (val >> 4) & 0xff;
	addr |= ((u64)(val & 0x3) << 32);

	mon = get_dos_axi_mon();
	for (i = 0; i < MAX_SW_IGN; i++) {
		if (id == mon->sw_ign_id[i]) {
			ign = 1;
			break;
		}
	}
	if (!ign)
		pr_info("[DOS_VIO]: id=0x%x, addr=0x%llx, pc(0x%x, 0x%x)\n", id, addr, pc, cur_pc);

	SET_VREG_MASK(HEVC_AXI_MON_CTRL, (1 << 4));

	return IRQ_HANDLED;
}

bool is_axi_mon_enabled(void)
{
	return (get_dos_axi_mon()->axi_monitor_ctl & 0x3);
}
EXPORT_SYMBOL(is_axi_mon_enabled);

void show_axi_mon_reg(void)
{
	if (!is_axi_mon_enabled())
		return;

	pr_info("protect: %x ~ %x, res %x\n",
		READ_VREG(HEVC_AXI_MON_A_START),
		READ_VREG(HEVC_AXI_MON_A_END),
		READ_VREG(HEVC_AXI_MON_A_RSV));

	pr_info("monitor: %x ~ %x, ign id %x\n",
		READ_VREG(HEVC_AXI_MON_B_START),
		READ_VREG(HEVC_AXI_MON_B_END),
		READ_VREG(HEVC_AXI_MON_B_IGN_ID));

	pr_info("monitor ctl: %x, INTA 0x%x\n",
		READ_VREG(HEVC_AXI_MON_CTRL), READ_VREG(HEVC_ASSIST_AMR1_INTA));

	pr_info("err addr %x, err id %x",
		READ_VREG(HEVC_AXI_MON_B_ERR_ADDR), READ_VREG(HEVC_AXI_MON_B_ERR_ID));
}
EXPORT_SYMBOL(show_axi_mon_reg);

static int protect_mem_config(ulong addr, ulong size, ulong res, u32 res_size)
{
	struct dos_axi_monitor_t *mon = get_dos_axi_mon();

	mon->protect_start = addr;
	mon->protect_size  = size;
	mon->res_start     = res;
	mon->res_size      = res_size;
	mon->axi_monitor_ctl |= ENABLE_PROTECT;

	pr_info("enable protect range: %lx ~ %lx, res %lx\n", addr, addr + size, res);

	return 0;
}

static int monitor_mem_config(ulong addr, ulong size, u32 port_id)
{
	struct dos_axi_monitor_t *mon = get_dos_axi_mon();

	mon->monitor_start = addr;
	mon->monitor_size = size;
	mon->monitor_id = port_id & 0xff;
	mon->axi_monitor_ctl |= ENABLE_MONITOR;

	pr_info("enable monitor range: %lx ~ %lx, id %x\n", addr, addr + size, mon->monitor_id);

	return 0;
}

static int monitor_ign_config(ulong *port, u32 num)
{
	struct dos_axi_monitor_t *mon = get_dos_axi_mon();
	int i;

	if (!port || !num)
		return -1;

	if (num > (MAX_HW_IGN + MAX_SW_IGN))
		pr_warn("no enough ignore id config(%d > %d)\n", num, (MAX_HW_IGN + MAX_SW_IGN));

	for (i = 0; i < MAX_HW_IGN; i++) {
		if (i >= num)
			mon->monitor_ign_id[i] = 0;
		else
			mon->monitor_ign_id[i] = port[i];
	}

	pr_info("hw ignore: 0x%x, 0x%x, 0x%x, 0x%x\n",
		mon->monitor_ign_id[0],
		mon->monitor_ign_id[1],
		mon->monitor_ign_id[2],
		mon->monitor_ign_id[3]);

	if (num <= MAX_HW_IGN)
		return 0;
	num -= MAX_HW_IGN;

	pr_cont("sw ignore : ");
	for (i = 0; i < MAX_SW_IGN; i++) {
		if (i >= num)
			mon->sw_ign_id[i] = 0;
		else
			mon->sw_ign_id[i] = port[i + MAX_HW_IGN];

		pr_cont("0x%x ", mon->sw_ign_id[i]);
	}
	pr_cont("\n");

	return 0;
}

void hw_axi_monitor_config(void)
{
	struct dos_axi_monitor_t *mon = get_dos_axi_mon();
	ulong end_addr;
	int val;

	if (!is_support_axi_monitor())
		return;

	if ((mon->axi_monitor_ctl & (ENABLE_PROTECT | ENABLE_MONITOR)) == 0)
		return;

	SET_VREG_MASK(DOS_GCLK_EN3, 0x3); //assist

	val = READ_VREG(HEVC_AXI_MON_CTRL);

	if (mon->axi_monitor_ctl & ENABLE_PROTECT) {
		end_addr = mon->protect_start + mon->protect_size;

		WRITE_VREG(HEVC_AXI_MON_A_START, mon->protect_start);
		WRITE_VREG(HEVC_AXI_MON_A_END, end_addr);
		WRITE_VREG(HEVC_AXI_MON_A_RSV, mon->res_start);

		val &= ~(0x3f << 5);
		val |= (PREFIX_ADDR(mon->protect_start) << 5) |
				(PREFIX_ADDR(end_addr) << 7) |
				(PREFIX_ADDR(mon->res_start) << 9);

		val |= 1;  //enable protect
	}

	if (mon->axi_monitor_ctl & ENABLE_MONITOR) {
		end_addr = mon->monitor_start + mon->monitor_size;

		WRITE_VREG(HEVC_AXI_MON_B_START, mon->monitor_start);
		WRITE_VREG(HEVC_AXI_MON_B_END, end_addr);

		WRITE_VREG(DOS_HEVC_INT_EN, (1 << 17));
		WRITE_VREG(HEVC_ASSIST_AMR1_INTA, 0x11);

		val &= ~((0xfff << 11) | (0xf << 1));

		val |= (PREFIX_ADDR(mon->monitor_start) << 11) |
				(PREFIX_ADDR(end_addr) << 13) |
				((mon->monitor_id & 0xff) << 15);

		val |= (1 << 1) |  //enable
			(1 << 2) | //irq enable
			(0 << 3);  //pulse trigger
	}

	if (mon->axi_monitor_ctl) {
		WRITE_VREG(HEVC_AXI_MON_B_IGN_ID, (mon->monitor_ign_id[0] << 0) |
			(mon->monitor_ign_id[1] << 8) |
			(mon->monitor_ign_id[2] << 16) |
			(mon->monitor_ign_id[3] << 24));
	}

	WRITE_VREG(HEVC_AXI_MON_CTRL, val);
}
EXPORT_SYMBOL(hw_axi_monitor_config);

u32 show_dos_axi_monitor_config(char *buf)
{
	struct dos_axi_monitor_t *mon;
	u32 len = 0, i;

	if (!is_support_axi_monitor()) {
		pr_err("no axi monitor in this chip\n");
		return 0;
	}

	mon = get_dos_axi_mon();

	len += snprintf(buf + len, 4096 - len, "protect: \n");
	len += snprintf(buf + len, 4096 - len, "\t 0x%lx ~ 0x%lx. res 0x%lx\n",
		mon->protect_start, mon->protect_start + mon->protect_size, mon->res_start);

	len += snprintf(buf + len, 4096 - len, "monitor: \n");
	len += snprintf(buf + len, 4096 - len, "\t 0x%lx ~ 0x%lx. id 0x%x\n",
		mon->monitor_start, mon->monitor_start + mon->monitor_size, mon->monitor_id);

	len += snprintf(buf + len, 4096 - len, "ignore id (hw): \n\t");
	for (i = 0; i < MAX_HW_IGN; i++) {
		len += snprintf(buf + len, 4096 - len, "0x%x  ",
			mon->monitor_ign_id[i]);
	}

	len += snprintf(buf + len, 4096 - len, "\nignore id (sw): ");
	for (i = 0; i < MAX_SW_IGN; i++) {
		if ((i % 8) == 0)
			len += snprintf(buf + len, 4096 - len, "\n\t");
		len += snprintf(buf + len, 4096 - len, "0x%x  ",
			mon->sw_ign_id[i]);
	}
	len += snprintf(buf + len, 4096 - len, "\n");

	return len;
}

ssize_t axi_monitor_config_setup(const char *buf, size_t size)
{
	//echo protect 0x61000000  0x8000000  $res $res_size
	//echo monitor 0x70000000  0x9000000  $id $ign_id ...
	int ret;
	char *str, *token, *tmp;
	ulong para[MAX_HW_IGN + MAX_SW_IGN];

	if (!is_support_axi_monitor()) {
		pr_err("no axi monitor in this chip\n");
		return 0;
	}

	memset(para, 0, sizeof(para));
	tmp = kstrdup(buf, GFP_KERNEL);
	if (!tmp)
		return 0;

	str = tmp;
	token = strsep(&str, " ");

	ret = sscanf(str, "%lx %lx %lx %lx %lx %lx %lx %lx",
		&para[0], &para[1], &para[2], &para[3],
		&para[4], &para[5], &para[6], &para[7]);

	if (!strncmp(token, "protect", strlen("protect"))) {
		protect_mem_config(para[0], para[1], para[2], para[3]);

	} else if (!strncmp(token, "monitor", strlen("monitor"))) {
		monitor_mem_config(para[0], para[1], para[2]);

	} else if (!strncmp(token, "ignore", strlen("ignore"))) {
		monitor_ign_config(&para[0], (MAX_HW_IGN + MAX_SW_IGN));

	} else if (!strncmp(token, "disable", strlen("disable"))) {
		struct dos_axi_monitor_t *mon = get_dos_axi_mon();

		if (para[0] == 0) {
			protect_mem_config(0, 0, 0, 0);
			mon->axi_monitor_ctl &= ~(ENABLE_PROTECT);
			pr_info("disable protect\n");
		} else if (para[0] > 0) {
			monitor_mem_config(0, 0, 0);
			para[3] = 0;
			para[4] = 0;
			para[5] = 0;
			para[6] = 0;
			mon->axi_monitor_ctl &= ~(ENABLE_MONITOR);
			pr_info("disable monitor\n");
		}
	}

	kfree(tmp);

	return 0;
}

