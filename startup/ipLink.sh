#!/bin/bash

#This goes in the /etc/init.d directory with 755 access

sudo /sbin/ip link set can0 up type can bitrate 500000 dbitrate 2000000 fd on sample-point .8 dsample-point .8
sudo /sbin/ip link set can1 up type can bitrate 500000 dbitrate 2000000 fd on sample-point .8 dsample-point .8
