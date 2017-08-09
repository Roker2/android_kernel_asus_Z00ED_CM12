/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/export.h>
#include "msm_camera_io_util.h"
#include "msm_led_flash.h"
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define FLASH_NAME "sky81296"

#define CONFIG_MSMB_CAMERA_DEBUG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define LM3642_DBG(fmt, args...) pr_err(fmt, ##args)
#else
#define LM3642_DBG(fmt, args...)
#endif


static struct msm_led_flash_ctrl_t fctrl;
static struct i2c_driver lm3642_i2c_driver;

static struct msm_camera_i2c_reg_array lm3642_init_array[] = {
	{0x05, 0x07},
	{0x06, 0x65},
	{0x02, 0x09},
};

static struct msm_camera_i2c_reg_array lm3642_off_array[] = {
	{0x05, 0x07},
	{0x06, 0x65},
	{0x02, 0x99},
 	{0x00, 0x0A},
  	{0x01, 0x0A},
  	{0x04, 0x22},
};

static struct msm_camera_i2c_reg_array lm3642_release_array[] = {
	{0x04, 0x00},
};

static struct msm_camera_i2c_reg_array lm3642_low_array[] = {
 	{0x03, 0x07},
  	{0x04, 0x01},
};

static struct msm_camera_i2c_reg_array lm3642_high_array[] = {
 	{0x00, 0x0A},
  	{0x04, 0x02},
};


static const struct of_device_id lm3642_i2c_trigger_dt_match[] = {
	{.compatible = "sky81296"},
	{}
};

MODULE_DEVICE_TABLE(of, lm3642_i2c_trigger_dt_match);
static const struct i2c_device_id lm3642_i2c_id[] = {
	{FLASH_NAME, (kernel_ulong_t)&fctrl},
	{ }
};

static void msm_led_torch_brightness_set(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	pr_info("%s:%d called\n", __func__, __LINE__);
	if (value > LED_OFF) {
		if(fctrl.func_tbl->flash_led_low)
			fctrl.func_tbl->flash_led_low(&fctrl);
	} else {
		if(fctrl.func_tbl->flash_led_off)
			fctrl.func_tbl->flash_led_off(&fctrl);
	}
};

static struct led_classdev msm_torch_led = {
	.name			= "torch-light",
	.brightness_set	= msm_led_torch_brightness_set,
	.brightness		= LED_OFF,
};

static int32_t msm_lm3642_torch_create_classdev(struct device *dev ,
				void *data)
{
	int rc;

	pr_info("%s:%d called\n", __func__, __LINE__);
	msm_led_torch_brightness_set(&msm_torch_led, LED_OFF);
	rc = led_classdev_register(dev, &msm_torch_led);
	if (rc) {
		pr_err("Failed to register led dev. rc = %d\n", rc);
		return rc;
	}

	return 0;
};

int msm_flash_lm3642_led_init(struct msm_led_flash_ctrl_t *fctrl)
{
	int rc = 0;
	struct msm_camera_sensor_board_info *flashdata = NULL;
	struct msm_camera_power_ctrl_t *power_info = NULL;
	pr_info("%s:%d called\n", __func__, __LINE__);

	flashdata = fctrl->flashdata;
	power_info = &flashdata->power_info;

	/*gpio_set_value_cansleep(
		power_info->gpio_conf->gpio_num_info->
		gpio_num[SENSOR_GPIO_FL_NOW],
		GPIO_OUT_LOW);*/

	if (fctrl->flash_i2c_client && fctrl->reg_setting) {
		rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_write_table(
			fctrl->flash_i2c_client,
			fctrl->reg_setting->init_setting);
		if (rc < 0)
			pr_err("%s:%d failed\n", __func__, __LINE__);
	}
	return rc;
}

