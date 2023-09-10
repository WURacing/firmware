#include "app_inputs.h"
#include "drv_adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sam.h"
#include "vehicle.h"
#include "app_datalogger.h"
#include "app_data.h"
#include "drv_can.h"

#define DELAY_PERIOD 20
#define APP_INPUTS_PRIORITY 4

static const struct {
	struct app_inputs_config_digital {
		uint8_t group;
		uint8_t pin;
		uint32_t port;
		uint8_t pincfg;
		uint32_t maxActiveTimeMs;
	} digital[APP_INPUTS_DIGITAL_COUNT];
} app_inputs_config = {
	.digital = {
		[APP_INPUTS_DRS_L]		= { 0, PIN_PA00, PORT_PA00, PORT_PINCFG_INEN(1) | PORT_PINCFG_PULLEN(1), 0 },
		[APP_INPUTS_DRS_R]		= { 0, PIN_PA01, PORT_PA01, PORT_PINCFG_INEN(1) | PORT_PINCFG_PULLEN(1), 0 },
		[APP_INPUTS_FUSE]		= { 0, PIN_PA04, PORT_PA04, PORT_PINCFG_INEN(1),						 0 },
		[APP_INPUTS_MISC_L]		= { 0, PIN_PA27, PORT_PA27, PORT_PINCFG_INEN(1) | PORT_PINCFG_PULLEN(1), 0 },
		[APP_INPUTS_MISC_R]		= { 0, PIN_PA28, PORT_PA28, PORT_PINCFG_INEN(1) | PORT_PINCFG_PULLEN(1), 0 },
		[APP_INPUTS_SHIFT_DOWN]	= { 1, PIN_PB02, PORT_PB02, PORT_PINCFG_INEN(1) | PORT_PINCFG_PULLEN(1), 200 },
		[APP_INPUTS_SHIFT_UP]	= { 1, PIN_PB03, PORT_PB03, PORT_PINCFG_INEN(1) | PORT_PINCFG_PULLEN(1), 200 },
	},
};

#define STACK_SIZE 512

static struct app_inputs_data
{
	struct drv_adc_results adc_results;
	struct {
		int position, last;
	} dials[NUM_DIALS];
	struct app_inputs_data_digital {
		bool active, last;
		uint32_t lastActiveStartMs;
	} digital[APP_INPUTS_DIGITAL_COUNT];
	StaticTask_t rtos_task_id;
	StackType_t  rtos_stack[STACK_SIZE];
} app_inputs_data = {0};

int app_inputs_get_dial(enum app_inputs_analog dial)
{
	return app_inputs_data.dials[dial].position;
}

bool app_inputs_get_button(enum app_inputs_digital channel)
{
	const struct app_inputs_config_digital * config = &app_inputs_config.digital[channel];
	struct app_inputs_data_digital * data = &app_inputs_data.digital[channel];
	
	bool active = app_inputs_data.digital[channel].active;

	if ((config->maxActiveTimeMs > 0U) && (data->active) && ((xTaskGetTickCount() - data->lastActiveStartMs) >= config->maxActiveTimeMs))
	{
		active = false;
	}
	return active;
}

static int rotaryPosition(uint16_t counts)
{
	if (counts > 3909) return 1;
	if (counts > 3537) return 2;
	if (counts > 3164) return 3;
	if (counts > 2792) return 4;
	if (counts > 2419) return 5;
	if (counts > 2048) return 6;
	if (counts > 1675) return 7;
	if (counts > 1303) return 8;
	if (counts > 931) return 9;
	if (counts > 558) return 10;
	if (counts > 186) return 11;
	return 12;
}


