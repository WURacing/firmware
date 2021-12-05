# Author: Shane Warga
# Purpose: Create a new python base CAN recorder and parser to replace the current 2022
# version of the DAQ board. This is meant to run on a raspberry pi running a the raspian lite linux distro
# Configuration: 
#     NOTE THIS ONLY NEEDS TO BE DONE ONCE ON A NEW PI
#     -Flash and SD card with Raspian Lite Linux
#     -power the pi and plug it into a monitor
#     -configure login information I use username:pi password:pi, i'm lazy
#     -run the command 'sudo raspi-config'
#         -this will enable the gnu configuration tool
#         -go to the interfaces menu and enable SSH, UART, and I2C
#         -for development set up a connection to wifi labeled as wireless lan
#     -install the latest version of python3 and pip3 with the linux package manager
#     -install pyserial, cantools, and pandas with pip if no requirements file
#     -pySerial uses connections defined in the /dev/ folder on linux. The UART serial ports are serial0 and serial1
#     -Cantools needs a reference to a DBC file or multiple files with multiple cantool objects
 
import cantools
import serial
import pprint from pprint
import can

# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT-0
from awscrt import io, mqtt, auth, http
from awsiot import mqtt_connection_builder
import time as t
import json

# Define ENDPOINT, CLIENT_ID, PATH_TO_CERTIFICATE, PATH_TO_PRIVATE_KEY, PATH_TO_AMAZON_ROOT_CA_1, MESSAGE, TOPIC, and RANGE
ENDPOINT = "agimxrztttugm-ats.iot.us-west-2.amazonaws.com"
CLIENT_ID = "RaspberryPi-LTE"
PATH_TO_CERTIFICATE = "certs/85057e83aaead5808eef972402f04be730d6c0b0e163e180f8db1c7d58703cb7-certificate.pem.crt"
PATH_TO_PRIVATE_KEY = "certs/85057e83aaead5808eef972402f04be730d6c0b0e163e180f8db1c7d58703cb7-private.pem.key"
PATH_TO_AMAZON_ROOT_CA_1 = "certs/AmazonRootCA1.crt"
TOPIC = "Telemetry"

#create logging folder and file
vehicle_db = cantools.database.load_file("VEHICLE.dbc")
pe3_db = cantools.database.load_file("PE3.dbc")
log_path = './LOGS'
if(os.path.isdir(self.log_path) == False):
    os.mkdir('LOGS')
moment=time.strftime("%Y-%b-%d__%H_%M",time.localtime())
log_file_path = self.log_path +'/CAN_'+moment+'.log'


#set up CAN connections

bus0 = can.interface.Bus(channel='can0', bustype='socketcan_native',fd = True)
#bus1 = can.interface.Bus(channel='can1', bustype='socketcan_native',fd = True)
logger = can.Logger(log_file_path, 'a')
notifier = can.Notifier(bus0, [can.Printer(), logger])

# Spin up resources
event_loop_group = io.EventLoopGroup(1)
host_resolver = io.DefaultHostResolver(event_loop_group)
client_bootstrap = io.ClientBootstrap(event_loop_group, host_resolver)
mqtt_connection = mqtt_connection_builder.mtls_from_path(
            endpoint=ENDPOINT,
            cert_filepath=PATH_TO_CERTIFICATE,
            pri_key_filepath=PATH_TO_PRIVATE_KEY,
            client_bootstrap=client_bootstrap,
            ca_filepath=PATH_TO_AMAZON_ROOT_CA_1,
            client_id=CLIENT_ID,
            clean_session=False,
            keep_alive_secs=6
            )
print("Connecting to {} with client ID '{}'...".format(
        ENDPOINT, CLIENT_ID))
# Make the connect() call
connect_future = mqtt_connection.connect()
# Future.result() waits until a result is available
connect_future.result()
print("Connected!")

# Publish message to server desired number of times.
print('Begin Publish')
for i in range (RANGE):
    data = "{} [{}]".format(MESSAGE, i+1)
    message = {"message" : data}
    mqtt_connection.publish(topic=TOPIC, payload=json.dumps(message), qos=mqtt.QoS.AT_LEAST_ONCE)
    print("Published: '" + json.dumps(message) + "' to the topic: " + "'test/testing'")
    t.sleep(0.1)
print('Publish End')
disconnect_future = mqtt_connection.disconnect()
disconnect_future.result()
