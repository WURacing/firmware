/*
 * led_task.c
 *
 * Created: 2/8/2020 5:25:50 PM
 *  Author: etshr
 */ 

#include <asf.h>
#include "exports.h"

xTimerHandle led_task_id;

volatile int led0, led1;

void configure_led(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &config_port_pin);
	port_pin_set_config(LED_1_PIN, &config_port_pin);
	led0 = led1 = 0;
}

void led_task(xTimerHandle h)
{
	port_pin_set_output_level(LED_0_PIN, led0);
	port_pin_set_output_level(LED_1_PIN, led1);

}