static void app_inputs_task()
{
	// Initialize digital inputs
	for (enum app_inputs_digital channel = (enum app_inputs_digital)0U; channel < APP_INPUTS_DIGITAL_COUNT; ++channel)
	{
		const struct app_inputs_config_digital * config = &app_inputs_config.digital[channel];
		PORT_REGS->GROUP[config->group].PORT_PINCFG[config->pin & 0x1F] = config->pincfg;
		PORT_REGS->GROUP[config->group].PORT_OUTSET = config->port;
	}
	
	TickType_t xLastWakeTime = xTaskGetTickCount();
	
	while (1)
	{
		// Reading of dials
		drv_adc_read_sequence_sync(&app_inputs_data.adc_results);
		for (int i = 0; i < NUM_DIALS; ++i)
		{
			int reading = rotaryPosition(app_inputs_data.adc_results.results[i]);
			if (reading != app_inputs_data.dials[i].position)
			{
				// Debouncing
				if (reading == app_inputs_data.dials[i].last)
				{
					app_inputs_data.dials[i].position = reading;
				}
			}
			app_inputs_data.dials[i].last = reading;
		}
		
		// Reading of digital inputs
		uint32_t din[2] = { // hit registers only once
			PORT_REGS->GROUP[0].PORT_IN,
			PORT_REGS->GROUP[1].PORT_IN,
		};
		for (enum app_inputs_digital channel = (enum app_inputs_digital)0U; channel < APP_INPUTS_DIGITAL_COUNT; ++channel)
		{
			const struct app_inputs_config_digital * config = &app_inputs_config.digital[channel];
			struct app_inputs_data_digital * data = &app_inputs_data.digital[channel];
			bool reading = !(din[config->group] & config->port);
			if (reading != data->active)
			{
				// Debouncing
				if (reading == data->last)
				{
					data->active = reading;
					if (data->active)
					{
						data->lastActiveStartMs = xLastWakeTime;
					}
				}
				data->last = reading;
			}
		}
		
		// Sending of CAN message
		struct drv_can_tx_buffer_element * buffer = drv_can_get_tx_buffer(DRV_CAN_TX_BUFFER_VEHICLE_UI_INPUTS);
		if (buffer)
		{
			const struct servo_config * servos = app_datalogger_get_servo_positions();
			bool drsOpen = app_inputs_get_button(APP_INPUTS_DRS_L) || app_inputs_get_button(APP_INPUTS_DRS_R);
			struct vehicle_ui_inputs_t ui_inputs = {
				.ui_drs_command = (drsOpen) ? servos->drsOpenPulse : servos->drsClosedPulse,
				.ui_e_arb_front_setting = servos->eARBFrontPulses[app_inputs_get_dial(APP_INPUTS_SW1)-1],
				.ui_e_arb_rear_setting = servos->eARBRearPulses[app_inputs_get_dial(APP_INPUTS_SW2)-1],
				.ui_shift_down_command = app_inputs_get_button(APP_INPUTS_SHIFT_DOWN),
				.ui_shift_up_command = app_inputs_get_button(APP_INPUTS_SHIFT_UP),
			};
			vehicle_ui_inputs_pack((uint8_t *)buffer->DB, &ui_inputs, 8);
			app_data_push_fifo(buffer);
			drv_can_queue_tx_buffer(CAN0_REGS, DRV_CAN_TX_BUFFER_VEHICLE_UI_INPUTS);
		}
		
		struct drv_can_tx_buffer_element debugBuf = {
			.TXBE_0 = { .bit = {
				.ID = (VEHICLE_UI_DEBUG_FRAME_ID << 18U),
				.XTD = 0,
			}},
		};
		struct vehicle_ui_debug_t ui_debug = {
			.ui_analog_dial1_dbg = app_inputs_data.adc_results.results[0],
			.ui_analog_dial2_dbg = app_inputs_data.adc_results.results[1],
			.ui_analog_dial3_dbg = app_inputs_data.adc_results.results[2],
			.ui_analog_dial4_dbg = app_inputs_data.adc_results.results[3],
		};
		vehicle_ui_debug_pack((uint8_t *)debugBuf.DB, &ui_debug, 8);
		app_data_push_fifo(&debugBuf);
		
		vTaskDelayUntil(&xLastWakeTime, DELAY_PERIOD);
	}
	vTaskDelete(NULL);
}

void app_inputs_init(void)
{
	xTaskCreateStatic(app_inputs_task, "INPUT", STACK_SIZE, NULL, APP_INPUTS_PRIORITY, app_inputs_data.rtos_stack, &app_inputs_data.rtos_task_id);
}
