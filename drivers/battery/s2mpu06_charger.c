/* drivers/battery/s2mpu06_charger.c
 * S2MPU06 Charger Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/mfd/samsung/s2mpu06.h>
#include <linux/power/s2mpu06_charger.h>
#include <linux/version.h>

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif

#define ENABLE 1
#define DISABLE 0

#define ENABLE_IVR 1

#define EN_OVP_IRQ 1
#define EN_IEOC_IRQ 1
#define EN_TOPOFF_IRQ 1
#define EN_RECHG_REQ_IRQ 0
#define EN_TR_IRQ 0
#define EN_MIVR_SW_REGULATION 0
#define EN_BST_IRQ 0
#define MINVAL(a, b) ((a <= b) ? a : b)

#define EOC_DEBOUNCE_CNT 2
#define HEALTH_DEBOUNCE_CNT 3
#define DEFAULT_CHARGING_CURRENT 500

#define EOC_SLEEP 200
#define EOC_TIMEOUT (EOC_SLEEP * 6)
#ifndef EN_TEST_READ
#define EN_TEST_READ 1
#endif

struct s2mpu06_charger_data {
	struct i2c_client       *client;
	struct device *dev;
	struct s2mpu06_platform_data *s2mpu06_pdata;
	struct s2mpu06_dev *s2mpu06;
	struct delayed_work	charger_work;
	struct workqueue_struct *charger_wqueue;
	struct power_supply	psy_chg;
	struct power_supply	psy_battery;
	struct power_supply	psy_usb;
	struct power_supply	psy_ac;
	s2mpu06_charger_platform_data_t *pdata;
	int dev_id;
	int input_current;
	int charging_current;
	int topoff_current;
	int cable_type;
	int battery_cable_type;
	bool is_charging;
	struct mutex io_lock;
	bool noti_check;

	/* register programming */
	int reg_addr;
	int reg_data;

	bool full_charged;
	bool ovp;
	int unhealth_cnt;
	bool battery_valid;
	int status;
	int health;
	int charge_mode;

	/* s2mpu06 */
	int irq_det_bat;
	int irq_chg;
	int irq_ovp;
	struct delayed_work polling_work;
#if defined(CONFIG_FUELGAUGE_S2MPU06)
	int voltage_now;
	int voltage_avg;
	int voltage_ocv;
	unsigned int capacity;
#endif
#if defined(CONFIG_MUIC_NOTIFIER)
	struct notifier_block cable_check;
#endif
};

static enum power_supply_property s2mpu06_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
};

static int s2mpu06_get_charging_health(struct s2mpu06_charger_data *charger);
static void s2mpu06_enable_charger_switch(struct s2mpu06_charger_data *charger,
		bool enable);

static void s2mpu06_test_read(struct i2c_client *i2c)
{
	u8 data;
	char str[1016] = {0,};
	int i;

	for (i = 0x0; i <= 0x23; i++) {
		s2mpu06_read_reg(i2c, i, &data);

		sprintf(str+strlen(str), "0x%02x:0x%02x, ", i, data);
	}

	pr_err("%s: %s\n", __func__, str);
}

static void s2mpu06_chg_check(struct s2mpu06_charger_data *charger)
{
	union power_supply_propval val;
	unsigned int soc;
	u8 temp;

	pr_info("%s: start\n", __func__);
	val.intval = SEC_FUELGAUGE_CAPACITY_TYPE_RAW;
	psy_do_property(charger->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CAPACITY, val);
	soc = val.intval / 100;
	pr_info("%s: soc %d\n", __func__, soc);
	s2mpu06_read_reg(charger->s2mpu06->i2c, 0x18, &temp);

	if (soc >= 90 && soc <= 92) {
		s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL12, 0x08, 0x0C);
		s2mpu06_write_reg(charger->client, 0x43, 0x34);
		msleep(10);
		s2mpu06_write_reg(charger->client, 0x43, 0x24);
		s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL12, 0x0C, 0x0C);
		pr_info("%s: enable %d\n", __func__, soc);
	}

	pr_info("%s: end, soc=%d, monitor? = 0x%2x\n", __func__, soc, temp);

}

