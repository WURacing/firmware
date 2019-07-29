/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include "CANBus19.h"

static struct can_module can_instance;
struct adc_module adc_instance;

static void configure_can(void)
{
	/* Set up the CAN TX/RX pins */
	struct system_pinmux_config pin_config;
	system_pinmux_get_config_defaults(&pin_config);
	pin_config.mux_position = CAN_TX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN_TX_PIN, &pin_config);
	pin_config.mux_position = CAN_RX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN_RX_PIN, &pin_config);
	
	/* Initialize the module. */
	struct can_config config_can;
	can_get_config_defaults(&config_can);
	can_init(&can_instance, CAN_MODULE, &config_can);
	can_start(&can_instance);
}

unsigned buffer;
static void can_send_standard_message(uint32_t id_value, uint8_t *data, uint32_t data_length)
{
	uint32_t i;
	struct can_tx_element tx_element;
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(id_value);
	tx_element.T1.bit.DLC = data_length;
	
	for (i = 0; i < data_length; i++) {
		tx_element.data[i] = *data;
		data++;
	}
	if (++buffer >= CONF_CAN0_TX_BUFFER_NUM)
		buffer = 0;
	
	can_set_tx_buffer_element(&can_instance, &tx_element, buffer);
	can_tx_transfer_request(&can_instance, 1 << buffer);
}


static void can_send_extended_message(uint32_t id_value, uint8_t *data,
uint32_t data_length)
{
	uint32_t i;
	struct can_tx_element tx_element;
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_EXTENDED_ID(id_value) |
	CAN_TX_ELEMENT_T0_XTD;
	tx_element.T1.bit.DLC = data_length;
	for (i = 0; i < data_length; i++) {
		tx_element.data[i] = *data;
		data++;
	}
	if (++buffer >= CONF_CAN0_TX_BUFFER_NUM)
	buffer = 0;
	
	can_set_tx_buffer_element(&can_instance, &tx_element, buffer);
	can_tx_transfer_request(&can_instance, 1 << buffer);
}


volatile uint8_t seqst;
volatile bool busy;




void configure_adc(void)
{
	struct adc_config config_adc;
	adc_get_config_defaults(&config_adc);
	config_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV8;
	config_adc.reference       = ADC_REFERENCE_INTVCC2;
	config_adc.positive_input  = ADC_POSITIVE_INPUT_PIN0;
	config_adc.resolution      = ADC_RESOLUTION_10BIT;
	adc_init(&adc_instance, ADC0, &config_adc);
	adc_enable(&adc_instance);
}

int ledstate;


// measures hundredths of a ms, 0.00001s per
// wont overflow for 11 hours
volatile uint32_t g_ul_ms_ticks = 0;
void SysTick_Handler(void)
{
	g_ul_ms_ticks++;
}

#ifdef WS_ENABLE

#define WS_COUNTS 6 
volatile uint32_t last_ws1 = 0;
volatile uint32_t last_ws2 = 0;
volatile uint32_t ticks_spoke_ws1 = 0;
volatile uint32_t ticks_spoke_ws2 = 0;

void ws1_trip(void)
{
	ticks_spoke_ws1 = g_ul_ms_ticks - last_ws1;
	last_ws1 = g_ul_ms_ticks;
}

void ws2_trip(void)
{
	ticks_spoke_ws2 = g_ul_ms_ticks - last_ws2;
	last_ws2 = g_ul_ms_ticks;
}

void configure_extint_channel(void)
{
	struct extint_chan_conf config_extint_chan;
	extint_chan_get_config_defaults(&config_extint_chan);
	
	// wheel speed 1
	config_extint_chan.gpio_pin           = WS1_PIN;
	config_extint_chan.gpio_pin_mux       = WS1_MUX;
	config_extint_chan.gpio_pin_pull      = EXTINT_PULL_UP;
	config_extint_chan.detection_criteria = EXTINT_DETECT_RISING;
	extint_chan_set_config(WS1_LINE, &config_extint_chan);
	
	// wheel speed 2
	config_extint_chan.gpio_pin           = WS2_PIN;
	config_extint_chan.gpio_pin_mux       = WS2_MUX;
	config_extint_chan.gpio_pin_pull      = EXTINT_PULL_UP;
	config_extint_chan.detection_criteria = EXTINT_DETECT_RISING;
	extint_chan_set_config(WS2_LINE, &config_extint_chan);
}

void configure_extint_callbacks(void)
{
	extint_register_callback(ws1_trip, WS1_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_chan_enable_callback(WS1_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_register_callback(ws2_trip, WS2_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_chan_enable_callback(WS2_LINE, EXTINT_CALLBACK_TYPE_DETECT);
}

#endif

void configure_gpio(void)
{
	struct system_pinmux_config config;
	
	system_pinmux_get_config_defaults(&config);
	config.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
	config.mux_position = 1;
	system_pinmux_pin_set_config(PIN_PA02, &config);
	system_pinmux_pin_set_config(PIN_PA03, &config);
	system_pinmux_pin_set_config(PIN_PA04, &config);
	system_pinmux_pin_set_config(PIN_PA05, &config);
	system_pinmux_pin_set_config(PIN_PA06, &config);
	system_pinmux_pin_set_config(PIN_PA07, &config);
}


int main (void)
{
	int i;
	uint8_t data[8];
	uint16_t rpm_ws1, rpm_ws2;
	
	int sensor_id;
	int sensor_ready;
	uint16_t sensor_data[6];
	
	uint32_t next;

	system_init();
	
	// 100kHz
	SysTick_Config(48000000 / 100000);
	

	configure_gpio();
	configure_can();
	configure_adc();
	#ifdef WS_ENABLE
	configure_extint_channel();
	configure_extint_callbacks();
	#endif

	


	sensor_id = 0;
	sensor_data[0] = sensor_data[1] = sensor_data[2] = sensor_data[3] =	sensor_data[4] = sensor_data[5] = 0;
	sensor_ready = 0;
	
	system_interrupt_enable_global();

	while(1) {

		next = g_ul_ms_ticks + 1 * 100;
		while(g_ul_ms_ticks < next) {};
		
		port_pin_set_output_level(LED_0_PIN, ledstate);
		ledstate = !ledstate;
		
		uint32_t start = g_ul_ms_ticks;
		
		*((uint64_t*)data) = 0;
		
		can_send_extended_message(ID_AEMEngine0, data, 8);

		uint32_t	delta = g_ul_ms_ticks - start;
		
		*((uint32_t*)data) = delta;
		
		can_send_extended_message(ID_AEMEngine3, data, 8);	
		
		
	}

}
