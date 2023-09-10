#include "app_actuator.h"
#include "app_statuslight.h"

void app_init(void)
{
	app_actuator_init();
	app_statuslight_init();
}
