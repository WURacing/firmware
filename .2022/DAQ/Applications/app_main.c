#include "app_display.h"
#include "app_inputs.h"
#include "app_datalogger.h"
#include "app_telemetry.h"

void app_init(void)
{
	app_display_init();
	app_inputs_init();
	app_datalogger_init();
	app_telemetry_init();
}
