#include "app_display.h"
#include "drv_i2c.h"
#include "app_inputs.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sam.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "drv_rtc.h"
#include "drv_lte.h"
#include "drv_can.h"
#include "pe3.h"
#include "app_data.h"
#include "app_datalogger.h"

#define APP_DISPLAY_PRIORITY 3

const uint8_t SevenSegmentASCII[96] = {
	0x00, /* (space) */
	0x86, /* ! */
	0x22, /* " */
	0x7E, /* # */
	0x6D, /* $ */
	0xD2, /* % */
	0x46, /* & */
	0x20, /* ' */
	0x29, /* ( */
	0x0B, /* ) */
	0x21, /* * */
	0x70, /* + */
	0x10, /* , */
	0x40, /* - */
	0x80, /* . */
	0x52, /* / */
	0x3F, /* 0 */
	0x06, /* 1 */
	0x5B, /* 2 */
	0x4F, /* 3 */
	0x66, /* 4 */
	0x6D, /* 5 */
	0x7D, /* 6 */
	0x07, /* 7 */
	0x7F, /* 8 */
	0x6F, /* 9 */
	0x09, /* : */
	0x0D, /* ; */
	0x61, /* < */
	0x48, /* = */
	0x43, /* > */
	0xD3, /* ? */
	0x5F, /* @ */
	0x77, /* A */
	0x7C, /* B */
	0x39, /* C */
	0x5E, /* D */
	0x79, /* E */
	0x71, /* F */
	0x3D, /* G */
	0x76, /* H */
	0x30, /* I */
	0x1E, /* J */
	0x75, /* K */
	0x38, /* L */
	0x15, /* M */
	0x37, /* N */
	0x3F, /* O */
	0x73, /* P */
	0x6B, /* Q */
	0x33, /* R */
	0x6D, /* S */
	0x78, /* T */
	0x3E, /* U */
	0x3E, /* V */
	0x2A, /* W */
	0x76, /* X */
	0x6E, /* Y */
	0x5B, /* Z */
	0x39, /* [ */
	0x64, /* \ */
	0x0F, /* ] */
	0x23, /* ^ */
	0x08, /* _ */
	0x02, /* ` */
	0x5F, /* a */
	0x7C, /* b */
	0x58, /* c */
	0x5E, /* d */
	0x7B, /* e */
	0x71, /* f */
	0x6F, /* g */
	0x74, /* h */
	0x10, /* i */
	0x0C, /* j */
	0x75, /* k */
	0x30, /* l */
	0x14, /* m */
	0x54, /* n */
	0x5C, /* o */
	0x73, /* p */
	0x67, /* q */
	0x50, /* r */
	0x6D, /* s */
	0x78, /* t */
	0x1C, /* u */
	0x1C, /* v */
	0x14, /* w */
	0x76, /* x */
	0x6E, /* y */
	0x5B, /* z */
	0x46, /* { */
	0x30, /* | */
	0x70, /* } */
	0x01, /* ~ */
	0xFF, /* (del) */
};

enum app_display_channel
{
	APP_DISPLAY_CHANNEL_RPM_1 = 0x40,
	APP_DISPLAY_CHANNEL_RPM_2,
	APP_DISPLAY_CHANNEL_RPM_3,
	APP_DISPLAY_CHANNEL_RPM_4,
	APP_DISPLAY_CHANNEL_GEAR,
	APP_DISPLAY_CHANNEL_SHIFT_1,
	APP_DISPLAY_CHANNEL_SHIFT_2,
	APP_DISPLAY_CHANNEL_WARNING,
};

static uint16_t make_code(uint8_t code) {
	uint16_t output = 0;
	// each LED takes 2 bits -- set to 01 if on, and 00 if off
	output |= code & 1;
	output |= (code & 0b10) << 1;
	output |= (code & 0b100) << 2;
	output |= (code & 0b1000) << 3;
	output |= (code & 0b10000) << 4;
	output |= (code & 0b100000) << 5;
	output |= (code & 0b1000000) << 6;
	output |= (code & 0b10000000) << 7;
	return output;
}

static void tlc59108_set_output_state(uint8_t address, uint8_t state)
{
	uint16_t tx = make_code(state);
	drv_i2c_write_register(DRV_I2C_CHANNEL_LED, address, 0x80 | 0xC, (const uint8_t *)&tx, 2);
}

static void display_gear(char c)
{
	tlc59108_set_output_state(APP_DISPLAY_CHANNEL_GEAR, SevenSegmentASCII[c - 0x20]);
}

