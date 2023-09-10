/*
 * app_main.c
 *
 * Created: 7/27/2020 9:25:45 PM
 *  Author: connor
 */ 

#include "app_main.h"
#include "app_statusLight.h"
#include "app_cantx.h"
#include "app_telemetry.h"

void app_init(void)
{	
	app_statusLight_init();
	app_cantx_init();
	app_telemetry_init();
}

void app_periodic(void)
{	
	app_statusLight_periodic();
	app_cantx_periodic();
	app_telemetry_periodic();
}