static void s2mpu06_charger_otg_control(struct s2mpu06_charger_data *charger,
		bool enable)
{
	if (enable) {
		/* set mode to OTG */
		if (s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL3,
					CHG_MODE_OTG_BOOST << CHG_MODE_SHIFT,
					CHG_MODE_MASK))
			pr_err("%s: error updating charger ctrl3\n", __func__);

		/* set boost frequency to 1MHz */
		/* set OTG current limit to 0.5 A */
		/* VBUS switches are OFF when OTG over-current happen */
		/* set OTG voltage to 5.1 V */

		/* turn on OTG */
		if (s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL14,
					OTG_EN_MASK, OTG_EN_MASK))
			pr_err("%s: error updating charger ctrl14\n", __func__);

		pr_info("%s : Turn on OTG\n",	__func__);
	} else {
		/* turn off OTG */
		if (s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL14,
					0x00, OTG_EN_MASK))
			pr_err("%s: error updating charger ctrl14\n", __func__);

		/* set mode to BUCK mode */
		if (s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL3,
					CHG_MODE_EN << CHG_MODE_SHIFT,
					CHG_MODE_MASK))
			pr_err("%s: error updating charger ctrl3\n", __func__);

		pr_info("%s : Turn off OTG\n",	__func__);
	}
}

static void s2mpu06_enable_charger_switch(struct s2mpu06_charger_data *charger,
		bool enable)
{
	if (enable) {
		pr_err("[DEBUG]%s: turn on charger\n", __func__);
		if (s2mpu06_update_reg(charger->client,
					S2MPU06_CHG_REG_CTRL3, CHG_MODE_EN << CHG_MODE_SHIFT,
										CHG_MODE_MASK))
			pr_err("%s: error updating charger ctrl3\n", __func__);
		msleep(200); /* 200ms delay for prevent fake UVLO */
	} else {
		charger->full_charged = false;
		charger->ovp = false;
		pr_err("[DEBUG] %s: turn off charger\n", __func__);
		if (s2mpu06_update_reg(charger->client,
					S2MPU06_CHG_REG_CTRL3, CHG_MODE_OFF << CHG_MODE_SHIFT,
										CHG_MODE_MASK))
			pr_err("%s: error updating charger ctrl3\n", __func__);
	}
}

static void s2mpu06_set_regulation_voltage(struct s2mpu06_charger_data *charger,
		int float_voltage)
{
	u8 data;

	pr_err("[DEBUG]%s: float_voltage %d\n", __func__, float_voltage);
	if (float_voltage <= 3600)
		data = 0x60;
	else if (float_voltage > 3600 && float_voltage <= 4450)
		data = (float_voltage - 3600) / 10 +  0x60;
	else
		data = 0xB5;

	if (s2mpu06_write_reg(charger->client, S2MPU06_CHG_REG_CTRL2, data))
		pr_err("%s: error writing charger ctrl2\n", __func__);
}

static void s2mpu06_set_input_current_limit(
		struct s2mpu06_charger_data *charger, int charging_current)
{
	u8 data;

	pr_err("%s: input current limit %d\n", __func__,
						 charging_current);
	if (charging_current <= 100)
		data = 0x0A;
	else if (charging_current > 100 && charging_current <= 1800)
		data = charging_current / 10;
	else
		data = 0xB4;

	if (s2mpu06_write_reg(charger->client, S2MPU06_CHG_REG_CTRL1, data))
		pr_err("%s: error writing charger ctrl1\n", __func__);
}

static int s2mpu06_get_input_current_limit(struct i2c_client *i2c)
{
	u8 data;

	if (s2mpu06_read_reg(i2c, S2MPU06_CHG_REG_CTRL1, &data)) {
		pr_err("%s: error reading charger ctrl1\n", __func__);
		return 0;
	}

	if (data > 0xB4)
		data = 0xB4;
	return (data * 10);
}