static void display_rpm(const char *str, unsigned dpmask)
{
	int i, n;
	char c[4];
	n = strlen(str);
	for (i = 0; i < 4; ++i)
	{
		c[i] = n > i ? SevenSegmentASCII[str[i] - 0x20] : 0;
		c[i] |= (dpmask & (1 << i)) << (7-i);
	}
	tlc59108_set_output_state(APP_DISPLAY_CHANNEL_RPM_1, c[0]);
	tlc59108_set_output_state(APP_DISPLAY_CHANNEL_RPM_2, c[1]);
	tlc59108_set_output_state(APP_DISPLAY_CHANNEL_RPM_3, c[2]);
	tlc59108_set_output_state(APP_DISPLAY_CHANNEL_RPM_4, c[3]);
}

static void display_shift(unsigned bitmask)
{
	tlc59108_set_output_state(APP_DISPLAY_CHANNEL_SHIFT_1, bitmask & 0xFF);
	tlc59108_set_output_state(APP_DISPLAY_CHANNEL_SHIFT_2, (bitmask >> 8) & 0x7F);
}

static void display_warning(unsigned bitmask)
{
	tlc59108_set_output_state(APP_DISPLAY_CHANNEL_WARNING, bitmask & 0x1F);
}

static void set_digit(uint8_t display_index, uint8_t digit, bool dp) {
	if(digit > 9) {
		// Invalid digit
		return; 
	}
	
	uint8_t code = SevenSegmentASCII[digit + 0x10];
	uint8_t address = APP_DISPLAY_CHANNEL_RPM_1 + display_index;
	uint16_t tx = make_code(code);
	tx |= (dp << 14);
	
	drv_i2c_write_register(DRV_I2C_CHANNEL_LED, address, 0x80 | 0xC, (const uint8_t *)&tx, 2);
}

static void set_shift_lights(int n)
{
	if (n < 0 || n > 15)
	{
		return;
	}
	unsigned bitmask = 0;
	for (int i = n; i > 0; --i)
	{
		bitmask |= (1 << (i-1));
	}
	display_shift(bitmask);
}

static void set_rpm(int rpm) {
	// Write the last four displays
	bool over10k = (rpm >= 10000);
	int rem = rpm;
	if (over10k)
	{
		rem /= 10;
	}
	for(int i = 0; i < 4; i++) {
		int digit = rem % 10;
		rem /= 10;
		bool dp = false;
		if ((over10k && i == 2) || (!over10k && i == 3))
		{
			dp = true;
		}
		set_digit(3-i, digit, dp);
	}
	
	// Write shift lights
	// <6400 -> 0
	// 6400 - 9000 -> 1 - 10
	// >9000 -> 15
	if (rpm < 6400)
	{
		// all off
		set_shift_lights(0);
	}
	else if (rpm >= 9000)
	{
		// all on
		set_shift_lights(15);
	}
	else
	{
		int n = (rpm - 6400 + 260) / 260;
		set_shift_lights(n);
	}
}

static void scroll_text(const char *s)
{
	int i, n;
	n = strlen(s);
	display_gear(' ');
	display_shift(0);
	display_warning(0);
	for (i = 0; i < n-3; ++i)
	{
		display_rpm(s + i, 0);
		vTaskDelay(200);
	}
	vTaskDelay(500);
}


#define INIT_DELAY 200
#define CHANGE_TIME 800


enum display_mode
{
	DM_RPM,
	DM_MPH,
	DM_TIME,
	DM_BATT,
	DM_TEMP,
	DM_DEBUG,
	
	DM_COUNT
};

#define DEBUG_LEN 128
#define TEMP_LEN 20
#define STACK_SIZE 768

static struct
{
	int eARBFront, eARBFrontPrevious;
	int eARBRear, eARBRearPrevious;
	int daqMode, daqModePrevious;
	int displayMode, displayModePrevious;
	TickType_t eARBFrontChanged, eARBRearChanged, daqModeChanged, displayModeChanged, drsChanged,
			shiftUpChanged, shiftDownChanged, miscChanged, engineMissingSince, sbfront1MissingSince;
	bool drs, drsPrevious;
	bool shiftUp, shiftUpPrevious;
	bool shiftDown, shiftDownPrevious;
	bool misc, miscPrevious;
	int rpm, tmp, decifeetpersecond, centivolt;
	bool rpmValid, tmpValid, ftsValid, voltValid;
	bool engineOnline, sbfront1Online;
	union {
		struct {
			uint8_t daq:1;
			uint8_t telemetry:1;
			uint8_t datalogger:1;
			uint8_t fuse:1;
			uint8_t coolant:1;
		} bit;
		uint8_t reg;
	} warnings;
	char debug[DEBUG_LEN];
	char temp[TEMP_LEN];
	StaticTask_t rtos_task_id;
	StackType_t  rtos_stack[STACK_SIZE];
} app_display_data = {0};

