/*
 * can.c
 *
 * Created: 1/20/2020 1:43:42 PM
 *  Author: Connor
 */ 

#include "exports.h"

struct can_module can_instance[2];

static volatile int can_i_status[2];

void configure_can(void)
{
	can_i_status[0] = can_i_status[1] = 0;
	
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
	struct can_config config_can;
	can_get_config_defaults(&config_can);
	config_can.nonmatching_frames_action_standard = CAN_NONMATCHING_FRAMES_FIFO_0;
	config_can.nonmatching_frames_action_extended = CAN_NONMATCHING_FRAMES_FIFO_1;
	
	can_init(&can_instance[0], CAN0_MODULE, &config_can);
	can_set_baudrate(CAN0_MODULE, CAN0_BAUD);
	can_start(&can_instance[0]);

	can_init(&can_instance[1], CAN1_MODULE, &config_can);
	can_set_baudrate(CAN1_MODULE, CAN1_BAUD);
	can_start(&can_instance[1]);
	
	// Enable interrupts for this CAN module
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN0);
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN1);
	can_enable_interrupt(&can_instance[0], CAN_PROTOCOL_ERROR_ARBITRATION | CAN_PROTOCOL_ERROR_DATA);
	can_enable_interrupt(&can_instance[1], CAN_PROTOCOL_ERROR_ARBITRATION | CAN_PROTOCOL_ERROR_DATA);
	
	can_enable_interrupt(&can_instance[0], CAN_RX_FIFO_0_NEW_MESSAGE);
	can_enable_interrupt(&can_instance[0], CAN_RX_FIFO_1_NEW_MESSAGE);
	can_enable_interrupt(&can_instance[1], CAN_RX_FIFO_0_NEW_MESSAGE);
	can_enable_interrupt(&can_instance[1], CAN_RX_FIFO_1_NEW_MESSAGE);
}

static void extract_message(struct can_rx_element_buffer *buf, struct can_message *output)
{
	int i;
	output->id = CAN_RX_ELEMENT_R0_ID(buf->R0.bit.ID);
	output->ts = CAN_RX_ELEMENT_R1_RXTS(buf->R1.bit.RXTS);
	for (i = 0; i < 8; ++i)
	{
		if (i >= buf->R1.bit.DLC)
			output->data[i] = 0;
		else
			output->data[i] = buf->data[i];
	}
}

static enum read_message_status read_message_module(struct can_module *module, struct can_message *output)
{
	uint32_t i;
	struct can_rx_element_buffer buf;
	volatile CAN_RXF0S_Type rxfifos0;
	volatile CAN_RXF1S_Type rxfifos1;
	
	
	for (i = 0; i < CONF_CAN0_RX_BUFFER_NUM; ++i)
	{
		if (can_rx_get_buffer_status(module, i))
		{
			can_rx_clear_buffer_status(module, i);
			if (can_get_rx_buffer_element(module, &buf, i) != STATUS_OK)
				return READ_ERROR;
			extract_message(&buf, output);
			return READ_ONE;
		}
	}
	

	// start reading from "get index" of FIFO, first unread
	rxfifos0.reg = can_rx_get_fifo_status(module, 0);
	if (rxfifos0.bit.F0FL > 0) // fill level
	{
		if (can_get_rx_fifo_0_element(module, &buf, rxfifos0.bit.F0GI) != STATUS_OK)
			return READ_ERROR;
		can_rx_fifo_acknowledge(module, 0, rxfifos0.bit.F0GI);
		extract_message(&buf, output);
		return READ_ONE;
	}

	// start reading from "get index" of FIFO, first unread
	rxfifos1.reg = can_rx_get_fifo_status(module, 1);
	if (rxfifos1.bit.F1FL > 0) // fill level
	{
		if (can_get_rx_fifo_1_element(module, &buf, rxfifos1.bit.F1GI) != STATUS_OK)
			return READ_ERROR;
		can_rx_fifo_acknowledge(module, 1, rxfifos1.bit.F1GI);
		extract_message(&buf, output);
		return READ_ONE;
	}
	return READ_NONE;
}

enum read_message_status read_message(struct can_message *output)
{
	int ret;
	ret = read_message_module(&can_instance[0], output);
	if (ret != READ_NONE) return ret;
	ret = read_message_module(&can_instance[1], output);
	return ret;
}

void CAN0_Handler(void)
{
	can_i_status[0] |= can_read_interrupt_status(&can_instance[0]);
	can_clear_interrupt_status(&can_instance[0], CAN_RX_BUFFER_NEW_MESSAGE | CAN_RX_FIFO_0_NEW_MESSAGE | CAN_RX_FIFO_1_NEW_MESSAGE | CAN_PROTOCOL_ERROR_ARBITRATION | CAN_PROTOCOL_ERROR_DATA);
	xSemaphoreGiveFromISR(new_data_semaphore, NULL);
}

void CAN1_Handler(void)
{
	can_i_status[1] |= can_read_interrupt_status(&can_instance[1]);
	can_clear_interrupt_status(&can_instance[1], CAN_RX_BUFFER_NEW_MESSAGE | CAN_RX_FIFO_0_NEW_MESSAGE | CAN_RX_FIFO_1_NEW_MESSAGE | CAN_PROTOCOL_ERROR_ARBITRATION | CAN_PROTOCOL_ERROR_DATA);
	xSemaphoreGiveFromISR(new_data_semaphore, NULL);
}