static void s2mpu06_set_charging_current(struct i2c_client *i2c,
		int charging_current)
{
	u8 data;

	pr_err("%s: current %d\n", __func__, charging_current);
	if (charging_current <= 240)
		data = 0x08;
	else if (charging_current > 240 && charging_current <= 2000)
		data = (charging_current - 240) / 20 + 0x08;
	else
		data = 0x60;

	if (s2mpu06_update_reg(i2c, S2MPU06_CHG_REG_CTRL10,
				data << FAST_CHARGING_CURRENT_SHIFT,
				FAST_CHARGING_CURRENT_MASK))
		pr_err("%s: error updating charger ctrl10\n", __func__);
}

static int s2mpu06_get_charging_current(struct i2c_client *i2c)
{
	u8 data;

	if (s2mpu06_read_reg(i2c, S2MPU06_CHG_REG_CTRL10, &data)) {
		pr_err("%s: error reading charger ctrl10\n", __func__);
		return 0;
	}

	data = data & FAST_CHARGING_CURRENT_MASK;

	if (data > 0x60)
		data = 0x60;
	return ((data - 0x08) * 20 + 240);
}

static int s2mpu06_get_topoff_current(struct s2mpu06_charger_data *charger)
{
	u8 data;

	if (s2mpu06_read_reg(charger->client, S2MPU06_CHG_REG_CTRL9, &data)) {
		pr_err("%s: error reading charger ctrl9\n", __func__);
		return 0;
	}

	data = data & FIRST_TOPOFF_CURRENT_MASK;

	if (data > 0x3C)
		data = 0x3C;
	return (data - 1) * 10 + 90;
}

static void s2mpu06_set_topoff_current(struct i2c_client *i2c,
		int eoc_1st_2nd, int current_limit)
{
	u8 data;

	pr_err("%s: current  %d\n", __func__, current_limit);
	if (current_limit <= 100)
		data = 0x02;
	else if (current_limit > 100 && current_limit <= 500)
		data = (current_limit - 100) / 10 + 0x02;
	else
		data = 0x2A;

	switch (eoc_1st_2nd) {
	case 1:
		if (s2mpu06_update_reg(i2c, S2MPU06_CHG_REG_CTRL9,
					data << FIRST_TOPOFF_CURRENT_SHIFT,
					FIRST_TOPOFF_CURRENT_MASK))
			pr_err("%s: error updating charger ctrl9\n", __func__);
		break;
	case 2:
		break;
	default:
		break;
	}
}

#if ENABLE_IVR
/* charger input regulation voltage setting */
static void s2mpu06_set_ivr_level(struct s2mpu06_charger_data *charger)
{
	u8 ivr = S2MPU06_IVR_4400MV;

	if (s2mpu06_write_reg(charger->client, S2MPU06_CHG_REG_CTRL5, ivr))
		pr_err("%s: error writing charger ctrl5\n", __func__);
}
#endif /* ENABLE_IVR */

