/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
//#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/pinctrl/consumer.h>
#include <linux/reset.h>
#include <linux/of_gpio.h>
#include <linux/amlogic/media/utils/amstream.h>
//#include <linux/clk.h>
#include "c_stb_define.h"
#include "c_stb_regs_define.h"
#include "../aml_dvb.h"
#include "dvb_reg.h"

#include "demod_gt.h"
#include "../../../../common/media_clock/switch/amports_gate.h"

#define pr_error(fmt, args...) printk("DVB: " fmt, ## args)
#define pr_inf(fmt, args...)   printk("DVB: " fmt, ## args)

static struct dvb_frontend *frontend[FE_DEV_COUNT] = {NULL, NULL};
static enum dtv_demod_type s_demod_type[FE_DEV_COUNT] = {AM_DTV_DEMOD_NONE, AM_DTV_DEMOD_NONE};

int frontend_probe(struct platform_device *pdev)
{
	struct demod_config config;
	char buf[32];
	const char *str = NULL;
	struct device_node *node_tuner = NULL;
	int i = 0;
	int ret =0;
	struct aml_dvb *advb = aml_get_dvb_device();

	for (i=0; i<FE_DEV_COUNT; i++) {
		memset(&config, 0, sizeof(struct demod_config));

		memset(buf, 0, 32);
		snprintf(buf, sizeof(buf), "fe%d_mode", i);
		ret = of_property_read_string(pdev->dev.of_node, buf, &str);
		if (ret) {
			continue;
		}

		if (!strcmp(str, "internal")) {
			config.mode = 0;
			config.id = AM_DTV_DEMOD_AMLDTV;
			frontend[i] = aml_attach_dtvdemod(config.id, &config);
			if (frontend[i] == NULL) {
				s_demod_type[i] = AM_DTV_DEMOD_NONE;
				pr_error("internal dtvdemod [type = %d] attach error.\n", config.id);
				goto error_fe;
			} else {
				s_demod_type[i] = config.id;
				pr_error("internal dtvdemod [type = %d] attach success.\n", config.id);
			}

			/* define general-purpose callback pointer */
			frontend[i]->callback = NULL;

			if (dvb_tuner_attach(frontend[i]) == NULL) {
				pr_error("tuner attach error.\n");
				goto error_fe;
			} else {
				pr_error("tuner attach sucess.\n");
			}

			ret = dvb_register_frontend(&advb->dvb_adapter, frontend[i]);
			if (ret) {
				pr_error("register dvb frontend failed\n");
				goto error_fe;
			}
		} else if(!strcmp(str, "external")) {
			config.mode = 1;
			config.id = AM_DTV_DEMOD_NONE;
			ret = aml_get_dts_demod_config(pdev->dev.of_node, &config, i);
			if (ret) {
				pr_err("can't find demod %d.\n", i);
				continue;
			}

			memset(buf, 0, 32);
			snprintf(buf, sizeof(buf), "fe%d_tuner", i);
			node_tuner = of_parse_phandle(pdev->dev.of_node, buf, 0);
			if (node_tuner) {
				aml_get_dts_tuner_config(node_tuner, &config.tuner0, 0);
				aml_get_dts_tuner_config(node_tuner, &config.tuner1, 1);
			} else
				pr_err("can't find %s.\n", buf);

			of_node_put(node_tuner);

			frontend[i] = aml_attach_dtvdemod(config.id, &config);
			if (frontend[i] == NULL) {
				s_demod_type[i] = AM_DTV_DEMOD_NONE;
				pr_error("external dtvdemod [type = %d] attach error.\n", config.id);
				goto error_fe;
			} else {
				s_demod_type[i] = config.id;
				pr_error("external dtvdemod [type = %d] attach success.\n", config.id);
			}

			if (frontend[i]) {
				ret = dvb_register_frontend(&advb->dvb_adapter, frontend[i]);
				if (ret) {
					pr_error("register dvb frontend failed\n");
					goto error_fe;
				}
			}
		}
	}

	return 0;
error_fe:
	for (i=0; i<FE_DEV_COUNT; i++) {
		aml_detach_dtvdemod(s_demod_type[i]);
		frontend[i] = NULL;
		s_demod_type[i] = AM_DTV_DEMOD_NONE;

		dvb_tuner_detach();
	}

	return 0;
}

int frontend_remove(void)
{
	int i;

	for (i=0; i<FE_DEV_COUNT; i++) {
		aml_detach_dtvdemod(s_demod_type[i]);

		dvb_tuner_detach();

		if (frontend[i]) {
			dvb_unregister_frontend(frontend[i]);
			dvb_frontend_detach(frontend[i]);
		}

		frontend[i] = NULL;
		s_demod_type[i] = AM_DTV_DEMOD_NONE;
	}

	return 0;
}

