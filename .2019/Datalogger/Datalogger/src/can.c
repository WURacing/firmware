/*
 * can.c
 *
 * Created: 4/27/2019 7:02:06 PM
 *  Author: Connor
 */ 

#include <asf.h>
#include "util.h"

volatile int canline_updated;
volatile can_message_t canline;

struct can_module can_instance;

void configure_can(void)
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
	config_can.nonmatching_frames_action_standard = CAN_NONMATCHING_FRAMES_FIFO_0;
	config_can.nonmatching_frames_action_extended = CAN_NONMATCHING_FRAMES_FIFO_1;
	config_can.timestamp_prescaler = 0xf;
	
	canline_updated = 0;
	
	can_init(&can_instance, CAN_MODULE, &config_can);
	can_set_baudrate(CAN_MODULE, 500000);

	can_start(&can_instance);

	/* Enable interrupts for this CAN module */
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN0);
	can_enable_interrupt(&can_instance, CAN_PROTOCOL_ERROR_ARBITRATION | CAN_PROTOCOL_ERROR_DATA);
	
	// Read everything into FIFO 1
	struct can_extended_message_filter_element et_filter;
	can_get_extended_message_filter_element_default(&et_filter);
	et_filter.F1.reg = CAN_EXTENDED_MESSAGE_FILTER_ELEMENT_F1_EFID2(0) | CAN_EXTENDED_MESSAGE_FILTER_ELEMENT_F1_EFT_CLASSIC;
	can_set_rx_extended_filter(&can_instance, &et_filter, 0);
	
	can_enable_interrupt(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
	can_enable_interrupt(&can_instance, CAN_RX_FIFO_1_NEW_MESSAGE);

}


static volatile uint32_t standard_receive_index = 0;
static volatile uint32_t extended_receive_index = 0;
static struct can_rx_element_fifo_0 rx_element_fifo_0;
static struct can_rx_element_fifo_1 rx_element_fifo_1;
static struct can_rx_element_buffer rx_element_buffer;


void CAN0_Handler(void)
{
	volatile uint32_t status, i, rx_buffer_index;
	status = can_read_interrupt_status(&can_instance);


	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance, CAN_RX_BUFFER_NEW_MESSAGE);
		for (i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can_instance, i);
				can_get_rx_buffer_element(&can_instance, &rx_element_buffer,
				rx_buffer_index);
				if (rx_element_buffer.R0.bit.XTD) {
					printf("\n\r Extended message received in Rx buffer. The received data is: \r\n");
					} else {
					printf("\n\r Standard message received in Rx buffer. The received data is: \r\n");
				}
				for (i = 0; i < rx_element_buffer.R1.bit.DLC; i++) {
					printf("  %d",rx_element_buffer.data[i]);
				}
				printf("\r\n\r\n");
			}
		}
	}
	if (status & CAN_RX_FIFO_0_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
		can_get_rx_fifo_0_element(&can_instance, &rx_element_fifo_0,
		standard_receive_index);
		can_rx_fifo_acknowledge(&can_instance, 0,
		standard_receive_index);
		standard_receive_index++;
		if (standard_receive_index == CONF_CAN0_RX_FIFO_0_NUM) {
			standard_receive_index = 0;
		}
		for (i = 0; i < rx_element_fifo_0.R1.bit.DLC; i++) {
			// store data
			canline.id = CAN_RX_ELEMENT_R0_ID(rx_element_fifo_0.R0.bit.ID)>>18;
			canline.data.arr[i] = rx_element_fifo_0.data[i];
			canline.ts = CAN_RX_ELEMENT_R1_RXTS(rx_element_buffer.R1.bit.RXTS);
		}
		canline_updated = 1;
	}
	
	
	
	if (status & CAN_RX_FIFO_1_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
		can_get_rx_fifo_1_element(&can_instance, &rx_element_fifo_1,
		extended_receive_index);
		can_rx_fifo_acknowledge(&can_instance, 1,
		extended_receive_index);
		extended_receive_index++;
		if (extended_receive_index == CONF_CAN0_RX_FIFO_1_NUM) {
			extended_receive_index = 0;
		}
		for (i = 0; i < rx_element_fifo_1.R1.bit.DLC; i++) {
			// store data
			canline.id = CAN_RX_ELEMENT_R0_ID(rx_element_fifo_1.R0.bit.ID);
			canline.data.arr[i] = rx_element_fifo_1.data[i];
			canline.ts = CAN_RX_ELEMENT_R1_RXTS(rx_element_buffer.R1.bit.RXTS);
		}
		canline_updated = 1;
	}

	if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
	|| (status & CAN_PROTOCOL_ERROR_DATA)) {
		can_clear_interrupt_status(&can_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
		printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
	}
}
