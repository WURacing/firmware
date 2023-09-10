#include "app_actuator.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sam.h"
#include "drv_can.h"
#include "vehicle.h"
#include "drv_tcc.h"

#define SHIFT_UP_PORT PORT_PA02
#define SHIFT_DOWN_PORT PORT_PA03

#define MISSING_TIME 200

static struct {
	struct vehicle_ui_inputs_t ui_inputs;
	TickType_t last_signal;
} app_actuator_data;

static inline int servo_degrees_to_us(int deg)
{
	return deg * 35 / 4 + 1500;
}


#define DELAY_PERIOD 25U
#define DELAY_STARTUP 200U



xTaskHandle ControlTaskID;
static void ControlTask()
{
	TickType_t xLastWakeTime;
	struct drv_can_rx_buffer_element * buffer;
	xLastWakeTime = xTaskGetTickCount();
	
	// Wait a bit for dash to come online
	vTaskDelayUntil(&xLastWakeTime, DELAY_STARTUP);
	
	while (1)
	{ 
		// Fault recovery
		if (drv_can_is_bus_off(CAN0_REGS))
		{
			drv_can_recover_from_bus_off(CAN0_REGS);
		}
		// Read CAN signal
		if (drv_can_check_rx_buffer(CAN0_REGS, DRV_CAN_RX_BUFFER_VEHICLE_UI_INPUTS))
		{
			buffer = drv_can_get_rx_buffer(DRV_CAN_RX_BUFFER_VEHICLE_UI_INPUTS);
			// Decode
			vehicle_ui_inputs_unpack(&app_actuator_data.ui_inputs, (uint8_t *)buffer->DB, 8);
			app_actuator_data.last_signal = xLastWakeTime;
			// Let CAN listen for next command
			drv_can_clear_rx_buffer(CAN0_REGS, DRV_CAN_RX_BUFFER_VEHICLE_UI_INPUTS);
			
			// Check whether message is well-formed and send servo commands if so
			if (vehicle_ui_inputs_ui_e_arb_front_setting_is_in_range(app_actuator_data.ui_inputs.ui_e_arb_front_setting))
			{
				int us = servo_degrees_to_us(app_actuator_data.ui_inputs.ui_e_arb_front_setting);
				drv_tcc_set_cc(DRV_TCC_CHANNEL_ARB, DRV_TCC_CHANNEL_ARB_CC_FRONT, us);
			}
			if (vehicle_ui_inputs_ui_e_arb_rear_setting_is_in_range(app_actuator_data.ui_inputs.ui_e_arb_rear_setting))
			{
				int us = servo_degrees_to_us(app_actuator_data.ui_inputs.ui_e_arb_rear_setting);
				drv_tcc_set_cc(DRV_TCC_CHANNEL_ARB, DRV_TCC_CHANNEL_ARB_CC_REAR, us);
			}
			if (vehicle_ui_inputs_ui_drs_command_is_in_range(app_actuator_data.ui_inputs.ui_drs_command))
			{
				int us = servo_degrees_to_us(app_actuator_data.ui_inputs.ui_drs_command);
				drv_tcc_set_cc(DRV_TCC_CHANNEL_DRS, DRV_TCC_CHANNEL_DRS_CC_REAR, us);
			}
			// Update shift actions
			uint32_t outset = 0;
			uint32_t outclr = 0;
			if (app_actuator_data.ui_inputs.ui_shift_up_command)
				outset |= SHIFT_UP_PORT;
			else
				outclr |= SHIFT_UP_PORT;
			if (app_actuator_data.ui_inputs.ui_shift_down_command)
				outset |= SHIFT_DOWN_PORT;
			else
				outclr |= SHIFT_DOWN_PORT;
			PORT_REGS->GROUP[0].PORT_OUTSET = outset;
			PORT_REGS->GROUP[0].PORT_OUTCLR = outclr;
		}
		else if (app_actuator_is_signal_missing())
		{
			// Return outputs to safety positions
			PORT_REGS->GROUP[0].PORT_OUTCLR = SHIFT_UP_PORT | SHIFT_DOWN_PORT;
			// Don't change the positions of the servos per discussion with Chris, just keep sending the last command
		}

		// Wait until next scheduled measurement time
		vTaskDelayUntil(&xLastWakeTime, DELAY_PERIOD);
	}
	
	vTaskDelete(NULL);
}

void app_actuator_init(void)
{
	// Set up GPIO for shift outputs
	PORT_REGS->GROUP[0].PORT_DIRSET = SHIFT_UP_PORT | SHIFT_DOWN_PORT;
	// Outputs high side triggering
	PORT_REGS->GROUP[0].PORT_OUTCLR = SHIFT_UP_PORT | SHIFT_DOWN_PORT;
	xTaskCreate(ControlTask, "CTRL", configMINIMAL_STACK_SIZE + 1000, NULL, 1, &ControlTaskID);
}

bool app_actuator_is_signal_missing(void)
{
	return ((xTaskGetTickCount() - app_actuator_data.last_signal) > MISSING_TIME)
			|| (app_actuator_data.last_signal == 0);
}
