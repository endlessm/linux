// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Intel Corporation */

#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/vsc.h>
#include <linux/wait.h>

#include "intel_vsc.h"

#define ACE_PRIVACY_ON 2

struct intel_vsc {
	struct mutex mutex;

	void *csi;
	struct vsc_csi_ops *csi_ops;
	uint16_t csi_registerred;
	wait_queue_head_t csi_waitq;

	void *ace;
	struct vsc_ace_ops *ace_ops;
	uint16_t ace_registerred;
	wait_queue_head_t ace_waitq;
};

static struct intel_vsc vsc;

static int wait_component_ready(void)
{
	int ret;

	ret = wait_event_interruptible(vsc.ace_waitq,
				       vsc.ace_registerred);
	if (ret < 0) {
		pr_err("wait ace register failed\n");
		return ret;
	}

	ret = wait_event_interruptible(vsc.csi_waitq,
				       vsc.csi_registerred);
	if (ret < 0) {
		pr_err("wait csi register failed\n");
		return ret;
	}

	return 0;
}

static void update_camera_status(struct vsc_camera_status *status,
				 struct camera_status *s)
{
	if (status && s) {
		status->owner = s->camera_owner;
		status->exposure_level = s->exposure_level;
		status->status = VSC_PRIVACY_OFF;

		if (s->privacy_stat == ACE_PRIVACY_ON)
			status->status = VSC_PRIVACY_ON;
	}
}

int vsc_register_ace(void *ace, struct vsc_ace_ops *ops)
{
	if (ace && ops)
		if (ops->ipu_own_camera && ops->ace_own_camera) {
			mutex_lock(&vsc.mutex);
			vsc.ace = ace;
			vsc.ace_ops = ops;
			vsc.ace_registerred = true;
			mutex_unlock(&vsc.mutex);

			wake_up_interruptible_all(&vsc.ace_waitq);
			return 0;
		}

	pr_err("register ace failed\n");
	return -1;
}
EXPORT_SYMBOL_GPL(vsc_register_ace);

void vsc_unregister_ace(void)
{
	mutex_lock(&vsc.mutex);
	vsc.ace_registerred = false;
	mutex_unlock(&vsc.mutex);
}
EXPORT_SYMBOL_GPL(vsc_unregister_ace);

int vsc_register_csi(void *csi, struct vsc_csi_ops *ops)
{
	if (csi && ops)
		if (ops->set_privacy_callback &&
		    ops->set_owner && ops->set_mipi_conf) {
			mutex_lock(&vsc.mutex);
			vsc.csi = csi;
			vsc.csi_ops = ops;
			vsc.csi_registerred = true;
			mutex_unlock(&vsc.mutex);

			wake_up_interruptible_all(&vsc.csi_waitq);
			return 0;
		}

	pr_err("register csi failed\n");
	return -1;
}
EXPORT_SYMBOL_GPL(vsc_register_csi);

void vsc_unregister_csi(void)
{
	mutex_lock(&vsc.mutex);
	vsc.csi_registerred = false;
	mutex_unlock(&vsc.mutex);
}
EXPORT_SYMBOL_GPL(vsc_unregister_csi);

int vsc_acquire_camera_sensor(struct vsc_mipi_config *config,
			      vsc_privacy_callback_t callback,
			      void *handle,
			      struct vsc_camera_status *status)
{
	int ret;
	struct camera_status s;
	struct mipi_conf conf = { 0 };

	struct vsc_csi_ops *csi_ops;
	struct vsc_ace_ops *ace_ops;

	if (!config)
		return -EINVAL;

	ret = wait_component_ready();
	if (ret)
		return ret;

	mutex_lock(&vsc.mutex);
	if (!vsc.csi_registerred || !vsc.ace_registerred) {
		ret = -1;
		goto err;
	}

	csi_ops = vsc.csi_ops;
	ace_ops = vsc.ace_ops;

	csi_ops->set_privacy_callback(vsc.csi, callback, handle);

	ret = ace_ops->ipu_own_camera(vsc.ace, &s);
	if (ret) {
		pr_err("ipu own camera failed\n");
		goto err;
	}
	update_camera_status(status, &s);

	ret = csi_ops->set_owner(vsc.csi, CSI_IPU);
	if (ret) {
		pr_err("ipu own csi failed\n");
		goto err;
	}

	conf.lane_num = config->lane_num;
	conf.freq = config->freq;
	ret = csi_ops->set_mipi_conf(vsc.csi, &conf);
	if (ret) {
		pr_err("config mipi failed\n");
		goto err;
	}

err:
	mutex_unlock(&vsc.mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(vsc_acquire_camera_sensor);

int vsc_release_camera_sensor(struct vsc_camera_status *status)
{
	int ret;
	struct camera_status s;

	struct vsc_csi_ops *csi_ops;
	struct vsc_ace_ops *ace_ops;

	ret = wait_component_ready();
	if (ret)
		return ret;

	mutex_lock(&vsc.mutex);
	if (!vsc.csi_registerred || !vsc.ace_registerred) {
		ret = -1;
		goto err;
	}

	csi_ops = vsc.csi_ops;
	ace_ops = vsc.ace_ops;

	csi_ops->set_privacy_callback(vsc.csi, NULL, NULL);

	ret = csi_ops->set_owner(vsc.csi, CSI_FW);
	if (ret) {
		pr_err("vsc own csi failed\n");
		goto err;
	}

	ret = ace_ops->ace_own_camera(vsc.ace, &s);
	if (ret) {
		pr_err("vsc own camera failed\n");
		goto err;
	}
	update_camera_status(status, &s);

err:
	mutex_unlock(&vsc.mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(vsc_release_camera_sensor);

static int __init intel_vsc_init(void)
{
	memset(&vsc, 0, sizeof(struct intel_vsc));

	mutex_init(&vsc.mutex);

	vsc.csi_registerred = false;
	vsc.ace_registerred = false;

	init_waitqueue_head(&vsc.ace_waitq);
	init_waitqueue_head(&vsc.csi_waitq);

	return 0;
}

static void __exit intel_vsc_exit(void)
{
	if (wq_has_sleeper(&vsc.ace_waitq))
		wake_up_all(&vsc.ace_waitq);

	if (wq_has_sleeper(&vsc.csi_waitq))
		wake_up_all(&vsc.csi_waitq);
}

module_init(intel_vsc_init);
module_exit(intel_vsc_exit);

MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Device driver for Intel VSC");
