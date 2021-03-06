/* 
 * Copyright (C) 2014 ASUSTek Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/********************************/
/* IR Sensor CM36686 Module */
/******************************/
#define MODULE_NAME	"cm36686"
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include "../hardware.h"
#include "../../IRsensor.h"
#include "cm36686.h"

/************************************/
/*Global static variable and function*/
/***********************************/
static struct i2c_client	*g_i2c_client = NULL;
static int cm36686_IRsensor_hw_check(void);
static int cm36686_proximity_hw_set_config(void);
static int cm36686_light_hw_set_config(void);

static int cm36686_IRsensor_hw_init(struct i2c_client* client);
static int cm36686_IRsensor_hw_show_allreg(void);
static int cm36686_IRsensor_hw_get_interrupt(void);

static int cm36686_proximity_hw_turn_onoff(bool bOn);
static int cm36686_proximity_hw_get_adc(void);
static int cm36686_proximity_hw_set_hi_threshold(int hi_threshold);
static int cm36686_proximity_hw_set_lo_threshold(int low_threshold);
static int cm36686_proximity_hw_set_led_current(uint8_t led_current_reg);
static int cm36686_proximity_hw_set_led_duty_ratio(uint8_t led_duty_ratio_reg);
static int cm36686_proximity_hw_set_persistence(uint8_t persistence);
static int cm36686_proximity_hw_set_integration(uint8_t integration);

static int cm36686_light_hw_turn_onoff(bool bOn);
static int cm36686_light_hw_get_adc(void);
static int cm36686_light_hw_set_hi_threshold(int hi_threshold);
static int cm36686_light_hw_set_lo_threshold(int low_threshold);
static int cm36686_light_hw_set_persistence(uint8_t persistence);
static int cm36686_light_hw_set_integration(uint8_t integration);

/**************************/
/* i2c read/write register */
/*************************/
static int cm36686_IRsensor_hw_check(void)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	

	/* Check the Device ID */
	ret = i2c_read_reg_u16(g_i2c_client, ID_REG, data_buf);
	if ((ret < 0) || (data_buf[0] != 0x86)) {
		err("[i2c] IR Sensor Device ID ERROR. (ID_REG : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
		return -ENOMEM;
	}
	
	return 0;
}

static int cm36686_proximity_hw_set_config(void)
{
	int ret = 0;	
	ret = cm36686_proximity_hw_set_led_current(CM36686_LED_I_100);
	if (ret < 0)		
		return ret;

	ret = cm36686_proximity_hw_set_led_duty_ratio(CM36686_PS_DR_1_40);
	if (ret < 0)		
		return ret;

	ret = cm36686_proximity_hw_set_persistence(CM36686_PS_PERS_1);
	if (ret < 0)		
		return ret;

	ret = cm36686_proximity_hw_set_integration(CM36686_PS_IT_2_5T);
	if (ret < 0)		
		return ret;
		
	return 0;	
}

static int cm36686_light_hw_set_config(void)
{
	int ret = 0;	
	ret = cm36686_light_hw_set_persistence(CM36686_ALS_PERS_1);
	if (ret < 0)		
		return ret;

	ret = cm36686_light_hw_set_integration(CM36686_ALS_IT_160MS);
	if (ret < 0)		
		return ret;

	return 0;	
}

/*****************/
/*IR Sensor Part*/
/****************/
static int cm36686_IRsensor_hw_init(struct i2c_client* client)
{
	int ret = 0;

	g_i2c_client = client;
	
	/* Check the Device ID */
	ret = cm36686_IRsensor_hw_check();
	//if (ret < 0) {		
	//	return ret;
	//}	
	
	 /*Set Proximity config */
	ret =cm36686_proximity_hw_set_config();
	if (ret < 0) {		
		return ret;
	}

	 /*Set Light Sensor config*/
	ret =cm36686_light_hw_set_config();
	if (ret < 0) {		
		return ret;
	}
	
	return 0;
}

