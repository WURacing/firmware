from struct import unpack
# >HHHH specifies our format, in this case we specify reading the data as four unsigned shorts in big endian
# b'\xbe\x05\xbe\x05\x00\x00\x00\xff specifies our data 'be05be05000000ff' given in byte format
data = unpack('>HHHH', bytes(b'\xbe\x05\xbe\x05\x00\x00\x00\xff'))
print("Ex: " + data)
if data[2] == 255:
    print("upshift")
if data[3] == 255:
    print("downshift")

shiftDownCommand = True
shiftUpCommand = False

# Bingus Struct
bingus = unpack('>??bbb', )
print("Bingus: " + bingus)