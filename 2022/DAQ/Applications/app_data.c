#include "app_data.h"
#include "FreeRTOS.h"
#include "task.h"
#include "drv_can.h"
#include "semphr.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

#define MISSING_FOR 1000
#define SYNC_INTERVAL 1000
#define DELAY_PERIOD 4



#define WRITE_QUEUE_SIZE 4
#define MESSAGE_BUFFER_SIZE 32

// Raw CAN data
static struct {
	struct app_data_message buffer[MESSAGE_BUFFER_SIZE];
	struct app_data_message fifo[WRITE_QUEUE_SIZE];
	struct drv_can_rx_fifo_0_element fifo_tmp;
	TickType_t tscv_epoch, prev_epoch, last_read;
	volatile uint8_t fifo_wp, fifo_rp;
} app_data_data = {0};


static inline bool is_missing(int i)
{
	return (xTaskGetTickCount() - app_data_data.buffer[i].timestamp_ms) > MISSING_FOR;
}

bool app_data_is_missing(int frame_id)
{
	for (int i = 0; i < MESSAGE_BUFFER_SIZE && app_data_data.buffer[i].id != 0; ++i)
	{
		if (app_data_data.buffer[i].id == frame_id)
		{
			return is_missing(i);
		}
	}
	return true;
}

bool app_data_read_message(int frame_id, struct app_data_message * output)
{
	for (int i = 0; i < MESSAGE_BUFFER_SIZE && app_data_data.buffer[i].id != 0; ++i)
	{
		if (app_data_data.buffer[i].id == frame_id)
		{
			if (is_missing(i))
			{
				return false;
			}
			else
			{
				memcpy(output, &app_data_data.buffer[i], sizeof(struct app_data_message));
				return true;
			}
		}
	}
	return false;
}

bool app_data_read_buffer(int i, struct app_data_message * output)
{
	if (i < MESSAGE_BUFFER_SIZE && app_data_data.buffer[i].id != 0)
	{
		memcpy(output, &app_data_data.buffer[i], sizeof(struct app_data_message));
		return true;
	}
	return false;
}

static inline int rxts_to_ms()
{
	return (app_data_data.fifo_tmp.RXF0E_1.bit.RXTS * 4 / 125);
}

static const struct app_data_message * insert_into_buffer()
{
	int id = app_data_data.fifo_tmp.RXF0E_0.bit.ID;
	if (!app_data_data.fifo_tmp.RXF0E_0.bit.XTD)
		id = (id >> 18) & 0x7FF;
	for (int i = 0; i < MESSAGE_BUFFER_SIZE; ++i)
	{
		if (app_data_data.buffer[i].id == id || app_data_data.buffer[i].id == 0)
		{
			app_data_data.buffer[i].id = id;
			int ms = rxts_to_ms();
			if ((xTaskGetTickCount() - app_data_data.tscv_epoch) < 2000)
			{
				// Get highly accurate tick count
				if (app_data_data.tscv_epoch + ms > xTaskGetTickCount() + 10)
				{
					// If the calculated time is in the future, we should use the old epoch
					app_data_data.buffer[i].timestamp_ms = app_data_data.prev_epoch + ms;
				}
				else
				{
					app_data_data.buffer[i].timestamp_ms = app_data_data.tscv_epoch + ms;
				}
			}
			else
			{
				// No hope
				app_data_data.buffer[i].timestamp_ms = xTaskGetTickCount();
			}
			if (app_data_data.buffer[i].timestamp_ms < app_data_data.last_read)
			{
				// We want the timestamps to be monotonically increasing, there's a slight bug with RXTS when its reset
				app_data_data.buffer[i].timestamp_ms = app_data_data.last_read;
			}
			app_data_data.last_read = app_data_data.buffer[i].timestamp_ms;
			memcpy(app_data_data.buffer[i].data, (const uint8_t *)app_data_data.fifo_tmp.DB, 8);
			return &app_data_data.buffer[i];
		}
	}
	return NULL;
}

static inline void reset_tscv_epoch(void)
{
	// Don't reset more often than once a second
	if (xTaskGetTickCount() - app_data_data.tscv_epoch > 1000)
	{
		drv_can_reset_timestamp(CAN0_REGS);
		drv_can_reset_timestamp(CAN1_REGS);
		app_data_data.prev_epoch = app_data_data.tscv_epoch;
		app_data_data.tscv_epoch = xTaskGetTickCount();
	}
}
const struct app_data_message * app_data_pop_fifo(void)
{
	const struct app_data_message * result;
	// Error recovery
	if (drv_can_is_bus_off(CAN0_REGS))
	{
		drv_can_recover_from_bus_off(CAN0_REGS);
	}
	if (drv_can_is_bus_off(CAN1_REGS))
	{
		drv_can_recover_from_bus_off(CAN1_REGS);
	}
	if (drv_can_pop_fifo_0(CAN0_REGS, &app_data_data.fifo_tmp))
	{
		if ((result = insert_into_buffer()) != NULL)
			return result;
	}
	if (drv_can_pop_fifo_0(CAN1_REGS, &app_data_data.fifo_tmp))
	{
		if ((result = insert_into_buffer()) != NULL)
			return result;
	}
	const uint8_t curr_rp = app_data_data.fifo_rp;
	if (curr_rp != app_data_data.fifo_wp) // something to read
	{
		result = &app_data_data.fifo[curr_rp];
		app_data_data.fifo_rp = (curr_rp + 1) % WRITE_QUEUE_SIZE;
		return result;
	}
	
	// No messages to read, might as well clean up
	reset_tscv_epoch();
	return NULL;
}

void app_data_push_fifo(const struct drv_can_tx_buffer_element * element)
{
	const uint8_t curr_wp = app_data_data.fifo_wp;
	const uint8_t next_wp = (curr_wp + 1) % WRITE_QUEUE_SIZE;
	if ((next_wp != app_data_data.fifo_rp) && (element != NULL)) // still has space left
	{
		int id = element->TXBE_0.bit.ID;
		if (!element->TXBE_0.bit.XTD)
			id = (id >> 18) & 0x7FF;

		app_data_data.fifo[curr_wp].id = id;
		app_data_data.fifo[curr_wp].timestamp_ms = xTaskGetTickCount();
		memcpy(app_data_data.fifo[curr_wp].data, (const uint8_t *)element->DB, 8);

		app_data_data.fifo_wp = next_wp;
	}
}