#define S2MPU06_MRSTB_CTRL 0x2C
static bool s2mpu06_chg_init(struct s2mpu06_charger_data *charger)
{
	/* Buck switching mode frequency setting */

	/* Disable Timer function (Charging timeout fault) */
	if (s2mpu06_update_reg(charger->client,
				S2MPU06_CHG_REG_CTRL4, 0x00, 0x01))
		pr_err("%s: error for timer disable for aging test\n", __func__);

	/* to be */

	/* Disable TE */
	/* to be */

	/* MUST set correct regulation voltage first
	 * Before MUIC pass cable type information to charger
	 * charger would be already enabled (default setting)
	 * it might cause EOC event by incorrect regulation voltage */
	/* to be */

#if !(ENABLE_IVR)
	/* voltage regulatio disable does not exist mu005 */
#endif
	s2mpu06_set_ivr_level(charger);
	/* TOP-OFF debounce time set 256us */
	/* only 003 ? need to check */

	/* Disable (set 0min TOP OFF Timer) */

	/* EN_BST_ST_DN disable */
	if (s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL8,
					0, EN_BST_ST_DN_MASK))
		pr_err("%s: error updating charger ctrl8\n", __func__);

	if (s2mpu06_update_reg(charger->client,
				S2MPU06_CHG_REG_CTRL16, 0x00, 0x40))
		pr_err("%s: error for updating charging Protection bit\n", __func__);

	/* OTG Current Limit : 900mA */
	s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL13,
			0x2, 0x3);

	/* Top-Off Timer 30 min */
	s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL20,
			0x80, 0xE0);

	/* Disable Internal Timer */
	s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL4,
			0x0, 0x1);

	/* Termination Disable */
	s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL3,
			0x0, 0x40);

	/* Auto-Recharging Disable & WatchdogTimer Enable */
	s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL12,
			0x8C, 0x8C);

	/* protection for early CV */
	if (s2mpu06_write_reg(charger->client, 0x50, 0x84))
		pr_err("%s: error protecting early CV 0x50\n", __func__);

	if (s2mpu06_write_reg(charger->client, 0x3C, 0x0D))
		pr_err("%s: error protecting early CV 0x3C\n", __func__);

	if (s2mpu06_write_reg(charger->client, 0x4B, 0xFA))
		pr_err("%s: error protecting early CV 0x4B\n", __func__);

	/* check CV mode */
	s2mpu06_write_reg(charger->s2mpu06->i2c, 0x18, 0x44);
	s2mpu06_write_reg(charger->client, 0x80, 0x7A);

	/* SOC sel 80%->90% */
	s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL3, 0x08, 0x08);

	return true;
}

static int s2mpu06_get_charging_status(struct s2mpu06_charger_data *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 chg_sts, chg_sts2;
	union power_supply_propval chg_mode;

	if (s2mpu06_read_reg(charger->client, S2MPU06_CHG_REG_STATUS1, &chg_sts)) {
		pr_err("%s: error reading charger status1\n", __func__);
		return POWER_SUPPLY_STATUS_UNKNOWN;
	}

	if (s2mpu06_read_reg(charger->client, S2MPU06_CHG_REG_STATUS2, &chg_sts2)) {
		pr_err("%s: error reading charger status2\n", __func__);
		return POWER_SUPPLY_STATUS_UNKNOWN;
	}

	psy_do_property("battery", get, POWER_SUPPLY_PROP_CHARGE_NOW, chg_mode);

	if ((chg_sts & 0x20) == 0 && (chg_sts2 & 0x20) == 0)
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if (chg_sts2 & 0x20)
		status = POWER_SUPPLY_STATUS_FULL;
	else if (chg_sts & 0x20)
		status = POWER_SUPPLY_STATUS_CHARGING;
	else if (!(chg_sts & 0x02))
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;

	return status;
}

static int s2mpu06_get_charge_type(struct i2c_client *i2c)
{
	int status = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
	u8 ret;

	if (s2mpu06_read_reg(i2c, S2MPU06_CHG_REG_STATUS1, &ret)) {
		pr_err("%s: error reading charger status1\n", __func__);
		return POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
	}

	switch (ret & 0x60) {
	case 0x60:
		status = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	default:
		/* pre-charge mode */
		status = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
		break;
	}

	return status;
}

static bool s2mpu06_get_batt_present(struct i2c_client *i2c)
{
	u8 ret;

	if (s2mpu06_read_reg(i2c, S2MPU06_CHG_REG_STATUS2, &ret)) {
		pr_err("%s: error reading charger status2\n", __func__);
		return false;
	}

	return (ret & DET_BAT_STATUS_MASK) ? false : true;
}

