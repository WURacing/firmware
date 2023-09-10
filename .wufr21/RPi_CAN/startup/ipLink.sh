#!/bin/bash

#run the <sudo crontab -e> command, this changes the root user cron files so no sudo needed in this script
#once you can edit the crontab file add the following line
# @reboot <path to this file>
#save and reboot the pi. should set up can0 and can1 link
#to test if this worked properly run the command <candump can0>, or <candump can1> if it is not connected you will get a network error, otherwise it will block


/sbin/ip link set can0 up type can bitrate 500000 dbitrate 2000000 fd on sample-point .8 dsample-point .8
/sbin/ip link set can1 up type can bitrate 500000 dbitrate 2000000 fd on smaple-point .8 dsample-point .8

