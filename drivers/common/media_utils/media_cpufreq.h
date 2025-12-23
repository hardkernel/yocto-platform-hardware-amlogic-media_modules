/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MEDIA_CPUFREQ_H__
#define __MEDIA_CPUFREQ_H__

#include <linux/cpufreq.h>
#include <linux/pm_qos.h>
#include <linux/smp.h>

enum media_set_cpufreq_type {
	MEDIA_BOOST_FREQ_MIN = 1,	/* set min freq */
	MEDIA_THROTTLE_FREQ_MAX,	/* set max freq */
};

static inline uint media_get_max_cpufreq(void)
{
	unsigned int max_freq;
	unsigned int cpu = smp_processor_id();
	struct cpufreq_policy *policy;

	policy = cpufreq_cpu_get(cpu);
	if (IS_ERR_OR_NULL(policy)) {
		pr_err("cpu%d policy not ready\n", cpu);
		return -EINVAL;
	}

	max_freq = policy->cpuinfo.max_freq;
	cpufreq_cpu_put(policy);

	return max_freq;
}

static inline uint media_get_min_cpufreq(void)
{
	unsigned int max_freq;
	unsigned int cpu = smp_processor_id();
	struct cpufreq_policy *policy;

	policy = cpufreq_cpu_get(cpu);
	if (IS_ERR_OR_NULL(policy)) {
		pr_err("cpu%d policy not ready\n", cpu);
		return -EINVAL;
	}

	max_freq = policy->cpuinfo.min_freq;
	cpufreq_cpu_put(policy);

	return max_freq;
}

/* set current cup core frequence */
static inline int media_set_cpufreq(struct freq_qos_request *qos_req, uint freq, enum media_set_cpufreq_type type)
{
	unsigned int cpu = smp_processor_id();
	struct cpufreq_policy *policy;
	enum freq_qos_req_type qos_type;
	int ret = 0;

	if (freq_qos_request_active(qos_req)) {
		pr_err("[%s]req:%px actived, freq:%d kHz\n", __func__, qos_req, cpufreq_get(cpu));
		return -EINVAL;
	}

	policy = cpufreq_cpu_get(cpu);
	if (IS_ERR_OR_NULL(policy)) {
		pr_err("cpu%d policy not ready\n", cpu);
		return -EINVAL;
	}

	if (type == MEDIA_BOOST_FREQ_MIN)
		qos_type = FREQ_QOS_MIN;
	else
		qos_type = FREQ_QOS_MAX;
	ret = freq_qos_add_request(&policy->constraints, qos_req, qos_type, freq);
	if (ret < 0) {
		pr_err("[%s] add request failed, ret:%d.\n", __func__, ret);
		return ret;
	}

	cpufreq_cpu_put(policy);

	return 0;
}

static inline int media_update_cpufreq(struct freq_qos_request *qos_req, int freq) {
	int ret = 0;

	ret = freq_qos_update_request(qos_req, freq);
	if (ret < 0)
		pr_err("[%s] update request %d failed, ret:%d.\n", __func__, freq, ret);

	return ret;
}

static inline void media_recover_cpufreq(struct freq_qos_request *qos_req)
{
	if (!freq_qos_request_active(qos_req)) {
		pr_err("[%s]req:%px not active!\n", __func__, qos_req);
		return;
	}

	freq_qos_remove_request(qos_req);
}

#endif