static int s2mpu06_get_charging_health(struct s2mpu06_charger_data *charger)
{
	u8 ret;

	s2mpu06_test_read(charger->client);
	s2mpu06_chg_check(charger);

	/* clear watchdog */
	s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL12,
			   0x01, 0x03);

	s2mpu06_update_reg(charger->client, S2MPU06_CHG_REG_CTRL12,
			   0x00, 0x03);

	if (s2mpu06_read_reg(charger->client, S2MPU06_CHG_REG_STATUS1, &ret)) {
		pr_err("%s: error reading charger status1\n", __func__);
		return POWER_SUPPLY_HEALTH_UNKNOWN;
	}

	pr_info("%s: read charger status 1 (0x%x)\n", __func__, ret);
	if (ret & (1 << CHG_STATUS1_CHGVIN_STS)) {
		charger->ovp = false;
		return POWER_SUPPLY_HEALTH_GOOD;
	}

	charger->unhealth_cnt = HEALTH_DEBOUNCE_CNT;
	if (charger->ovp)
		return POWER_SUPPLY_HEALTH_OVERVOLTAGE;

	return POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
}

static int sec_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	int chg_curr, aicr;
	struct s2mpu06_charger_data *charger =
		container_of(psy, struct s2mpu06_charger_data, psy_chg);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->charging_current ? 1 : 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s2mpu06_get_charging_status(charger);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s2mpu06_get_charging_health(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = s2mpu06_get_input_current_limit(charger->client);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->charging_current) {
			aicr = s2mpu06_get_input_current_limit(charger->client);
			chg_curr =
			s2mpu06_get_charging_current(charger->client);
			val->intval = MINVAL(aicr, chg_curr);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		val->intval = s2mpu06_get_topoff_current(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = s2mpu06_get_charge_type(charger->client);
		break;
#if defined(CONFIG_BATTERY_SWELLING) || \
		defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = charger->pdata->chg_float_voltage;
		break;
#endif
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = s2mpu06_get_batt_present(charger->client);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = charger->is_charging;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct s2mpu06_charger_data *charger =
		container_of(psy, struct s2mpu06_charger_data, psy_chg);

	/* int eoc;
	int previous_cable_type = charger->cable_type; */

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;
		/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		charger->input_current =
			charger->pdata->charging_current[charger->cable_type].input_current_limit;
		pr_info("%s:[BATT] cable_type(%d), input_current(%d)mA\n",
				__func__, charger->cable_type, charger->input_current);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		s2mpu06_set_input_current_limit(charger,
				min(charger->input_current, val->intval));
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		pr_info("%s: is_charging %d\n", __func__, charger->is_charging);
		charger->charging_current = val->intval;
		/* set charging current */
		if (charger->is_charging)
			s2mpu06_set_charging_current(charger->client,
				charger->charging_current);
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		charger->topoff_current = val->intval;
		s2mpu06_set_topoff_current(charger->client, 1, val->intval);
		s2mpu06_test_read(charger->client);
		break;
#if defined(CONFIG_BATTERY_SWELLING) || \
			defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		pr_info("%s: float voltage(%d)\n", __func__, val->intval);
		charger->pdata->chg_float_voltage = val->intval;
		s2mpu06_set_regulation_voltage(charger,
				charger->pdata->chg_float_voltage);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		s2mpu06_charger_otg_control(charger, val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->charge_mode = val->intval;
		pr_err("%s: Set Charge Mode :%d\n", __func__, charger->charge_mode);
		switch (charger->charge_mode) {
			case SEC_BAT_CHG_MODE_BUCK_OFF:
			case SEC_BAT_CHG_MODE_CHARGING_OFF:
				charger->is_charging = false;
				break;
			case SEC_BAT_CHG_MODE_CHARGING:
				charger->is_charging = true;
				break;
		}
		s2mpu06_enable_charger_switch(charger, charger->is_charging);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_CHARGER_SYSFS
ssize_t s2mu003_chg_show_attrs(struct device *dev,
		const ptrdiff_t offset, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct s2mu003_charger_data *charger =
		container_of(psy, struct s2mu003_charger_data, psy_chg);
	int i = 0;
	char *str = NULL;

	switch (offset) {
	case CHG_REG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
				charger->reg_addr);
		break;
	case CHG_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
				charger->reg_data);
		break;
	case CHG_REGS:
		str = kzalloc(sizeof(char) * 256, GFP_KERNEL);
		if (!str)
			return -ENOMEM;

	/*	s2mpu06_read_regs(charger->client, str); */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
				str);

		kfree(str);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t s2mu003_chg_store_attrs(struct device *dev,
		const ptrdiff_t offset,
		const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct s2mu003_charger_data *charger =
		container_of(psy, struct s2mu003_charger_data, psy_chg);

	int ret = 0;
	int x = 0;
	uint8_t data = 0;

	switch (offset) {
	case CHG_REG:
		if (sscanf(buf, "%x\n", &x) == 1) {
			charger->reg_addr = x;
			data = s2mu003_reg_read(charger->client,
					charger->reg_addr);
			charger->reg_data = data;
			dev_dbg(dev, "%s: (read) addr = 0x%x, data = 0x%x\n",
			__func__, charger->reg_addr, charger->reg_data);
			ret = count;
		}
		break;
	case CHG_DATA:
		if (sscanf(buf, "%x\n", &x) == 1) {
			data = (u8)x;

			dev_dbg(dev, "%s: (write) addr = 0x%x, data = 0x%x\n",
					__func__, charger->reg_addr, data);
			ret = s2mu003_reg_write(charger->client,
					charger->reg_addr, data);
			if (ret < 0) {
				dev_dbg(dev, "I2C write fail Reg0x%x = 0x%x\n",
					(int)charger->reg_addr, (int)data);
			}
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
#endif

/* s2mpu06 interrupt service routine */
static irqreturn_t s2mpu06_det_bat_isr(int irq, void *data)
{
	struct s2mpu06_charger_data *charger = data;
	u8 val;

	if (s2mpu06_read_reg(charger->client, S2MPU06_CHG_REG_STATUS2, &val)) {
		pr_err("%s: error reading charger status2\n", __func__);
		return IRQ_HANDLED;
	}

	if ((val & DET_BAT_STATUS_MASK) == 0) {
		s2mpu06_enable_charger_switch(charger, 0);
		pr_err("charger-off if battery removed\n");
	}
	return IRQ_HANDLED;
}
static irqreturn_t s2mpu06_chg_isr(int irq, void *data)
{
	struct s2mpu06_charger_data *charger = data;
	u8 val;

	if (s2mpu06_read_reg(charger->client, S2MPU06_CHG_REG_STATUS1, &val)) {
		pr_err("%s: error reading charger status1\n", __func__);
		return IRQ_HANDLED;
	}

	pr_err("%s , %02x\n " , __func__, val);
	if (val & (1 << CHG_STATUS1_CHG_STS)) {
		pr_err("add self chg done \n");
		/* add chg done code here */
	}
	return IRQ_HANDLED;
}
static irqreturn_t s2mpu06_ovp_isr(int irq, void *data)
{
	struct s2mpu06_charger_data *charger = data;
	union power_supply_propval value;
	u8 val;

	if (s2mpu06_read_reg(charger->client, S2MPU06_CHG_REG_STATUS2, &val)) {
		pr_err("%s: error reading charger status2\n", __func__);
		return IRQ_HANDLED;
	}

	pr_err("%s , %02x\n " , __func__, val);
	if (val & (1 << CHGVINOVP_STS_SHIFT)) {
		pr_err("ovp status\n");
		charger->ovp = true;
		value.intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		psy_do_property("battery", set,
				POWER_SUPPLY_PROP_HEALTH, value);
		/* add chg done code here */
	}
	return IRQ_HANDLED;
}


static int s2mpu06_charger_parse_dt(struct device *dev,
		struct s2mpu06_charger_platform_data *pdata)
{
	struct device_node *np = of_find_node_by_name(NULL, "s2mpu06-charger");
	const u32 *p;
	int ret, i , len;


	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_string(np,
			"battery,charger_name", (char const **)&pdata->charger_name);
#if defined(CONFIG_FUELGAUGE_S2MPU06)
		ret = of_property_read_string(np,
			"battery,fuelgauge_name",
			(char const **)&pdata->fuelgauge_name);
#endif

		pdata->battery_type = of_get_named_gpio(np, "battery,battery_type", 0);
		if (gpio_get_value(pdata->battery_type)) {
			/* SC_CTRL8 , SET_VF_VBAT , Battery regulation voltage setting */
			ret = of_property_read_u32(np, "battery,chg_float_voltage2",
						   &pdata->chg_float_voltage);

			pr_info("%s : 2600mAh %d voltage Battery\n", __func__, pdata->chg_float_voltage);

		} else {
			/* SC_CTRL8 , SET_VF_VBAT , Battery regulation voltage setting */
			ret = of_property_read_u32(np, "battery,chg_float_voltage",
						   &pdata->chg_float_voltage);

			pr_info("%s : 2400mAh %d voltage Battery\n", __func__, pdata->chg_float_voltage);
		}

		ret = of_property_read_u32(np, "battery,full_check_type_2nd",
				&pdata->full_check_type_2nd);
		if (ret)
			pr_info("%s : Full check type 2nd is Empty\n",
						__func__);

		pdata->chg_eoc_dualpath = of_property_read_bool(np,
				"battery,chg_eoc_dualpath");

		p = of_get_property(np, "battery,input_current_limit", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->charging_current =
			kzalloc(sizeof(sec_charging_current_t) * len, GFP_KERNEL);

		for (i = 0; i < len; i++) {
			ret = of_property_read_u32_index(np,
				"battery,input_current_limit", i,
				&pdata->charging_current[i].input_current_limit);
			if (ret)
				pr_info("%s : Input_current_limit is Empty\n", __func__);
		}
	}

	dev_info(dev, "s2mpu06 charger parse dt retval = %d\n", ret);
	return ret;
}

/* if need to set s2mpu06 pdata */
static struct of_device_id s2mpu06_charger_match_table[] = {
	{ .compatible = "samsung,s2mpu06-charger",},
	{},
};

static int s2mpu06_charger_probe(struct platform_device *pdev)
{
	struct s2mpu06_dev *s2mpu06 = dev_get_drvdata(pdev->dev.parent);
	struct s2mpu06_platform_data *pdata = dev_get_platdata(s2mpu06->dev);
	struct s2mpu06_charger_data *charger;
	int ret = 0;

	pr_err("%s:[BATT] S2MPU06 Charger driver probe\n", __func__);
	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	mutex_init(&charger->io_lock);

	charger->dev = &pdev->dev;
	charger->client = s2mpu06->charger;
	charger->s2mpu06 = s2mpu06;
	charger->pdata = devm_kzalloc(&pdev->dev, sizeof(*(charger->pdata)),
			GFP_KERNEL);
	if (!charger->pdata) {
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_parse_dt_nomem;
	}
	ret = s2mpu06_charger_parse_dt(&pdev->dev, charger->pdata);
	if (ret < 0)
		goto err_parse_dt;

	platform_set_drvdata(pdev, charger);

	charger->dev_id = s2mpu06->pmic_rev;
	dev_info(&charger->client->dev, "%s : DEV ID : 0x%x\n", __func__,
			charger->dev_id);

	if (charger->pdata->charger_name == NULL)
		charger->pdata->charger_name = "sec-charger";

	charger->psy_chg.name           = charger->pdata->charger_name;
	charger->psy_chg.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property   = sec_chg_get_property;
	charger->psy_chg.set_property   = sec_chg_set_property;
	charger->psy_chg.properties     = s2mpu06_charger_props;
	charger->psy_chg.num_properties = ARRAY_SIZE(s2mpu06_charger_props);

	ret = power_supply_register(&pdev->dev, &charger->psy_chg);
	if (ret) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		goto err_power_supply_register;
	}

	s2mpu06_chg_init(charger);

	/*
	 * irq request
	 * if you need to add irq , please refer below code.
	 */
	charger->irq_det_bat = pdata->irq_base + S2MPU06_CHG_IRQ_BATP_INT1;
	ret = request_threaded_irq(charger->irq_det_bat, NULL,
			s2mpu06_det_bat_isr, 0 , "det-bat-in-irq", charger);
	if (ret < 0) {
		dev_err(s2mpu06->dev, "%s: Fail to request det bat in IRQ: %d: %d\n",
					__func__, charger->irq_det_bat, ret);
		goto err_reg_irq;
	}
	charger->irq_chg = pdata->irq_base + S2MPU06_CHG_IRQ_CHGSTS_INT3;
	ret = request_threaded_irq(charger->irq_chg, NULL,
			s2mpu06_chg_isr, 0 , "chg-irq", charger);
	if (ret < 0) {
		dev_err(s2mpu06->dev, "%s: Fail to request det bat in IRQ: %d: %d\n",
					__func__, charger->irq_chg, ret);
		goto err_reg_irq;
	}
	charger->irq_ovp = pdata->irq_base + S2MPU06_CHG_IRQ_CINOVP_INT1;
	ret = request_threaded_irq(charger->irq_ovp, NULL,
			s2mpu06_ovp_isr, 0 , "chg-ovp", charger);
	if (ret < 0) {
		dev_err(s2mpu06->dev, "%s: Fail to request det bat in IRQ: %d: %d\n",
					__func__, charger->irq_chg, ret);
		goto err_reg_irq;
	}
	s2mpu06_test_read(charger->client);

	charger->battery_cable_type = POWER_SUPPLY_TYPE_BATTERY;
	charger->cable_type = POWER_SUPPLY_TYPE_BATTERY;

	pr_info("%s:[BATT] S2MPU06 charger driver loaded OK\n", __func__);

	return 0;
err_reg_irq:
	power_supply_unregister(&charger->psy_chg);
err_power_supply_register:
err_parse_dt:
err_parse_dt_nomem:
	mutex_destroy(&charger->io_lock);
	kfree(charger);
	return ret;
}

static int s2mpu06_charger_remove(struct platform_device *pdev)
{
	struct s2mpu06_charger_data *charger =
		platform_get_drvdata(pdev);

	power_supply_unregister(&charger->psy_chg);
	mutex_destroy(&charger->io_lock);
	kfree(charger);
	return 0;
}

#if defined CONFIG_PM
static int s2mpu06_charger_suspend(struct device *dev)
{
	return 0;
}

static int s2mpu06_charger_resume(struct device *dev)
{
	return 0;
}
#else
#define s2mpu06_charger_suspend NULL
#define s2mpu06_charger_resume NULL
#endif

static void s2mpu06_charger_shutdown(struct device *dev)
{
	pr_info("%s: S2MPU06 Charger driver shutdown\n", __func__);
}

static SIMPLE_DEV_PM_OPS(s2mpu06_charger_pm_ops, s2mpu06_charger_suspend,
		s2mpu06_charger_resume);

static struct platform_driver s2mpu06_charger_driver = {
	.driver         = {
		.name   = "s2mpu06-charger",
		.owner  = THIS_MODULE,
		.of_match_table = s2mpu06_charger_match_table,
		.pm     = &s2mpu06_charger_pm_ops,
		.shutdown = s2mpu06_charger_shutdown,
	},
	.probe          = s2mpu06_charger_probe,
	.remove		= s2mpu06_charger_remove,
};

static int __init s2mpu06_charger_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&s2mpu06_charger_driver);

	return ret;
}
module_init(s2mpu06_charger_init);

static void __exit s2mpu06_charger_exit(void)
{
	platform_driver_unregister(&s2mpu06_charger_driver);
}
module_exit(s2mpu06_charger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Charger driver for S2MPU06");
