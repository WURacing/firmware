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
volatile uint32_t ticks_rev_ws1[WS_COUNTS] = {0};
volatile uint32_t ticks_rev_ws2[WS_COUNTS] = {0};
volatile int ticker_ws1 = 0;
volatile int ticker_ws2 = 0;

void ws1_trip(void)
{
	ticks_rev_ws1[ticker_ws1] = g_ul_ms_ticks - last_ws1;
	last_ws1 = g_ul_ms_ticks;
	
	if (++ticker_ws1 >= WS_COUNTS) {
		ticker_ws1 = 0;
	}
}

void ws2_trip(void)
{
	ticks_rev_ws2[ticker_ws2] = g_ul_ms_ticks - last_ws2;
	last_ws2 = g_ul_ms_ticks;
	
	if (++ticker_ws2 >= WS_COUNTS) {
		ticker_ws2 = 0;
	}
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
	uint32_t ticks_total_rev_ws1, ticks_total_rev_ws2;
	uint32_t rpm_ws1, rpm_ws2;
	
	int sensor_id;
	int sensor_ready;
	uint16_t sensor_data[6];

	system_init();
	delay_init();
	
	// 10kHz
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
		adc_set_negative_input(&adc_instance, ADC_NEGATIVE_INPUT_GND);
		adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN0 );
		adc_start_conversion(&adc_instance);
		while (adc_read(&adc_instance, sensor_data + 0) == STATUS_BUSY) ;
		adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN1);
		adc_start_conversion(&adc_instance);
		while (adc_read(&adc_instance, sensor_data + 1) == STATUS_BUSY) ;
		adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN4);
		adc_start_conversion(&adc_instance);
		while (adc_read(&adc_instance, sensor_data + 2) == STATUS_BUSY) ;
		adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN5);
		adc_start_conversion(&adc_instance);
		while (adc_read(&adc_instance, sensor_data + 3) == STATUS_BUSY) ;
		adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN6);
		adc_start_conversion(&adc_instance);
		while (adc_read(&adc_instance, sensor_data + 4) == STATUS_BUSY) ;
		adc_set_positive_input(&adc_instance, ADC_POSITIVE_INPUT_PIN7);
		adc_start_conversion(&adc_instance);
		while (adc_read(&adc_instance, sensor_data + 5) == STATUS_BUSY) ;
		
		delay_ms(25);
		
		port_pin_set_output_level(LED_0_PIN, ledstate);
		ledstate = !ledstate;
		
		#ifdef WS_ENABLE
		ticks_total_rev_ws1 = ticks_total_rev_ws2 = 0;
		for (i = 0; i < WS_COUNTS; ++i) {
			// wheel hasn't moved for 0.5 sec
			if (g_ul_ms_ticks - last_ws1 > 50000)
				ticks_rev_ws1[i] = 0;
			if (g_ul_ms_ticks - last_ws2 > 50000)
				ticks_rev_ws2[i] = 0;

			ticks_total_rev_ws1 += ticks_rev_ws1[i];
			ticks_total_rev_ws2 += ticks_rev_ws2[i];
		}
		if (ticks_total_rev_ws1 > 0)
			rpm_ws1 = min(6000000UL / ticks_total_rev_ws1, 1023);
		else
			rpm_ws1 = 0;
		if (ticks_total_rev_ws2 > 0)
			rpm_ws2 = min(6000000UL / ticks_total_rev_ws2, 1023);
		else
			rpm_ws2 = 0;
		#endif
		
		INIT_DataLogger3(data);
		#ifdef BOARD_1
		SET_DataLogger3_Analog1(data, sensor_data[0]);
		SET_DataLogger3_Analog2(data, sensor_data[1]);
		#endif
		#ifdef BOARD_2
		SET_DataLogger3_Analog1(data, rpm_ws1);
		SET_DataLogger3_Analog2(data, rpm_ws2);
		#endif
		SET_DataLogger3_Analog3(data, sensor_data[2]);
		SET_DataLogger3_Analog4(data, sensor_data[3]);
		SET_DataLogger3_Analog5(data, sensor_data[4]);
		SET_DataLogger3_Analog6(data, sensor_data[5]);
		#ifdef BOARD_1
		can_send_extended_message(ID_DataLogger3, data, 8);
		#endif
		#ifdef BOARD_2
		can_send_extended_message(ID_DataLogger3 + 1, data, 8);
		#endif
		#ifdef BOARD_3
		can_send_extended_message(ID_DataLogger3 + 10, data, 8);
		#endif
		#ifdef BOARD_4
		can_send_extended_message(ID_DataLogger3 + 61, data, 8);
		#endif

		
		
	}

}
