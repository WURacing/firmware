import time
import os
from zlib import compress:

class CanRecorder():
    log_path = './LOGS'
    def __init__(self):
        if(os.path.isdir(self.log_path) == False):
            os.mkdir('LOGS')
        moment=time.strftime("%Y-%b-%d__%H_%M",time.localtime())
        self.log_file_path = self.log_path +'/CAN_'+moment+'.log'
        self.log_file = open(self.log_file_path, 'a')
        self.log_file.write('time,id,data\n')
    
    def __del__(self):
        self.log_file.close()
        print(self.log_file_path + " has been created")
    
    def writeCanFrame(self, frame, time):
        #frame is byte encoded and will be saved as an int so frame needs to be converted
        frame_zip = compress(frame)
        frame_id = "01"
        self.log_file.write(str(time)+ "," + str(frame_id) + "," + str(frame_zip) + '\n')