static void read_inputs()
{
	strcpy(app_display_data.debug, " ");
	app_display_data.eARBFrontPrevious = app_display_data.eARBFront;
	app_display_data.eARBRearPrevious = app_display_data.eARBRear;
	app_display_data.daqModePrevious = app_display_data.daqMode;
	app_display_data.displayModePrevious = app_display_data.displayMode;
	app_display_data.drsPrevious = app_display_data.drs;
	app_display_data.shiftUpPrevious = app_display_data.shiftUp;
	app_display_data.shiftDownPrevious = app_display_data.shiftDown;
	app_display_data.miscPrevious = app_display_data.misc;
	
	app_display_data.eARBFront = app_inputs_get_dial(APP_INPUTS_SW1);
	app_display_data.eARBRear = app_inputs_get_dial(APP_INPUTS_SW2);
	app_display_data.daqMode = app_inputs_get_dial(APP_INPUTS_SW3);
	if (app_display_data.daqMode > 3) app_display_data.daqMode = 3;
	app_display_data.displayMode = app_inputs_get_dial(APP_INPUTS_SW4)-1;
	if (app_display_data.displayMode >= DM_COUNT) app_display_data.displayMode = DM_COUNT-1;
	app_display_data.drs = (app_inputs_get_button(APP_INPUTS_DRS_L) || app_inputs_get_button(APP_INPUTS_DRS_R));
	app_display_data.shiftUp = app_inputs_get_button(APP_INPUTS_SHIFT_UP);
	app_display_data.shiftDown = app_inputs_get_button(APP_INPUTS_SHIFT_DOWN);
	app_display_data.misc = (app_inputs_get_button(APP_INPUTS_MISC_L) || app_inputs_get_button(APP_INPUTS_MISC_R));
	
	if (app_display_data.eARBFront != app_display_data.eARBFrontPrevious) app_display_data.eARBFrontChanged = xTaskGetTickCount();
	if (app_display_data.eARBRear != app_display_data.eARBRearPrevious) app_display_data.eARBRearChanged = xTaskGetTickCount();
	if (app_display_data.daqMode != app_display_data.daqModePrevious) app_display_data.daqModeChanged = xTaskGetTickCount();
	if (app_display_data.displayMode != app_display_data.displayModePrevious) app_display_data.displayModeChanged = xTaskGetTickCount();
	if (app_display_data.drs != app_display_data.drsPrevious && app_display_data.drs) app_display_data.drsChanged = xTaskGetTickCount();
	if (app_display_data.shiftUp != app_display_data.shiftUpPrevious && app_display_data.shiftUp) app_display_data.shiftUpChanged = xTaskGetTickCount();
	if (app_display_data.shiftDown != app_display_data.shiftDownPrevious && app_display_data.shiftDown) app_display_data.shiftDownChanged = xTaskGetTickCount();
	if (app_display_data.misc != app_display_data.miscPrevious && app_display_data.misc) app_display_data.miscChanged = xTaskGetTickCount();
	
	// Read engine speed
	struct app_data_message message;
	if (app_data_read_message(PE3_PE01_FRAME_ID, &message))
	{
		struct pe3_pe01_t pe01;
		pe3_pe01_unpack(&pe01, message.data, 8);
		app_display_data.engineOnline = true;
		if (pe3_pe01_engine_speed_is_in_range(pe01.engine_speed))
		{
			app_display_data.rpm = pe01.engine_speed;
			app_display_data.rpmValid = true;
		}
		else
		{
			app_display_data.rpmValid = false;
		}
	}
	else
	{
		app_display_data.rpmValid = false;
		if (app_display_data.engineOnline)
			app_display_data.engineMissingSince = xTaskGetTickCount();
		app_display_data.engineOnline = false;
		strcat(app_display_data.debug, "nO PE01 - ");
	}
	
	// Read coolant temp (battery volt is also here)
	bool battery_bad = true;
	bool coolant_bad = true;
	if (app_data_read_message(PE3_PE06_FRAME_ID, &message))
	{
		struct pe3_pe06_t pe06;
		pe3_pe06_unpack(&pe06, message.data, 8);
		if (pe3_pe06_coolant_temp_is_in_range(pe06.coolant_temp))
		{
			if (pe06.temp_type) // F to C
				app_display_data.tmp = ((int)pe06.coolant_temp * 9 / 50) + 32;
			else
				app_display_data.tmp = (int)pe06.coolant_temp / 10;
			app_display_data.tmpValid = true;
			coolant_bad = (app_display_data.tmp > 200 || app_display_data.tmp < 35);
		}
		else
		{
			app_display_data.tmpValid = false;
		}
		if (pe3_pe06_battery_volt_is_in_range(pe06.battery_volt))
		{
			app_display_data.centivolt = pe06.battery_volt;
			app_display_data.voltValid = true;
			battery_bad = (pe06.battery_volt < 1200 || pe06.battery_volt > 1600);
		}
		else
		{
			app_display_data.voltValid = false;
		}
	}
	else
	{
		app_display_data.tmpValid = false;
		app_display_data.voltValid = false;
		strcat(app_display_data.debug, "nO PE06 - ");
	}
	
	// Read wheel speed
	if (app_data_read_message(PE3_PE12_FRAME_ID, &message))
	{
		struct pe3_pe12_t pe12;
		pe3_pe12_unpack(&pe12, message.data, 8);
		if (pe3_pe12_driven_avg_wheel_speed_is_in_range(pe12.driven_avg_wheel_speed))
		{
			app_display_data.decifeetpersecond = pe12.driven_avg_wheel_speed;
			app_display_data.ftsValid = true;
		}
		else
		{
			app_display_data.ftsValid = false;
		}
	}
	else
	{
		app_display_data.ftsValid = false;
		strcat(app_display_data.debug, "nO PE12 - ");
	}
	
	
	bool sb_front1_missing = app_data_is_missing(VEHICLE_SB_FRONT1_SIGNALS1_FRAME_ID);
	if (sb_front1_missing)
	{
		strcat(app_display_data.debug, "nO SbF1 - ");
	}
	bool lte_missing = !drv_lte_is_logged_in();
	if (lte_missing)
	{
		strcat(app_display_data.debug, "LtE nr - ");
	}
#if 0 /* fuse indicator broken on PDM Rev0 */
	bool fuse_blown = app_inputs_get_button(APP_INPUTS_FUSE);
	if (fuse_blown)
	{
		strcat(app_display_data.debug, "FuSE bLOn - ");
	}
#endif
	bool datalogger_bad = !app_datalogger_okay();
	
	app_display_data.warnings.bit.daq = sb_front1_missing;
	app_display_data.warnings.bit.telemetry = lte_missing;
	app_display_data.warnings.bit.datalogger = datalogger_bad;
	app_display_data.warnings.bit.fuse = battery_bad;
	app_display_data.warnings.bit.coolant = coolant_bad;
	
	strcat(app_display_data.debug, ".");
}