int msm_flash_lm3642_led_release(struct msm_led_flash_ctrl_t *fctrl)
{
	int rc = 0;
	struct msm_camera_sensor_board_info *flashdata = NULL;
	struct msm_camera_power_ctrl_t *power_info = NULL;

	flashdata = fctrl->flashdata;
	power_info = &flashdata->power_info;
	LM3642_DBG("%s:%d called\n", __func__, __LINE__);
	if (!fctrl) {
		pr_err("%s:%d fctrl NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

/*	gpio_set_value_cansleep(
		power_info->gpio_conf->gpio_num_info->
		gpio_num[SENSOR_GPIO_FL_NOW],
		GPIO_OUT_LOW);*/
	if (fctrl->flash_i2c_client && fctrl->reg_setting) {
		rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_write_table(
			fctrl->flash_i2c_client,
			fctrl->reg_setting->release_setting);
		if (rc < 0)
			pr_err("%s:%d failed\n", __func__, __LINE__);
	}
	return 0;
}

int msm_flash_lm3642_led_off(struct msm_led_flash_ctrl_t *fctrl)
{
	int rc = 0;
	struct msm_camera_sensor_board_info *flashdata = NULL;
	struct msm_camera_power_ctrl_t *power_info = NULL;

	flashdata = fctrl->flashdata;
	power_info = &flashdata->power_info;
	pr_info("%s:%d called\n", __func__, __LINE__);

	if (!fctrl) {
		pr_err("%s:%d fctrl NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (fctrl->flash_i2c_client && fctrl->reg_setting) {
		rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_write_table(
			fctrl->flash_i2c_client,
			fctrl->reg_setting->off_setting);
		if (rc < 0)
			pr_err("%s:%d failed\n", __func__, __LINE__);
	}

	/*gpio_set_value_cansleep(
		power_info->gpio_conf->gpio_num_info->
		gpio_num[SENSOR_GPIO_FL_NOW],
		GPIO_OUT_LOW);*/

	return rc;
}

int msm_flash_lm3642_led_low(struct msm_led_flash_ctrl_t *fctrl)
{
	int rc = 0;
	struct msm_camera_sensor_board_info *flashdata = NULL;
	struct msm_camera_power_ctrl_t *power_info = NULL;
	pr_info("%s:%d called\n", __func__, __LINE__);

	flashdata = fctrl->flashdata;
	power_info = &flashdata->power_info;

/*
	gpio_set_value_cansleep(
		power_info->gpio_conf->gpio_num_info->
		gpio_num[SENSOR_GPIO_FL_NOW],
		GPIO_OUT_HIGH);
*/
	if (fctrl->flash_i2c_client && fctrl->reg_setting) {
		rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_write_table(
			fctrl->flash_i2c_client,
			fctrl->reg_setting->low_setting);
		if (rc < 0)
			pr_err("%s:%d failed\n", __func__, __LINE__);
	}

	return rc;
}

int msm_flash_lm3642_led_high(struct msm_led_flash_ctrl_t *fctrl)
{
	int rc = 0;
	struct msm_camera_sensor_board_info *flashdata = NULL;
	struct msm_camera_power_ctrl_t *power_info = NULL;
	pr_info("%s:%d called\n", __func__, __LINE__);

	flashdata = fctrl->flashdata;

	power_info = &flashdata->power_info;
/*
	gpio_set_value_cansleep(
		power_info->gpio_conf->gpio_num_info->
		gpio_num[SENSOR_GPIO_FL_NOW],
		GPIO_OUT_HIGH);
*/
	if (fctrl->flash_i2c_client && fctrl->reg_setting) {
		rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_write_table(
			fctrl->flash_i2c_client,
			fctrl->reg_setting->high_setting);
		if (rc < 0)
			pr_err("%s:%d failed\n", __func__, __LINE__);
	}

	return rc;
}

/*For FLASH STATUS Controll+++*/
#define	STATUS_PROC_FILE	"driver/flash_status"
static struct proc_dir_entry *status_proc_file;
static int ATD_status;

static int status_proc_read(struct seq_file *buf, void *v)
{
    seq_printf(buf, "%d\n", ATD_status);
	ATD_status = 0;
    return 0;
}

static int status_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, status_proc_read, NULL);
}


static const struct file_operations status_fops = {
	.owner = THIS_MODULE,
	.open = status_proc_open,
	.read = seq_read,
	//.write = status_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void create_status_proc_file(void)
{
    status_proc_file = proc_create(STATUS_PROC_FILE, 0666, NULL, &status_fops);
    if (status_proc_file) {
	printk("%s sucessed!\n", __func__);
    } else {
	printk("%s failed!\n", __func__);
    }
}
/*For FLASH STATUS Controll---*/
/*For ASUS FLASH+++*/
/*#define	ASUS_FLASH_PROC_FILE	"driver/asus_flash"
static struct proc_dir_entry *asus_flash_proc_file;
static int asus_flash_value;*/

static ssize_t asus_flash_show(struct file *dev, char *buffer, size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t asus_flash_store(struct file *dev, const char *buf, size_t count, loff_t *loff)
{
	int mode = -1, set_val = -1;
	int map_num = -1;


	sscanf(buf, "%d %d", &mode, &set_val);
	pr_info("[AsusFlash]flash mode=%d value=%d\n", mode, set_val);
	msm_flash_lm3642_led_init(&fctrl);

	if(mode == 0) {
		if (set_val < 0 || set_val > 250 || set_val == 1) {
			ATD_status = 1;
			msm_flash_lm3642_led_low(&fctrl);
		} else if (set_val == 0 ) {
			ATD_status = 1;
			msm_flash_lm3642_led_release(&fctrl);
		} else if(0 < set_val && set_val < 251) {
			printk(KERN_INFO "[AsusFlash] current now in 1~250");
		} else {
			msm_flash_lm3642_led_release(&fctrl);
			return -1;
		}
		printk(KERN_INFO "[AsusFlash] Real set torch current  to %d\n", map_num);
	} else if(mode == 1) {
		if (set_val == 1 || set_val < 0 || set_val > 1500) {
			ATD_status = 1;
			msm_flash_lm3642_led_high(&fctrl);
		} else if (set_val == 0) {
			ATD_status = 1;
			msm_flash_lm3642_led_release(&fctrl);
		} else if (0 < set_val && set_val < 1501) {
			printk(KERN_INFO "[AsusFlash] Flash current now in 1~1500");
		} else {
			msm_flash_lm3642_led_release(&fctrl);
			return -1;
		}
		printk(KERN_INFO "[AsusFlash] Real set flash current to %d\n", map_num);
		/*if (set_val2 == -1) {
			set_val2 = 1;
		}
		if (set_val2 < 0 || set_val2 > 1425 || set_val2 == 1) {
			msm_flash_lm3642_led_high(&fctrl);
		} else if (set_val2 == 0) {
			msm_flash_lm3642_led_off(&fctrl);
		} else if (0 < set_val2 && set_val2 < 1426) {
			printk(KERN_INFO "[AsusFlash] Flash time out now in 1~1425");
		} else {
			return -1;
		}
		printk(KERN_INFO "[AsusFlash] Real set flash time out to %d\n", map_num);*/
	} else {
		return -1;
	}

	return count;
}

static const struct file_operations asus_flash_proc_fops = {
	.read = asus_flash_show,
	.write = asus_flash_store,
};
/*For ASUS FLASH---*/

static int msm_flash_lm3642_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct msm_camera_sensor_board_info *flashdata = NULL;
	struct msm_camera_power_ctrl_t *power_info = NULL;
	/*For ASUS FLASH+++*/
	struct proc_dir_entry* proc_entry_flash;
	void* dummy = NULL;
	/*For ASUS FLASH---*/
	int rc = 0 ;
	pr_info("%s entry\n", __func__);
	ATD_status = 0;
	create_status_proc_file();
	if (!id) {
		pr_err("msm_flash_lm3642_i2c_probe: id is NULL");
		id = lm3642_i2c_id;
	}
	rc = msm_flash_i2c_probe(client, id);
	if (rc < 0) {
		pr_err("%s: msm_flash_i2c_probe failed\n", __func__);
		return rc;
	}

	flashdata = fctrl.flashdata;
	power_info = &flashdata->power_info;

	rc = msm_camera_request_gpio_table(
		power_info->gpio_conf->cam_gpio_req_tbl,
		power_info->gpio_conf->cam_gpio_req_tbl_size, 1);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		return rc;
	}

	/*gpio_set_value_cansleep(
		power_info->gpio_conf->gpio_num_info->
		gpio_num[SENSOR_GPIO_FL_EN],
		GPIO_OUT_HIGH);*/
	gpio_set_value_cansleep(
		power_info->gpio_conf->gpio_num_info->
		gpio_num[SENSOR_GPIO_FL_NOW],
		GPIO_OUT_HIGH);

	/*For ASUS FLASH+++*/
	proc_entry_flash = proc_create_data("driver/asus_flash", 0666, NULL, &asus_flash_proc_fops, dummy);
	proc_set_user(proc_entry_flash, 1000, 1000);
	/*For ASUS FLASH---*/

	if (fctrl.pinctrl_info.use_pinctrl == true) {
		pr_err("%s:%d PC:: flash pins setting to active state",
				__func__, __LINE__);
		rc = pinctrl_select_state(fctrl.pinctrl_info.pinctrl,
				fctrl.pinctrl_info.gpio_state_active);
		if (rc)
			pr_err("%s:%d cannot set pin to active state",
					__func__, __LINE__);
	}

	if (!rc)
		msm_lm3642_torch_create_classdev(&(client->dev),NULL);
	return rc;
}

static int msm_flash_lm3642_i2c_remove(struct i2c_client *client)
{
	struct msm_camera_sensor_board_info *flashdata = NULL;
	struct msm_camera_power_ctrl_t *power_info = NULL;
	int rc = 0 ;
	pr_info("%s entry\n", __func__);
	flashdata = fctrl.flashdata;
	power_info = &flashdata->power_info;

	rc = msm_camera_request_gpio_table(
		power_info->gpio_conf->cam_gpio_req_tbl,
		power_info->gpio_conf->cam_gpio_req_tbl_size, 0);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		return rc;
	}

	if (fctrl.pinctrl_info.use_pinctrl == true) {
		rc = pinctrl_select_state(fctrl.pinctrl_info.pinctrl,
				fctrl.pinctrl_info.gpio_state_suspend);
		if (rc)
			pr_err("%s:%d cannot set pin to suspend state",
				__func__, __LINE__);
	}
	return rc;
}


