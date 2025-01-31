/*
 * Samsung Exynos5 SoC series Flash driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>

#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#include "fimc-is-flash.h"
#include "fimc-is-flash-gpio.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-helper-flash-i2c.h"
#include "fimc-is-core.h"
#include "fimc-is-regs.h"
#include "fimc-is-flash-i2c.h"

#ifdef CAMERA_SYSFS_V2
struct device *led_dev;
extern struct class *camera_class;
#endif

static int flash_torch_set_init(struct i2c_client *client)
{
	int ret = 0;

	/* Auto BST EN */
	ret = fimc_is_flash_updata_reg(client, REG_FLED_CTRL0, AUTO_BST_EN_ON, AUTO_BST_EN_MASK);
	if (ret < 0)
		warn("%s: Failed to auto BST EN setting\n", __func__);

	/* Time out Max Setting */
	ret = fimc_is_flash_updata_reg(client, REG_FLED_CTRL4, TIMEOUT_MAX, TIMEOUT_MASK);
	if (ret < 0)
		warn("%s: Failed to time out max setting\n", __func__);

	/* Torch Timer Max mode setting */
	ret = fimc_is_flash_updata_reg(client, REG_FLED_CTRL4, TORCH_TMR_MODE_ON, TORCH_TMR_MODE_MASK);
	if (ret < 0)
		warn("%s: Failed to torch time out max setting\n", __func__);

	/* Torch off */
	ret = fimc_is_flash_updata_reg(client, REG_FLED_CTRL0, TORCH_ENABLE_OFF, TORCH_ENABLE_MASK);
	if (ret < 0)
		err("torch flash off fail");

	return ret;
}

#ifdef CONFIG_MUIC_NOTIFIER
static void attach_cable_check(muic_attached_dev_t attached_dev,
	int *attach_ta, int *attach_sdp)
{
	BUG_ON(!attach_ta);
	BUG_ON(!attach_sdp);

	if (attached_dev == ATTACHED_DEV_USB_MUIC)
		*attach_sdp = 1;
	else
		*attach_sdp = 0;

	switch (attached_dev) {
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_SMARTDOCK_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_TA_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
		*attach_ta = 1;
		break;
	default:
		*attach_ta = 0;
	}
}

static int flash_ta_notification(struct notifier_block *nb,
	unsigned long action, void *data)
{
	int ret = 0;
	u8 reg = 0;
	struct fimc_is_flash *flash;
	muic_attached_dev_t attached_dev;

	BUG_ON(!data);

	attached_dev = *(muic_attached_dev_t *)data;

	flash = container_of(nb, struct fimc_is_flash, flash_noti_ta);
	BUG_ON(!flash);

	switch (action) {
	case MUIC_NOTIFY_CMD_DETACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
		if (!flash->attach_ta)
			goto p_err;

		flash->attach_ta = 0;

		if (flash->flash_data.mode == CAM2_FLASH_MODE_NONE) {
			dbg_flash("%s: flash mode nothing\n", __func__);
			goto p_err;
		}

		/* CHGVIN bit disable, AUTO_BST bit enable */
		ret = fimc_is_flash_updata_reg(flash->client, REG_FLED_CTRL0,
				(CHGVIN_EN_OFF | AUTO_BST_EN_ON),
				(CHGVIN_EN_MASK | AUTO_BST_EN_MASK));
		if (ret < 0) {
			pr_err("CHGVIN, AUTO_BST function change fail");
			goto p_err;
		}

		fimc_is_flash_data_read(flash->client, REG_FLED_CTRL0, &reg);
		if (reg < 0) {
			pr_err("i2c read fail\n");
			goto p_err;
		}

		/*
		 * If ta(usb) detach when torch working
		 * go out to torch so write i2c torch off and torch on
		 */
		if ((reg & 0x20) == 0x20) {
			/* Torch off */
			ret = fimc_is_flash_updata_reg(flash->client, REG_FLED_CTRL0,
				TORCH_ENABLE_OFF, TORCH_ENABLE_MASK);
			if (ret < 0) {
				pr_err("torch off is failed");
				goto p_err;
			}

			/* Torch on */
			ret = fimc_is_flash_updata_reg(flash->client, REG_FLED_CTRL0,
				TORCH_ENABLE_ON, TORCH_ENABLE_MASK);
			if (ret < 0) {
				pr_err("torch on is failed");
				goto p_err;
			}
		}
		break;
	case MUIC_NOTIFY_CMD_ATTACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
		flash->attach_ta = 0;

		attach_cable_check(attached_dev, &flash->attach_ta, &flash->attach_sdp);

		if (flash->attach_ta) {
			/* CHGVIN bit enable, AUTO_BST bit disable */
			ret = fimc_is_flash_updata_reg(flash->client, REG_FLED_CTRL0,
					(CHGVIN_EN_ON | AUTO_BST_EN_OFF),
					(CHGVIN_EN_MASK | AUTO_BST_EN_MASK));
			if (ret < 0) {
				pr_err("CHGVIN, AUTO_BST function change fail");
				goto p_err;
			}
		}
		break;
	}

p_err:
	return ret;
}
#endif

