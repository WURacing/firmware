#include "app_sensor.h"
#include "app_statuslight.h"

void app_init(void)
{
	app_sensor_init();
	app_statuslight_init();
}