static int cm36686_IRsensor_hw_show_allreg(void)
{
	int ret = 0;	
	uint8_t buf[2] = {0};
	int reg = 0;
	for (; reg< CM36686_NUM_REGS; reg++)	
	{
		ret = i2c_read_reg_u16(g_i2c_client, reg, buf);
		log("Show All Register (0x%X) = 0x%02X%02X\n", reg, buf[1], buf[0]);
		if(ret < 0) 
		{
			err("IR Sensor show all Register ERROR. (REG:0x%X)\n", reg);
			return ret;
		}
	}
	return 0;
}

static int cm36686_IRsensor_hw_get_interrupt(void)
{
	uint8_t buf[2] = {0};
	bool check_flag = false;
	int irsensor_int = 0;
	
	/* Read INT_FLAG will clean the interrupt */
	i2c_read_reg_u16(g_i2c_client, INT_FLAG, buf);

	/*Proximity Sensor work */
	if (buf[1]&INT_FLAG_PS_IF_AWAY || buf[1]&INT_FLAG_PS_IF_CLOSE) 
	{
		check_flag =true;
		if (buf[1]&INT_FLAG_PS_IF_AWAY) {
			irsensor_int |= IRSENSOR_INT_PS_AWAY;	
		}else if (buf[1]&INT_FLAG_PS_IF_CLOSE) {		
			irsensor_int |= IRSENSOR_INT_PS_CLOSE;
		}else {
			err("Can NOT recognize the Proximity INT_FLAG (0x%02X%02X)\n", buf[1], buf[0]);
			return -1;
		}
	}

	/* Light Sensor work */
	if(buf[1]&INT_FLAG_ALS_IF_L || buf[1]&INT_FLAG_ALS_IF_H) 
	{	
		check_flag =true;
		irsensor_int |= IRSENSOR_INT_ALS;
	} 

	/* Interrupt Error */
	if(check_flag == false){		
		err("Can NOT recognize the INT_FLAG (0x%02X%02X)\n", buf[1], buf[0]);
		return -1;
	}

	return irsensor_int;
}