static struct i2c_driver lm3642_i2c_driver = {
	.id_table = lm3642_i2c_id,
	.probe  = msm_flash_lm3642_i2c_probe,
	.remove = msm_flash_lm3642_i2c_remove,
	.driver = {
		.name = FLASH_NAME,
		.owner = THIS_MODULE,
		.of_match_table = lm3642_i2c_trigger_dt_match,
	},
};

static int __init msm_flash_lm3642_init(void)
{
	pr_info("%s PJ 9 entry\n", __func__);
	return i2c_add_driver(&lm3642_i2c_driver);
}

static void __exit msm_flash_lm3642_exit(void)
{
	pr_info("%s entry\n", __func__);
	i2c_del_driver(&lm3642_i2c_driver);
	return;
}


static struct msm_camera_i2c_client lm3642_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_camera_i2c_reg_setting lm3642_init_setting = {
	.reg_setting = lm3642_init_array,
	.size = ARRAY_SIZE(lm3642_init_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_camera_i2c_reg_setting lm3642_off_setting = {
	.reg_setting = lm3642_off_array,
	.size = ARRAY_SIZE(lm3642_off_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_camera_i2c_reg_setting lm3642_release_setting = {
	.reg_setting = lm3642_release_array,
	.size = ARRAY_SIZE(lm3642_release_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_camera_i2c_reg_setting lm3642_low_setting = {
	.reg_setting = lm3642_low_array,
	.size = ARRAY_SIZE(lm3642_low_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_camera_i2c_reg_setting lm3642_high_setting = {
	.reg_setting = lm3642_high_array,
	.size = ARRAY_SIZE(lm3642_high_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_led_flash_reg_t lm3642_regs = {
	.init_setting = &lm3642_init_setting,
	.off_setting = &lm3642_off_setting,
	.low_setting = &lm3642_low_setting,
	.high_setting = &lm3642_high_setting,
	.release_setting = &lm3642_release_setting,
};

static struct msm_flash_fn_t lm3642_func_tbl = {
	.flash_get_subdev_id = msm_led_i2c_trigger_get_subdev_id,
	.flash_led_config = msm_led_i2c_trigger_config,
	.flash_led_init = msm_flash_lm3642_led_init,
	.flash_led_release = msm_flash_lm3642_led_release,
	.flash_led_off = msm_flash_lm3642_led_off,
	.flash_led_low = msm_flash_lm3642_led_low,
	.flash_led_high = msm_flash_lm3642_led_high,
};

static struct msm_led_flash_ctrl_t fctrl = {
	.flash_i2c_client = &lm3642_i2c_client,
	.reg_setting = &lm3642_regs,
	.func_tbl = &lm3642_func_tbl,
};

module_init(msm_flash_lm3642_init);
module_exit(msm_flash_lm3642_exit);
MODULE_DESCRIPTION("lm3642 FLASH");
MODULE_LICENSE("GPL v2");
