/*
 * current.c
 *
 * Created: 1/18/2020 3:42:43 PM
 *  Author: etshr
 */ 

#include "current.h"

#define CURR_FUEL 17
#define CURR_RAD 18
#define CURR_TOTAL 19

#define CURR_PINS 20

uint8_t pins[] = {
	PIN_PB00, // LC0
	PIN_PB01,
	PIN_PB02,
	PIN_PB03,
	PIN_PA02,
	PIN_PA03,
	PIN_PB04,
	PIN_PB05,
	PIN_PB06,
	PIN_PB07,
	PIN_PB08,
	PIN_PB09,
	PIN_PA04,
	PIN_PA05,
	PIN_PA06,
	PIN_PA07,
	PIN_PA08, // LC16
	PIN_PA09, // fuel
	PIN_PA10, // rad
	PIN_PA11  // total
};

int adc_no[] = {
	1,
	1,
	1,
	1,
	0,
	0,
	1,
	1,
	1,
	1,
	1,
	1,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

enum adc_positive_input adc_pin[] = {
	ADC_POSITIVE_INPUT_PIN0,
	ADC_POSITIVE_INPUT_PIN1,
	ADC_POSITIVE_INPUT_PIN2,
	ADC_POSITIVE_INPUT_PIN3,
	ADC_POSITIVE_INPUT_PIN0,
	ADC_POSITIVE_INPUT_PIN1,
	ADC_POSITIVE_INPUT_PIN6,
	ADC_POSITIVE_INPUT_PIN7,
	ADC_POSITIVE_INPUT_PIN8,
	ADC_POSITIVE_INPUT_PIN9,
	ADC_POSITIVE_INPUT_PIN4,
	ADC_POSITIVE_INPUT_PIN5,
	ADC_POSITIVE_INPUT_PIN4,
	ADC_POSITIVE_INPUT_PIN5,
	ADC_POSITIVE_INPUT_PIN6,
	ADC_POSITIVE_INPUT_PIN7,
	ADC_POSITIVE_INPUT_PIN8,
	ADC_POSITIVE_INPUT_PIN9,
	ADC_POSITIVE_INPUT_PIN10,
	ADC_POSITIVE_INPUT_PIN11
};

struct adc_module adc[2];


static void configure_gpio(void)
{
	struct system_pinmux_config config;
	int i;
	
	system_pinmux_get_config_defaults(&config);
	config.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
	config.mux_position = 1;
	
	for (i = 0; i < CURR_PINS; ++i)
		system_pinmux_pin_set_config(pins[i], &config);
}

static enum status_code configure_adc(void)
{
	enum status_code result;
	struct adc_config config;
	adc_get_config_defaults(&config);
	
	config.clock_prescaler = ADC_CLOCK_PRESCALER_DIV8;
	config.reference = ADC_REFERENCE_INTVCC2;
	config.positive_input = ADC_POSITIVE_INPUT_PIN0;
	config.resolution = ADC_RESOLUTION_12BIT;
	result = adc_init(&adc[0], ADC0, &config);
	if (result != STATUS_OK) return result;
	result = adc_init(&adc[1], ADC1, &config);
	if (result != STATUS_OK) return result;
	result = adc_enable(&adc[0]);
	if (result != STATUS_OK) return result;
	result = adc_enable(&adc[1]);
	return result;
}

enum status_code configure_current(void)
{
	configure_gpio();
	return configure_adc();
}

static uint16_t analog_read(int pin)
{
//	if (pin >= CURR_PINS)
	//	return -1;
		
	enum adc_positive_input pos = adc_pin[pin];
	struct adc_module *adc_instance = &adc[adc_no[pin]];
	
	uint16_t output;
	
	adc_set_negative_input(adc_instance, ADC_NEGATIVE_INPUT_GND);
	adc_set_positive_input(adc_instance, pos);
	adc_start_conversion(adc_instance);
	while (adc_read(adc_instance, &output) == STATUS_BUSY) ;
	
	return output;
}

void read_current(struct current_readings * output)
{
	int i;
	for (i = 0; i < 17; ++i)
	{
		output->lc[i] = analog_read(i);
	}
	output->fuel_pump = analog_read(CURR_FUEL);
	output->rad_fan = analog_read(CURR_RAD);
	output->total = analog_read(CURR_TOTAL);
}