static int flash_i2c_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct fimc_is_flash *flash;

	BUG_ON(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);

	BUG_ON(!flash);

	/* TODO: init flash driver */
	flash->flash_data.mode = CAM2_FLASH_MODE_OFF;
	flash->flash_data.intensity = 100; /* TODO: Need to figure out min/max range */
	flash->flash_data.firing_time_us = 1 * 1000 * 1000; /* Max firing time is 1sec */
	flash->flash_data.flash_fired = false;

	gpio_request_one(flash->flash_gpio, GPIOF_OUT_INIT_LOW, "CAM_FLASH_GPIO_OUTPUT");
	gpio_free(flash->flash_gpio);

	return ret;
}

static int sensor_i2c_flash_control(struct v4l2_subdev *subdev, enum flash_mode mode, u32 intensity)
{
	int ret = 0;
	struct fimc_is_flash *flash = NULL;

	BUG_ON(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);
	BUG_ON(!flash);

	info("%s : mode = %s, intensity = %d\n", __func__,
		mode == CAM2_FLASH_MODE_OFF ? "OFF" :
		mode == CAM2_FLASH_MODE_SINGLE ? "FLASH" : "TORCH",
		intensity);

	if (mode == CAM2_FLASH_MODE_OFF) {
		ret = control_flash_gpio(flash->flash_gpio, 0);
		if (ret)
			err("capture flash off fail");

		ret = fimc_is_flash_updata_reg(flash->client, REG_FLED_CTRL0, TORCH_ENABLE_OFF, TORCH_ENABLE_MASK);
		if (ret)
			err("torch flash off fail");
	} else if (mode == CAM2_FLASH_MODE_SINGLE) {
		ret = control_flash_gpio(flash->flash_gpio, intensity);
		if (ret)
			err("capture flash on fail");
		ret = fimc_is_flash_updata_reg(flash->client, REG_FLED_CTRL3, 0xff, CURRENT_MASK);
		if (ret)
			err("setting flash timing is fail");
	} else if (mode == CAM2_FLASH_MODE_TORCH) {
		/* Brightness set */
		ret = fimc_is_flash_updata_reg(flash->client, REG_FLED_CTRL1, (u8)(intensity & 0xff), CURRENT_MASK);
		/* Torch on */
		ret = fimc_is_flash_updata_reg(flash->client, REG_FLED_CTRL0, TORCH_ENABLE_ON, TORCH_ENABLE_MASK);
		if (ret)
			err("torch flash on fail");
	} else {
		err("Invalid flash mode");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int flash_i2c_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct fimc_is_flash *flash = NULL;

	BUG_ON(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);
	BUG_ON(!flash);

	switch(ctrl->id) {
	case V4L2_CID_FLASH_SET_INTENSITY:
		/* TODO : Check min/max intensity */
		if (ctrl->value < 0) {
			err("failed to flash set intensity: %d\n", ctrl->value);
			ret = -EINVAL;
			goto p_err;
		}
		flash->flash_data.intensity = ctrl->value;
		break;
	case V4L2_CID_FLASH_SET_FIRING_TIME:
		/* TODO : Check min/max firing time */
		if (ctrl->value < 0) {
			err("failed to flash set firing time: %d\n", ctrl->value);
			ret = -EINVAL;
			goto p_err;
		}
		flash->flash_data.firing_time_us = ctrl->value;
		break;
	case V4L2_CID_FLASH_SET_FIRE:
#ifdef CONFIG_VENDER_MCD
		ctrl->value = flash->flash_data.flash_current+flash->flash_data.movie_current;
#endif
		ret = sensor_i2c_flash_control(subdev, flash->flash_data.mode, ctrl->value);
		if (ret) {
			err("sensor_i2c_flash_control(mode:%d, val:%d) is fail(%d)",
					(int)flash->flash_data.mode, ctrl->value, ret);
			goto p_err;
		}
		break;
	default:
		err("err!!! Unknown CID(%#x)", ctrl->id);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

#ifdef CAMERA_SYSFS_V2
static ssize_t store_rear_torch_flash(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = 0;
	struct fimc_is_flash *flash;
	struct fimc_is_device_sensor *device;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_core *core;
	u32 sensor_id = 0;
	int value = 0;
	int brightness = 0;
	int mode = 0;

	if ((buf == NULL) || kstrtouint(buf, 10, &value)) {
		return -1;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(dev);
	if (!core) {
		err("core is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	/* TODO: Get a sensor_id by user */
	device = &core->sensor[sensor_id];
	if (!device) {
		err("sensor device is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	sensor_peri = find_peri_by_flash_id(device, FLADRV_NAME_DRV_FLASH_I2C);
	if (!sensor_peri) {
		err("sensor peri is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	flash = &sensor_peri->flash;

	if (value == 0) {
		/* Turn off Torch */
		mode = CAM2_FLASH_MODE_OFF;
		dbg_flash("flash torch off!\n");
	} else if(value == 1) {
		/* Turn on Torch */
		brightness = flash->flash_data.torch_current;
		mode = CAM2_FLASH_MODE_TORCH;
		dbg_flash("flash torch on!\n");
	} else if(value == 100) {
		/* Factory mode Turn on Torch */
		brightness = flash->flash_data.factory_current;
		mode = CAM2_FLASH_MODE_TORCH;
		dbg_flash("Factory mode flash torch on!\n");
	} else if(1001 <= value && value <= 1010) {
		/* Turn on Torch Step 20mA ~ 200mA */
		brightness = USE_FLASH_STEP(value);
		mode = CAM2_FLASH_MODE_TORCH;
		dbg_flash("Turn Torch Step on!\n");
	} else {
		pr_err("%s: Invalid value:%d\n", __func__, value);
		goto p_err;
	}

	info("%s : mode = %d, brightness = %d\n", __func__, mode, brightness);

	ret = sensor_i2c_flash_control(flash->subdev, mode, brightness);

	if (ret < 0) {
		err("Failed to i2c flash torch control");
		goto p_err;
	}

p_err:
	return count;
}

static ssize_t show_rear_torch_flash(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	pr_info("[LED] %s , MAX STEP TORCH_LED:%d\n", __func__, TORCH_OUT_I_MAX - 1);
	return sprintf(buf, "%d\n", TORCH_OUT_I_MAX - 1);
}

static DEVICE_ATTR(rear_torch_flash, 0644, show_rear_torch_flash, store_rear_torch_flash);
static DEVICE_ATTR(rear_flash, 0644, show_rear_torch_flash, store_rear_torch_flash);
#endif

static ssize_t store_flash_torch_control(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = 0;
	struct fimc_is_flash *flash;
	struct fimc_is_device_sensor *device;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_core *core;
	u32 sensor_id = 0;
	int value = 0;
	int mode = 0;

	core = (struct fimc_is_core *)dev_get_drvdata(dev);
	if (!core) {
		err("core is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	/* TODO: Get a sensor_id by user */
	device = &core->sensor[sensor_id];
	if (!device) {
		err("sensor device is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	sensor_peri = find_peri_by_flash_id(device, FLADRV_NAME_DRV_FLASH_I2C);
	if (!sensor_peri) {
		err("sensor peri is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	flash = &sensor_peri->flash;

	if (!strncmp(buf, "0", 1)) {
		value = 0x00;
		mode = CAM2_FLASH_MODE_OFF;
		dbg_flash("flash torch off!\n");
	} else if(!strncmp(buf, "1", 1)) {
		value = 0xf;
		mode = CAM2_FLASH_MODE_TORCH;
		dbg_flash("flash torch on!\n");
	} else {
		pr_err("%s: incorrect number\n", __func__);
		goto p_err;
	}

	ret = sensor_i2c_flash_control(flash->subdev, mode, value);

	if (ret < 0) {
		err("Failed to i2c flash torch control");
		goto p_err;
	}

p_err:
	return count;
}

static ssize_t show_flash_torch_control(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", buf);
}

static DEVICE_ATTR(flash_torch_control, 0644, show_flash_torch_control, store_flash_torch_control);

static struct attribute *fimc_is_flash_entries[] = {
	&dev_attr_flash_torch_control.attr,
	NULL,
};

static struct attribute_group fimc_is_flash_attr_group = {
	.name	= "torch",
	.attrs	= fimc_is_flash_entries,
};

static const struct v4l2_subdev_core_ops core_ops = {
	.init = flash_i2c_init,
	.s_ctrl = flash_i2c_s_ctrl,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

int flash_probe(struct device *dev, struct i2c_client *client)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_flash;
	struct fimc_is_device_sensor *device;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_flash *flash;
	u32 sensor_id = 0;
	struct device_node *dnode;

#ifdef CONFIG_VENDER_MCD
	u32 flash_current = 0;
	u32 movie_current = 0;
	u32 torch_current = 0;
	u32 factory_current = 0;
#endif

	BUG_ON(!fimc_is_dev);
	BUG_ON(!dev);

	dnode = dev->of_node;

	ret = of_property_read_u32(dnode, "id", &sensor_id);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto p_err;
	}

#ifdef CONFIG_VENDER_MCD
	ret = of_property_read_u32(dnode, "flash_current", &flash_current);
	if (ret) {
		err("flash_current read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "movie_current", &movie_current);
	if (ret) {
		err("movie_current read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "torch_current", &torch_current);
	if (ret) {
		err("movie_current read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "factory_current", &factory_current);
	if (ret) {
		err("movie_current read is fail(%d)", ret);
		goto p_err;
	}
#endif

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	device = &core->sensor[sensor_id];
	if (!device) {
		err("sensor device is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	sensor_peri = find_peri_by_flash_id(device, FLADRV_NAME_DRV_FLASH_I2C);
	if (!sensor_peri) {
		probe_info("sensor peri is not yet probed");
		return -EPROBE_DEFER;
	}

	flash = &sensor_peri->flash;
	if (!flash) {
		err("flash is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	subdev_flash = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_flash) {
		err("subdev_flash is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	sensor_peri->subdev_flash = subdev_flash;

	flash = &sensor_peri->flash;
	flash->id = FLADRV_NAME_DRV_FLASH_I2C;
	flash->id = sensor_id;
	flash->subdev = subdev_flash;
	flash->client = client;

	flash->flash_gpio = of_get_named_gpio(dnode, "flash-gpio", 0);
	if (!gpio_is_valid(flash->flash_gpio)) {
		dev_err(dev, "failed to get PIN_RESET\n");
		return -EINVAL;
	}

	gpio_request_one(flash->flash_gpio, GPIOF_OUT_INIT_LOW, "CAM_FLASH_GPIO_OUTPUT");

	flash->flash_data.mode = CAM2_FLASH_MODE_OFF;
	flash->flash_data.intensity = 100; /* TODO: Need to figure out min/max range */
	flash->flash_data.firing_time_us = 1 * 1000 * 1000; /* Max firing time is 1sec */

#ifdef CONFIG_VENDER_MCD
#ifdef FLASH_S2MPU06
	flash->flash_data.flash_current = S2MPU06_FLASH_CURRENT(flash_current);
	flash->flash_data.movie_current = S2MPU06_TORCH_CURRENT(movie_current);
	flash->flash_data.torch_current = S2MPU06_TORCH_CURRENT(torch_current);
	flash->flash_data.factory_current = S2MPU06_TORCH_CURRENT(factory_current);
#endif
#endif

	/* flash init for torch setting */
	ret = flash_torch_set_init(flash->client);
	if (ret < 0)
		warn("%s: Failt to torch init setting\n", __func__);

	/* init to sysfs for torch control */
	ret = sysfs_create_group(&core->pdev->dev.kobj, &fimc_is_flash_attr_group);
	if (ret < 0)
		warn("%s: Failt to torch sysfs init\n", __func__);

#ifdef CAMERA_SYSFS_V2
	led_dev = device_create(camera_class, NULL, 3, NULL, "flash");
	if (IS_ERR(led_dev)) {
		pr_err("<%s> Failed to create device(flash)!\n", __func__);
	} else {
	    dev_set_drvdata(led_dev, core);
		if (device_create_file(led_dev, &dev_attr_rear_flash) < 0) {
			pr_err("<%s> failed to create device file, %s\n",
			__func__ ,dev_attr_rear_torch_flash.attr.name);
		}
		if (device_create_file(led_dev, &dev_attr_rear_torch_flash) < 0) {
			pr_err("<%s> failed to create device file, %s\n",
			__func__ ,dev_attr_rear_torch_flash.attr.name);
		}
	}
#endif

#ifdef CONFIG_MUIC_NOTIFIER
	/* use to muic notifier for torch */
	muic_notifier_register(&flash->flash_noti_ta,
		flash_ta_notification,
		MUIC_NOTIFY_DEV_CHARGER);
#endif

	v4l2_i2c_subdev_init(subdev_flash, client, &subdev_ops);
	v4l2_set_subdevdata(subdev_flash, flash);
	v4l2_set_subdev_hostdata(subdev_flash, device);
	snprintf(subdev_flash->name, V4L2_SUBDEV_NAME_SIZE, "flash-subdev.%d", flash->id);

p_err:
	return ret;
}

static int flash_remove(struct i2c_client *client)
{
	int ret = 0;

#ifdef CAMERA_SYSFS_V2
	if (led_dev) {
		device_remove_file(led_dev, &dev_attr_rear_flash);
		device_remove_file(led_dev, &dev_attr_rear_torch_flash);
	}

	if (camera_class && led_dev) {
		device_destroy(camera_class, led_dev->devt);
	}
#endif

	info("%s\n", __func__);

	return ret;
}

int flash_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct device *dev;

	if (!client) {
		probe_info("client is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	dev = &client->dev;

	ret = flash_probe(dev, client);
	if (ret < 0) {
		probe_err("flash i2c probe fail(%d)\n", ret);
		goto p_err;
	}

	probe_info("%s done\n", __func__);

p_err:
    return ret;
}

static int flash_i2c_remove(struct i2c_client *client)
{
	int ret = 0;

	if (!client) {
		err("client is null");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

#ifdef CAMERA_SYSFS_V2
	flash_remove(client);
#endif

	info("%s\n", __func__);

p_err:
	return ret;
}

static const struct of_device_id exynos_fimc_is_sensor_flash_i2c_match[] = {
	{
		.compatible = "samsung,sensor-flash-i2c",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_flash_i2c_match);

/* register I2C driver */
static const struct i2c_device_id flash_i2c_idt[] = {
    { "I2C", 0 },
};

static struct i2c_driver sensor_flash_gpio_i2c_driver = {
	.driver = {
		.name   = "FIMC-IS-SENSOR-FLASH-I2C",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_flash_i2c_match
	},
	.probe  = flash_i2c_probe,
	.remove = flash_i2c_remove,
	.id_table = flash_i2c_idt
};
module_i2c_driver(sensor_flash_gpio_i2c_driver);
