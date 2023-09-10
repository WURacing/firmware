/*
 * app_telemetry.c
 *
 * Created: 8/16/2020 11:41:00 AM
 *  Author: connor
 */ 

#include "app_telemetry.h"
#include "drv_lte.h"
#include "drv_can.h"
#include "FreeRTOS.h"
#include "task.h"
#define ubyte uint8_t
#define uword uint16_t
#define ulong uint32_t
#include "2020.1.0.h"

struct transmit_message 
{
	uint32_t id, ts;
	uint8_t data[8];
	uint8_t checksum;
	char word[3];
};

static void app_telemetry_read_and_queue_messages(void);

static TickType_t timer4Hz = 0;
static int totalSentMessages = 0;

void app_telemetry_init(void)
{
	
}

void app_telemetry_periodic(void)
{
	TickType_t time = xTaskGetTickCount();
		
	if (time >= timer4Hz)   //1Hz
	{
		timer4Hz = time + 250;
		app_telemetry_read_and_queue_messages();
	}
}

int app_telemetry_sent_messages(void)
{
	return totalSentMessages;
}

static inline unsigned htonl(unsigned input)
{
	return ((input & 0xFF) << 24) | ((input & 0xFF00) << 8) | ((input & 0xFF0000) >> 8) | ((input & 0xFF000000) >> 24);
}

static inline void build_msg(struct transmit_message * tx_buf, int id, int ts, const uint8_t *data)
{
	tx_buf->id = htonl(id);
	tx_buf->ts = htonl(ts);
	tx_buf->checksum = 0;
	for (int j = 0; j < 8; ++j)
	{
		tx_buf->data[j] = data[j];
		tx_buf->checksum ^= data[j];
	}
	tx_buf->word[0] = 'W';
	tx_buf->word[1] = 'U';
	tx_buf->word[2] = '\n';
}

static void app_telemetry_read_and_queue_messages(void)
{
	struct transmit_message * tx_buf = (struct transmit_message *) drv_lte_get_transmission_queue();
	int length = 0;
	
	if (tx_buf != NULL)
	{
		for (enum drv_can_rx_buffer_table i = (enum drv_can_rx_buffer_table)0U; i < DRV_CAN_RX_BUFFER_COUNT; ++i)
		{
			if (drv_can_check_rx_buffer(CAN0_REGS, (int) i))
			{
				struct drv_can_rx_buffer_element * can_buf = drv_can_get_rx_buffer((int) i);
				int id;
				if (can_buf->RXBE_0.bit.XTD)
				{
					id = can_buf->RXBE_0.bit.ID;
				}
				else
				{
					id = can_buf->RXBE_0.bit.ID >> 18;
				}
				build_msg(tx_buf, id, can_buf->RXBE_1.bit.RXTS, (const uint8_t *)can_buf->DB);
				drv_can_clear_rx_buffer(CAN0_REGS, (int) i);
				length++;
				tx_buf++;
				totalSentMessages++;
			}
		}
		if (drv_lte_get_last_time() != NULL)
		{
			build_msg(tx_buf, ID_GPS_time, 0, (const uint8_t *)drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_CAN0_GPS_time)->DB);
			length++;
			tx_buf++;
			totalSentMessages++;
		}
		if (drv_lte_get_last_location() != NULL)
		{
			build_msg(tx_buf, ID_GPS_POS, 0, (const uint8_t *)drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_CAN0_GPS_POS)->DB);
			length++;
			tx_buf++;
			totalSentMessages++;
		}
		if (length > 0)
		{
			drv_lte_queue_transmission(length * sizeof(struct transmit_message));
		}
		else
		{
			drv_lte_cancel_transmission();
		}
	}
	
}

