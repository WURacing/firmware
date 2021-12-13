# uPy w/ CANPico SDK compatible

from rp2 import CAN, CANID, CANFrame

c = CAN()
frame_id = CANID(0)
frame_data = b'hello'
frame = CANFrame(frame_id,data=frame_data)
