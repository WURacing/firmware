import struct
import datetime
import functools
import socket
import sys

# Create a UDP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the port
server_address = ('0', 10000)
print('starting up on {} port {}'.format(*server_address))
sock.bind(server_address)


while True:
    data, addr = sock.recvfrom(20)
    now = datetime.datetime.now()
    print("RX message ")
    print(addr)
    print(data)
    buf = struct.unpack("!LlBBBBBBBBB3s", data)
    mid = buf[0]
    ts = buf[1]
    data = buf[2:10]
    cs = buf[10]
    word = buf[11]
    if word != b'WU\n':
        print("invalid")
        continue

    print(data)
    print(functools.reduce(lambda x,y: x ^ y, data))
    print(cs)
    if functools.reduce(lambda x,y: x ^ y, data) ^ cs != 0:
        print("bad CS")
        continue

    time = now + datetime.timedelta(milliseconds=ts)

    print(f"CAN message received at {time} and id {mid}")