/****************/
/*Proximity Part*/
/****************/
static int cm36686_proximity_hw_turn_onoff(bool bOn)
{
	int ret = 0;	
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02X%02X) \n", power_state_data_buf[1], power_state_data_buf[0]);
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));
	
	if (bOn == 1)	{	/* power on */		
		power_state_data_buf[0] &= CM36686_PS_SD_MASK;
		power_state_data_buf[1] |= CM36686_PS_INT_IN_AND_OUT;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if (ret < 0) {
			err("Proximity power on ERROR (PS_CONF1) \n");
			return ret;
		} else {
			log("Proximity power on (PS_CONF1 : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	} else	{	/* power off */		
		power_state_data_buf[0] |= CM36686_PS_SD;
      	power_state_data_buf[1] &= CM36686_PS_INT_MASK;
		
		ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, power_state_data_buf);
		if (ret < 0) {
			err("Proximity power off ERROR (PS_CONF1) \n");
			return ret;
		} else {
			log("Proximity power off (PS_CONF1 : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}
	
	return 0;
}

static int cm36686_proximity_hw_get_adc(void)
{
	int ret = 0;
	int adc = 0;
	uint8_t adc_buf[2] = {0, 0};	

	ret = i2c_read_reg_u16(g_i2c_client, PS_DATA, adc_buf);
	if (ret < 0) {
		err("Proximity get adc ERROR. (PS_DATA)\n");
		return ret;
	}
	adc = (adc_buf[1] << 8) + adc_buf[0];
	dbg("Proximity get adc : 0x%02X%02X\n", adc_buf[1], adc_buf[0]); 
	
	return adc;
}

static int cm36686_proximity_hw_set_hi_threshold(int hi_threshold)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	
	/*Set Proximity High Threshold*/
	data_buf[0] = hi_threshold % (1<<8);
	data_buf[1] = hi_threshold /  (1<<8);
	ret = i2c_write_reg_u16(g_i2c_client, PS_THDH, data_buf);
	if(ret < 0) {
		err("Proximity write High Threshold ERROR. (PS_THDH : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    	log("Proximity write High Threshold (PS_THDH : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	}		

	return 0;
}

static int cm36686_proximity_hw_set_lo_threshold(int low_threshold)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	

	/*Set Proximity Low Threshold*/	
	data_buf[0] = low_threshold % (1<<8);
	data_buf[1] = low_threshold /  (1<<8);
	ret = i2c_write_reg_u16(g_i2c_client, PS_THDL, data_buf);
	if(ret < 0) {
		err("Proximity write Low Threshold ERROR. (PS_THDL : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    	log("Proximity write Low Threshold (PS_THDL : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	}

	return 0;
}

static int cm36686_proximity_hw_set_led_current(uint8_t led_current_reg)
{
	int ret = 0;
	uint8_t data_buf[2] = {0, 0};	
	uint8_t data_origin[2] = {0, 0};	

	/* set LED config */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF3, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF3 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF3 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	data_origin[0] = data_buf[0];
	data_origin[1] = data_buf[1];

	led_current_reg <<= CM36686_LED_I_SHIFT;
	data_buf[1] &= CM36686_LED_I_MASK;
	data_buf[1] |= led_current_reg;
		
	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF3, data_buf);
	if (ret < 0) {
		err("Proximity set LED Current (PS_CONF3) ERROR \n");
		return ret;
	} else {
		log("Proximity set LED Current (PS_CONF3 : 0x%X -> 0x%X) \n", 
			data_origin[1], data_buf[1]);
	}

	return 0;
}

static int cm36686_proximity_hw_set_led_duty_ratio(uint8_t led_duty_ratio_reg)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity LED Duty Ratio */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, strlen(data_buf));

	led_duty_ratio_reg <<= CM36686_PS_DR_SHIFT;
	data_buf[0] &= CM36686_PS_DR_MASK;
	data_buf[0] |= led_duty_ratio_reg;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity set LED Duty Ratio (PS_CONF1) ERROR \n");
		return ret;
	} else {
		log("Proximity set LED Duty Ratio (PS_CONF1 : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;	
}

static int cm36686_proximity_hw_set_persistence(uint8_t persistence)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Persistence */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, strlen(data_buf));

	persistence <<= CM36686_PS_PERS_SHIFT;
	data_buf[0] &= CM36686_PS_PERS_MASK;
	data_buf[0] |= persistence;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity set Persistence (PS_CONF1) ERROR \n");
		return ret;
	} else {
		log("Proximity set Persistence (PS_CONF1 : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int cm36686_proximity_hw_set_integration(uint8_t integration)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Proximity Integration */
	ret = i2c_read_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity read PS_CONF1 ERROR\n");
		return ret;
	}
	dbg("Proximity read PS_CONF1 (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, strlen(data_buf));

	integration <<= CM36686_PS_IT_SHIFT;
	data_buf[0] &= CM36686_PS_IT_MASK;
	data_buf[0] |= integration;

	ret = i2c_write_reg_u16(g_i2c_client, PS_CONF1, data_buf);
	if (ret < 0) {
		err("Proximity set Integration (PS_CONF1) ERROR \n");
		return ret;
	} else {
		log("Proximity set Integration (PS_CONF1 : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

/*********************/
/* Light Sensor Part */
/********************/
static int cm36686_light_hw_turn_onoff(bool bOn)
{
	int ret = 0;	
	uint8_t power_state_data_buf[2] = {0, 0};
	uint8_t power_state_data_origin[2] = {0, 0};

	/* read power status */
	ret = i2c_read_reg_u16(g_i2c_client, ALS_CONF, power_state_data_buf);	
	if (ret < 0) {
		err("Light Sensor read ALS_CONF ERROR\n");
		return ret;
	}
	dbg("Light Sensor read ALS_CONF (0x%02X%02X) \n", power_state_data_buf[1], power_state_data_buf[0]);
	
	memcpy(power_state_data_origin, power_state_data_buf, sizeof(power_state_data_buf));

	if (bOn == 1)	{	/* power on */			
		power_state_data_buf[0] &= CM36686_ALS_SD_MASK;
		power_state_data_buf[0] |= CM36686_ALS_INT_EN;

		ret = i2c_write_reg_u16(g_i2c_client, ALS_CONF, power_state_data_buf);
		if (ret < 0) {
			err("Light Sensor power on ERROR (ALS_CONF) \n");
			return ret;
		} else {
			log("Light Sensor power on (ALS_CONF : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}		
	} else	{	/* power off */	
		power_state_data_buf[0] |= CM36686_ALS_SD;
      	power_state_data_buf[0] &= CM36686_ALS_INT_MASK;
		
		ret = i2c_write_reg_u16(g_i2c_client, ALS_CONF, power_state_data_buf);
		if (ret < 0) {
			err("Light Sensor power off ERROR (ALS_CONF) \n");
			return ret;
		} else {
			log("Light Sensor power off (ALS_CONF : 0x%X -> 0x%X) \n", 
				power_state_data_origin[0], power_state_data_buf[0]);
		}
	}	

	return 0;
}

static int cm36686_light_hw_get_adc(void)
{
	int ret = 0;
	int adc = 0;
	uint8_t adc_buf[2] = {0, 0};	
	
	ret = i2c_read_reg_u16(g_i2c_client, ALS_DATA, adc_buf);
	if (ret < 0) {
		err("Light Sensor Get adc ERROR (ALS_DATA)\n");
		return ret;
	}
	adc = (adc_buf[1] << 8) + adc_buf[0];
	dbg("Light Sensor Get adc : 0x%02X%02X\n", adc_buf[1], adc_buf[0]); 
	
	return adc;
}

static int cm36686_light_hw_set_hi_threshold(int hi_threshold)
{
	int ret = 0;	
	uint8_t data_buf[2] = {0, 0};	

	/*Set Light Sensor High Threshold*/	
	data_buf[0] = hi_threshold % (1<<8);
	data_buf[1] = hi_threshold /  (1<<8);
	ret = i2c_write_reg_u16(g_i2c_client, ALS_THDH, data_buf);
	if(ret < 0) {
		err("[i2c] Light Sensor write High Threshold ERROR. (ALS_THDH : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    	dbg("[i2c] Light Sensor write High Threshold (ALS_THDH : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	}		

	return 0;
}

static int cm36686_light_hw_set_lo_threshold(int low_threshold)
{
	int ret = 0;	
	uint8_t data_buf[2] = {0, 0};	

	/*Set Light Sensor Low Threshold*/		
	data_buf[0] = low_threshold % (1<<8);
	data_buf[1] = low_threshold /  (1<<8);
	ret = i2c_write_reg_u16(g_i2c_client, ALS_THDL, data_buf);
	if(ret < 0) {
		err("[i2c] Light Sensor write Low Threshold ERROR. (ALS_THDL : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	    	return ret;
	} else {
	    	dbg("[i2c] Light Sensor write Low Threshold (ALS_THDL : 0x%02X%02X)\n", data_buf[1], data_buf[0]);
	}

	return 0;
}

static int cm36686_light_hw_set_persistence(uint8_t persistence)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Persistence */
	ret = i2c_read_reg_u16(g_i2c_client, ALS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor read ALS_CONF ERROR\n");
		return ret;
	}
	dbg("Light Sensor read ALS_CONF (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, strlen(data_buf));

	persistence <<= CM36686_ALS_PERS_SHIFT;
	data_buf[0] &= CM36686_ALS_PERS_MASK;
	data_buf[0] |= persistence;

	ret = i2c_write_reg_u16(g_i2c_client, ALS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor set Persistence (ALS_CONF) ERROR \n");
		return ret;
	} else {
		log("Light Sensor set Persistence (ALS_CONF : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;
}

static int cm36686_light_hw_set_integration(uint8_t integration)
{
	int ret = 0;	
	uint8_t data_origin[2] = {0, 0};
	uint8_t data_buf[2] = {0, 0};	

	/* set Light Sensor Integration */
	ret = i2c_read_reg_u16(g_i2c_client, ALS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor read ALS_CONF ERROR\n");
		return ret;
	}
	dbg("Light Sensor read ALS_CONF (0x%02X%02X) \n", data_buf[1], data_buf[0]);
	memcpy(data_origin, data_buf, strlen(data_buf));

	integration <<= CM36686_ALS_IT_SHIFT;
	data_buf[0] &= CM36686_ALS_IT_MASK;
	data_buf[0] |= integration;

	ret = i2c_write_reg_u16(g_i2c_client, ALS_CONF, data_buf);
	if (ret < 0) {
		err("Light Sensor set Integration (ALS_CONF) ERROR \n");
		return ret;
	} else {
		log("Light Sensor set Integration (ALS_CONF : 0x%X -> 0x%X) \n", 
			data_origin[0], data_buf[0]);
	}

	return 0;	
}

static struct IRsensor_hw IRsensor_hw_cm36686 = {	
	.IRsensor_hw_init = cm36686_IRsensor_hw_init,
	.IRsensor_hw_show_allreg = cm36686_IRsensor_hw_show_allreg,
	.IRsensor_hw_get_interrupt = cm36686_IRsensor_hw_get_interrupt,
	
	.proximity_hw_turn_onoff = cm36686_proximity_hw_turn_onoff,
	.proximity_hw_get_adc = cm36686_proximity_hw_get_adc,
	.proximity_hw_set_hi_threshold = cm36686_proximity_hw_set_hi_threshold,
	.proximity_hw_set_lo_threshold = cm36686_proximity_hw_set_lo_threshold,
	.proximity_hw_set_led_current = cm36686_proximity_hw_set_led_current,
	.proximity_hw_set_led_duty_ratio = cm36686_proximity_hw_set_led_duty_ratio,
	.proximity_hw_set_persistence = cm36686_proximity_hw_set_persistence,
	.proximity_hw_set_integration = cm36686_proximity_hw_set_integration,

	.light_hw_turn_onoff = cm36686_light_hw_turn_onoff,
	.light_hw_get_adc = cm36686_light_hw_get_adc,
	.light_hw_set_hi_threshold = cm36686_light_hw_set_hi_threshold,
	.light_hw_set_lo_threshold = cm36686_light_hw_set_lo_threshold,
	.light_hw_set_persistence = cm36686_light_hw_set_persistence,
	.light_hw_set_integration = cm36686_light_hw_set_integration,

	.proximity_low_threshold_default = CM36686_PROXIMITY_THDL_DEFAULT,
	.proximity_hi_threshold_default = CM36686_PROXIMITY_THDH_DEFAULT,
	.proximity_max_led_current = CM36686_LED_I_MAX,
	.proximity_max_led_duty_ratio = CM36686_PS_DR_MAX,
	.proximity_max_persistence = CM36686_PS_PERS_MAX,
	.proximity_max_integration = CM36686_PS_IT_MAX,
	
	.light_max_threshold = CM36686_LIGHT_MAX_THRESHOLD,
	.light_200lux_default = CM36686_LIGHT_200LUX_DEFAULT,
	.light_1000lux_default = CM36686_LIGHT_1000LUX_DEFAULT,
	.light_max_persistence = CM36686_ALS_PERS_MAX,
	.light_max_integration = CM36686_ALS_IT_MAX,
	
	.vendor = "Capella",
	.module_number = "cm36686",
};

IRsensor_hw* IRsensor_hw_cm36686_getHardware(void)
{
	IRsensor_hw* IRsensor_hw_client = NULL;
	IRsensor_hw_client = &IRsensor_hw_cm36686;
	return IRsensor_hw_client;
}
