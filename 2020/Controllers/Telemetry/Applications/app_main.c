/*
 * app_main.c
 *
 * Created: 7/27/2020 9:25:45 PM
 *  Author: connor
 */ 

#include "app_main.h"
#include "app_statusLight.h"

void app_init(void)
{	
	app_statusLight_init();
}

void app_periodic(void)
{	
	app_statusLight_periodic();
}