// IMPORTANT: keep all floating point numbers inside this macro else the slow code demons will emerge
#define TO_FIXED(x) ((int)((x)*1000.0))
#define IN_PER_DF 1.2
#define S_PER_MIN 60
#define PI 3.1415926
#define TIRE_DIA_IN 18.3
#define PRIMARY_RATIO 2.073
#define FINAL_DRIVE 3.27
/*
 * @param rpm Engine speed, in revolutions per minute
 * @param dfs Wheel speed, in tenths of feet per second 
 * @return Thousandths of a gear ratio
 */
static inline int estimate_gear_ratio(int rpm, int dfs)
{
	// Algorithm: get wheel RPM, divide engine RPM by it, divide by primary and final drive ratios
	int icoefficient = TO_FIXED(1.0 / ((IN_PER_DF * S_PER_MIN * 1.0/(TIRE_DIA_IN*PI)) * PRIMARY_RATIO * FINAL_DRIVE));
	return (rpm * icoefficient) / dfs;
}

static inline char estimate_gear(int ratio_thou)
{
	if (ratio_thou > TO_FIXED(3)) return '~'; // ratio too high
	if (ratio_thou > TO_FIXED((2.583 - 2)/2.0)) return '1';
	if (ratio_thou > TO_FIXED((2 - 1.67)/2.0)) return '2';
	if (ratio_thou > TO_FIXED((1.67 - 1.444)/2.0)) return '3';
	if (ratio_thou > TO_FIXED((1.444 - 1.286)/2.0)) return '4';
	if (ratio_thou > TO_FIXED((1.286 - 1.15)/2.0)) return '5';
	if (ratio_thou > TO_FIXED(1)) return '6';
	return '_'; // meaning ratio too low
}

