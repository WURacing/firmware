/*
 * app_main.c
 *
 * Created: 7/11/2020 7:32:50 PM
 *  Author: connor
 */ 

#include "app_main.h"
#include "app_statusLight.h"
#include "app_cantx.h"
#include "app_sensorRead.h"

void app_init(void)
{
	app_sensorRead_init();
	app_statusLight_init();
	app_cantx_init();
}

void app_periodic(void)
{
	app_sensorRead_periodic();
	app_statusLight_periodic();
	app_cantx_periodic();
}
