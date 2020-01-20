/*
 * can.c
 *
 * Created: 4/27/2019 7:02:06 PM
 *  Author: Connor
 */ 

#include <asf.h>
#include "util.h"

volatile int canline_0_updated;
volatile int canline_1_updated;

struct can_module can0_instance;
struct can_module can1_instance;

/*
doncu, forget about 
http://asf.atmel.com/docs/latest/samc21/html/asfdoc_sam0_can_basic_use_case.html
*/

volatile can_message_t canline[64];
volatile int canline_i = 0;
volatile int pkt_count = 0;

void configure_can(void)
{
	
	// Set up the CAN TX/RX pins
	struct system_pinmux_config pin_config;
	system_pinmux_get_config_defaults(&pin_config);
	// CAN0
	pin_config.mux_position = CAN0_TX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN0_TX_PIN, &pin_config);
	pin_config.mux_position = CAN0_RX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN0_RX_PIN, &pin_config);
	
	// CAN1
	
	pin_config.mux_position = CAN1_TX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN1_TX_PIN, &pin_config);
	pin_config.mux_position = CAN1_RX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN1_RX_PIN, &pin_config);
	
	// Initialize the module.
	struct can_config config_can0;
	// CAN0
	can_get_config_defaults(&config_can0);
	config_can0.nonmatching_frames_action_standard = CAN_NONMATCHING_FRAMES_FIFO_0;
	config_can0.nonmatching_frames_action_extended = CAN_NONMATCHING_FRAMES_FIFO_1;
	config_can0.timestamp_prescaler = 0xf;
	
	can_init(&can0_instance, CAN0_MODULE, &config_can0);
	can_start(&can0_instance);
	can_set_baudrate(CAN0_MODULE, 500000);
	
	canline_0_updated = 0;
	
	// CAN1
	struct can_config config_can1;
	can_get_config_defaults(&config_can1);
	config_can1.nonmatching_frames_action_standard = CAN_NONMATCHING_FRAMES_FIFO_0;
	config_can1.nonmatching_frames_action_extended = CAN_NONMATCHING_FRAMES_FIFO_1;
	config_can1.timestamp_prescaler = 0xf;
	can_init(&can1_instance, CAN1_MODULE, &config_can1);
	can_start(&can1_instance);
	can_set_baudrate(CAN1_MODULE, 500000);
	
	canline_1_updated = 0;
	// Enable interrupts for this CAN module 
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN0);
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN1);
	can_enable_interrupt(&can0_instance, CAN_PROTOCOL_ERROR_ARBITRATION | CAN_PROTOCOL_ERROR_DATA);
	can_enable_interrupt(&can1_instance, CAN_PROTOCOL_ERROR_ARBITRATION | CAN_PROTOCOL_ERROR_DATA);
	
	// read can 0 into fifo 1
	struct can_extended_message_filter_element et0_filter;
	can_get_extended_message_filter_element_default(&et0_filter);
	et0_filter.F1.reg = CAN_EXTENDED_MESSAGE_FILTER_ELEMENT_F1_EFID2(0) | CAN_EXTENDED_MESSAGE_FILTER_ELEMENT_F1_EFT_CLASSIC;
	can_set_rx_extended_filter(&can0_instance, &et0_filter, 0);
	
	// read can 1 into fifo 1
	
	struct can_extended_message_filter_element et1_filter;
	can_get_extended_message_filter_element_default(&et1_filter);
	et1_filter.F1.reg = CAN_EXTENDED_MESSAGE_FILTER_ELEMENT_F1_EFID2(0) | CAN_EXTENDED_MESSAGE_FILTER_ELEMENT_F1_EFT_CLASSIC;
	can_set_rx_extended_filter(&can1_instance, &et1_filter, 0);
	
	//can_enable_interrupt(&can0_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
	can_enable_interrupt(&can1_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
	//can_enable_interrupt(&can1_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
	can_enable_interrupt(&can0_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
	//can_enable_interrupt(&can0_instance, CAN_RX_BUFFER_NEW_MESSAGE);

}


static volatile uint32_t standard_receive_index = 0;
static volatile uint32_t extended_receive_index = 0;
static struct can_rx_element_fifo_0 rx_element_fifo_0;
static struct can_rx_element_fifo_1 rx_element_fifo_1;
static struct can_rx_element_buffer rx_element_buffer;


void CAN0_Handler(void)
{
	volatile uint32_t status, i, rx_buffer_index;
	status = can_read_interrupt_status(&can0_instance);


	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can0_instance, CAN_RX_BUFFER_NEW_MESSAGE);
		for (i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can0_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can0_instance, i);
				can_get_rx_buffer_element(&can0_instance, &rx_element_buffer,
				rx_buffer_index);
				if (rx_element_buffer.R0.bit.XTD) {
					//printf("\n\r Extended message received in Rx buffer. The received data is: \r\n");
					} else {
					//printf("\n\r Standard message received in Rx buffer. The received data is: \r\n");
				}
				for (i = 0; i < rx_element_buffer.R1.bit.DLC; i++) {
					//printf("  %d",rx_element_buffer.data[i]);
				}
				//printf("\r\n\r\n");
			}
		}
	}
	if (status & CAN_RX_FIFO_0_NEW_MESSAGE) {
		can_clear_interrupt_status(&can0_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
		can_get_rx_fifo_0_element(&can0_instance, &rx_element_fifo_0,
		standard_receive_index);
		can_rx_fifo_acknowledge(&can0_instance, 0,
		standard_receive_index);
		standard_receive_index++;
		if (standard_receive_index == CONF_CAN0_RX_FIFO_0_NUM) {
			standard_receive_index = 0;
		}
		if (canline_i >= 64) return;
		canline[canline_i].id = CAN_RX_ELEMENT_R0_ID(rx_element_fifo_0.R0.bit.ID);
		canline[canline_i].ts = CAN_RX_ELEMENT_R1_RXTS(rx_element_buffer.R1.bit.RXTS);
		
		for (i = 0; i < rx_element_fifo_1.R1.bit.DLC; i++) {
			// store data
			canline[canline_i].data.arr[i] = rx_element_fifo_0.data[i];
		}
		canline_0_updated = 1;
	}
	
	
	
	if (status & CAN_RX_FIFO_1_NEW_MESSAGE) {
		can_clear_interrupt_status(&can0_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
		can_get_rx_fifo_1_element(&can0_instance, &rx_element_fifo_1,
		extended_receive_index);
		can_rx_fifo_acknowledge(&can0_instance, 1,
		extended_receive_index);
		extended_receive_index++;
		if (extended_receive_index == CONF_CAN0_RX_FIFO_1_NUM) {
			extended_receive_index = 0;
		}
		if (canline_i >= 64) return;
		canline[canline_i].id = CAN_RX_ELEMENT_R0_ID(rx_element_fifo_1.R0.bit.ID);
		canline[canline_i].ts = CAN_RX_ELEMENT_R1_RXTS(rx_element_buffer.R1.bit.RXTS);
		
		for (i = 0; i < rx_element_fifo_1.R1.bit.DLC; i++) {
			// store data
			canline[canline_i].data.arr[i] = rx_element_fifo_1.data[i];
		}
		canline_1_updated = 1;
		pkt_count++;
	}

	if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
	|| (status & CAN_PROTOCOL_ERROR_DATA)) {
		can_clear_interrupt_status(&can0_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
		//printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
	}
}

void CAN1_Handler(void)
{
	volatile uint32_t status, i, rx_buffer_index;
	status = can_read_interrupt_status(&can1_instance);

	/*
	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can1_instance, CAN_RX_BUFFER_NEW_MESSAGE);
		for (i = 0; i < CONF_CAN1_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can1_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can1_instance, i);
				can_get_rx_buffer_element(&can1_instance, &rx_element_buffer,
				rx_buffer_index);
				if (rx_element_buffer.R0.bit.XTD) {
					//printf("\n\r Extended message received in Rx buffer. The received data is: \r\n");
					} else {
					//printf("\n\r Standard message received in Rx buffer. The received data is: \r\n");
				}
				for (i = 0; i < rx_element_buffer.R1.bit.DLC; i++) {
					//printf("  %d",rx_element_buffer.data[i]);
				}
				//printf("\r\n\r\n");
			}
		}
	}
	if (status & CAN_RX_FIFO_0_NEW_MESSAGE) {
		can_clear_interrupt_status(&can1_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
		can_get_rx_fifo_0_element(&can1_instance, &rx_element_fifo_0,
		standard_receive_index);
		can_rx_fifo_acknowledge(&can1_instance, 0,
		standard_receive_index);
		standard_receive_index++;
		if (standard_receive_index == CONF_CAN1_RX_FIFO_0_NUM) {
			standard_receive_index = 0;
		}
		canline_1_updated = 1;
	}
	*/
	
	
	if (status & CAN_RX_FIFO_1_NEW_MESSAGE) {
		can_clear_interrupt_status(&can1_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
		can_get_rx_fifo_1_element(&can1_instance, &rx_element_fifo_1,
		extended_receive_index);
		can_rx_fifo_acknowledge(&can1_instance, 1,
		extended_receive_index);
		extended_receive_index++;
		if (extended_receive_index == CONF_CAN1_RX_FIFO_1_NUM) {
			extended_receive_index = 0;
		}
		//if (canline_i >= 64) return;
		canline[0].id = CAN_RX_ELEMENT_R0_ID(rx_element_fifo_1.R0.bit.ID);
		canline[0].ts = CAN_RX_ELEMENT_R1_RXTS(rx_element_buffer.R1.bit.RXTS);
		
		for (i = 0; i < rx_element_fifo_1.R1.bit.DLC; i++) {
			// store data
			canline[0].data.arr[i] = rx_element_fifo_1.data[i];
		}
		//canline_i++;
		canline_1_updated = 1;
	}

	if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
	|| (status & CAN_PROTOCOL_ERROR_DATA)) {
		can_clear_interrupt_status(&can1_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
		//printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
	}
}