static void app_display_task()
{
	// Toggle DISPLAY_RESET_L
	PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA21;
	PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA21;
	vTaskDelay(1);
	PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA21;
	vTaskDelay(1);

	// set MODE1 for all TLC ICs
	uint8_t data[8];
	data[0] = 0x01;
	for (int i = 0; i < 8; ++i)
	{
		drv_i2c_write_register(DRV_I2C_CHANNEL_LED, 0x40 | i, 0, data, 1);
	}
	vTaskDelay(1);
	
	// Initialization visual / power on self test
	display_rpm("InIt", 0);
	vTaskDelay(INIT_DELAY);
	display_shift(0x1F);
	vTaskDelay(INIT_DELAY);	
	display_shift(0x1F << 5);
	vTaskDelay(INIT_DELAY);	
	display_shift(0x1F << 10);
	vTaskDelay(INIT_DELAY);	
	display_shift(0);
	display_warning(0x1F);
	vTaskDelay(INIT_DELAY);
	display_warning(0);
	//display_rpm("\x7F\x7F\x7F\x7F", 0xF);
	//display_gear('\x7F');
	//vTaskDelay(INIT_DELAY);
	
	TickType_t now = xTaskGetTickCount();
	bool sd_bad, can_bad;
	do {
		display_rpm("----", 0);
		display_gear('-');
		sd_bad = !app_datalogger_read_data();
		can_bad = app_data_is_missing(VEHICLE_SB_FRONT1_SIGNALS1_FRAME_ID);
		vTaskDelayUntil(&now, 10);
	} while (now < 5000 && (sd_bad || can_bad));
	
	if (sd_bad)
	{
		scroll_text("Err Sd CArd");
	}
	if (can_bad)
	{
		scroll_text("Err CAn dAq");
	}
	
	app_display_data.warnings.reg = 0;
	app_display_data.displayMode = DM_TIME;
	read_inputs();
	while (1)
	{
		read_inputs();
		char gear = '-';
		if (app_display_data.rpmValid && app_display_data.ftsValid)
		{
			int gear_ratio_thou = estimate_gear_ratio(app_display_data.rpm, app_display_data.decifeetpersecond);
			gear = estimate_gear(gear_ratio_thou);
		}
		// Check for changes in dial position and display value if so
		if (xTaskGetTickCount() - app_display_data.eARBFrontChanged < CHANGE_TIME)
		{
			const struct servo_config * servos = app_datalogger_get_servo_positions();
			snprintf(app_display_data.temp, TEMP_LEN, "%d", servos->eARBFrontPulses[app_display_data.eARBFront-1]);
			display_gear('F');
			display_rpm(app_display_data.temp, 0);
			display_shift(0);
			display_warning(0);
		}
		else if (xTaskGetTickCount() - app_display_data.eARBRearChanged < CHANGE_TIME)
		{
			const struct servo_config * servos = app_datalogger_get_servo_positions();
			snprintf(app_display_data.temp, TEMP_LEN, "%d", servos->eARBRearPulses[app_display_data.eARBRear-1]);
			display_gear('R');
			display_rpm(app_display_data.temp, 0);
			display_shift(0);
			display_warning(0);
		}
		else if (xTaskGetTickCount() - app_display_data.daqModeChanged < CHANGE_TIME)
		{
			const char *s;
			switch (app_display_data.daqMode)
			{
			case 1:
				s = "ACCELErAtIOn";
				break;
			case 2:
				s = "AUtOCrOSS";
				break;
			case 3:
				s = "brAkES";
				break;
			default:
				s = "EEEE";
				break;
			}
			scroll_text(s);
		}
		else if (xTaskGetTickCount() - app_display_data.displayModeChanged < CHANGE_TIME)
		{
			const char *s;
			switch (app_display_data.displayMode)
			{
			case DM_RPM:
				s = "rot";
				break;
			case DM_MPH:
				s = "LIn";
				break;
			case DM_TIME:
				s = "CLOC";
				break;
			case DM_BATT:
				s = "bAtt";
				break;
			case DM_TEMP:
				s = "dEgF";
				break;
			case DM_DEBUG:
				s = "dbg";
				break;
			default:
				s = "EEEE";
				break;
			}
			display_rpm(s, 0);
		}
		else if (xTaskGetTickCount() - app_display_data.drsChanged < CHANGE_TIME)
		{
			display_rpm("DrS", 0);
		}
		else if (xTaskGetTickCount() - app_display_data.miscChanged < CHANGE_TIME)
		{
			display_rpm("MISC", 0);
		}
		else if (xTaskGetTickCount() - app_display_data.shiftDownChanged < CHANGE_TIME)
		{
			display_rpm("DOWN", 0);
		}
		else if (xTaskGetTickCount() - app_display_data.shiftUpChanged < CHANGE_TIME)
		{
			display_rpm("UP", 0);
		}
		else if (xTaskGetTickCount() - app_display_data.engineMissingSince < CHANGE_TIME)
		{
			scroll_text("EngInE OFF");
		}
		else if (app_display_data.displayMode == DM_RPM)
		{
			if (app_display_data.rpmValid)
			{
				set_rpm(app_display_data.rpm);
			}
			else
			{
				display_rpm("Err", 0);
				display_shift(0);
			}
			
			display_gear(gear);
		}
		else if (app_display_data.displayMode == DM_MPH)
		{
			if (app_display_data.ftsValid)
			{
				// ft/s factor 0.1, so 1 unit = 0.068 MPH. 15 units is about 1 whole MPH
				snprintf(app_display_data.temp, TEMP_LEN, "%2d%02d", app_display_data.decifeetpersecond / 15, (app_display_data.decifeetpersecond % 15) * 7);
				display_rpm(app_display_data.temp, 0b0010);
			}
			else
			{
				display_rpm("Err", 0);
				display_shift(0);
			}
			
			display_gear(gear);
		}
		else if (app_display_data.displayMode == DM_TIME)
		{
			struct tm * time = localtime(NULL);
			if (time->tm_sec % 15 < 2)
			{
				strftime(app_display_data.temp, TEMP_LEN, "%Y", time);
				display_rpm(app_display_data.temp, 0);
				display_gear('y');
			}
			else if (time->tm_sec % 15 < 5)
			{
				strftime(app_display_data.temp, TEMP_LEN, "%m%d", time);
				display_rpm(app_display_data.temp, 0b0010);
				display_gear('d');
			}
			else if (time->tm_sec % 15 < 10)
			{
				strftime(app_display_data.temp, TEMP_LEN, "%H%M", time);
				display_rpm(app_display_data.temp, (time->tm_sec % 2 == 0) << 1);
				display_gear('H');
			}
			else
			{
				strftime(app_display_data.temp, TEMP_LEN, "%S", time);
				// Print decimal seconds
				snprintf(app_display_data.temp + 2, TEMP_LEN - 2, "%02d", drv_rtc_get_ms() / 10);
				display_rpm(app_display_data.temp, 0b0010);
				display_gear('s');
			}
		}
		else if (app_display_data.displayMode == DM_BATT)
		{
			if (app_display_data.voltValid)
			{
				snprintf(app_display_data.temp, TEMP_LEN, "%4d", app_display_data.centivolt);
				display_rpm(app_display_data.temp, 0b0010);
			}
			else
			{
				display_rpm("Err", 0);
				display_shift(0);
			}
		}
		else if (app_display_data.displayMode == DM_TEMP)
		{
			snprintf(app_display_data.temp, TEMP_LEN, "%3dF", app_display_data.tmp);
			display_rpm(app_display_data.temp, 0);
		}
		else if (app_display_data.displayMode == DM_DEBUG)
		{
			scroll_text(app_display_data.debug);
		}
		else
		{
			display_rpm(" -- ", 0);
		}
		
		
		
		display_warning(app_display_data.warnings.reg);
		vTaskDelayUntil(&now, 50);
	}
	vTaskDelete(NULL);
}


void app_display_init(void)
{
	xTaskCreateStatic(app_display_task, "STATUS", STACK_SIZE, NULL, APP_DISPLAY_PRIORITY, app_display_data.rtos_stack, &app_display_data.rtos_task_id);
}


void vApplicationStackOverflowHook(TaskHandle_t xTask,
								   signed char *pcTaskName)
{
	(void)xTask;
	(void)pcTaskName;
	display_rpm("-So-", 0);
	configASSERT(0);
}

__attribute__((naked))
void HardFault_Handler(void)
{
	display_rpm("-HF-", 0);
	configASSERT(0);
}