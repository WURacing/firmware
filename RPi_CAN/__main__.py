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
import can
import os
import sys
import threading
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT-0
from awscrt import io, mqtt, auth, http
from awsiot import mqtt_connection_builder
import time as t
import json

# Define ENDPOINT, CLIENT_ID, PATH_TO_CERTIFICATE, PATH_TO_PRIVATE_KEY, PATH_TO_AMAZON_ROOT_CA_1, MESSAGE, TOPIC, and RANGE

#create logging folder and file

# Publish message to server desired number of times.
class LTE_Listener():
    def __init__(self, id, target):
        self.target = target
        self.lock = threading.Lock()
        self.TOPIC = "telemetry/{}/data".format(id)
        self.ENDPOINT = "agimxrztttugm-ats.iot.us-west-2.amazonaws.com"
        self.CLIENT_ID = "RaspberryPi-LTE"
        self.PATH_TO_CERTIFICATE = "certs/85057e83aaead5808eef972402f04be730d6c0b0e163e180f8db1c7d58703cb7-certificate.pem.crt"
        self.PATH_TO_PRIVATE_KEY = "certs/85057e83aaead5808eef972402f04be730d6c0b0e163e180f8db1c7d58703cb7-private.pem.key"
        self.PATH_TO_AMAZON_ROOT_CA_1 = "certs/AmazonRootCA1.crt"
        self.event_loop_group = io.EventLoopGroup(1)
        self.host_resolver = io.DefaultHostResolver(self.event_loop_group)
        self.client_bootstrap = io.ClientBootstrap(self.event_loop_group, self.host_resolver)
        self.mqtt_connection = mqtt_connection_builder.mtls_from_path(
                    endpoint=self.ENDPOINT,
                    cert_filepath=self.PATH_TO_CERTIFICATE,
                    pri_key_filepath=self.PATH_TO_PRIVATE_KEY,
                    client_bootstrap=self.client_bootstrap,
                    ca_filepath=self.PATH_TO_AMAZON_ROOT_CA_1,
                    client_id=self.CLIENT_ID,
                    clean_session=False,
                    keep_alive_secs=6
                    )
        print("Connecting to {} with client ID '{}'...".format(
                self.ENDPOINT, self.CLIENT_ID))
        # Make the connect() call
        self.connect_future = self.mqtt_connection.connect()
        # Future.result() waits until a result is available
        self.connect_future.result()
        print("Connected!")

    def __call__(self, message):
        if(self.lock.locked()):
            print("locked, passing")
            return
        self.lock.acquire()
        message = {"timestamp":message.timestamp,"arbitration_id":message.arbitration_id,"dlc": message.dlc,"data":int.from_bytes(message.data, "little") ,"dbc_target": self.target}
        self.mqtt_connection.publish(topic=self.TOPIC, payload=json.dumps(message), qos=mqtt.QoS.AT_LEAST_ONCE)
        self.lock.release()

if __name__ == "__main__":
    vehicle_db = cantools.database.load_file("VEHICLE.dbc")
    pe3_db = cantools.database.load_file("PE3.dbc")
    log_path = './LOGS'
    if(os.path.isdir(log_path) == False):
        os.mkdir('LOGS')
    moment=t.strftime("%Y-%b-%d__%H_%M",t.localtime())
    log_file_path_pe3 = log_path +'/CAN_'+moment+"_PE3"+'.log'
    log_file_path_vehicle = log_path +'/CAN_'+moment+"_VEHICLE"+'.log'
    #set up CAN connections
    bus0 = can.interface.Bus(channel='can0', bustype='socketcan_native',fd = True)
    bus1 = can.interface.Bus(channel='can1', bustype='socketcan_native',fd = True)
    #set lte listeners
    lte0 = LTE_Listener(0, "P21")
    lte1 = LTE_Listener(1, "V21")
    logger_vehicle = can.Logger(log_file_path_vehicle, 'a')
    logger_pe3 = can.Logger(log_file_path_pe3, 'a')
    notifier0 = can.Notifier(bus0, [logger_pe3, lte0])
    notifier1 = can.Notifier(bus1, [logger_vehicle, lte1])
    try:
        while(True):
            time  = t.time()
            if (t.time() >= time + 1):
                logger_pe3.file.close()
                logger_vehicle.file.close()
                logger_pe3.file.open()
                logger_vehicle.file.open()
                time = t.time()
    except KeyboardInterrupt:
        print('Interrupted')
        notifier0.stop()
        notifier1.stop()
        bus0.shutdown()
        bus1.shutdown()
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)

