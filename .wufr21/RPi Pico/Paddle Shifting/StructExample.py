from struct import unpack
# >HHHH specifies our format, in this case we specify reading the data as four unsigned shorts in big endian
# b'\xbe\x05\xbe\x05\x00\x00\x00\xff specifies our data 'be05be05000000ff' given in byte format
data = unpack('>HHHH', bytes(b'\xbe\x05\xbe\x05\x00\x00\x00\xff'))
print("Ex: " + str(data))
if data[2] == 255:
    print("upshift")
if data[3] == 255:
    print("downshift")

shiftUpDown = b'\x01'
eARBFrontSetting = b'\x26'
eARBRearSetting = b'\x04'
DRSCommand = b'\xf0'

bingusData = bytes(shiftUpDown + eARBFrontSetting + eARBRearSetting + DRSCommand)


# Bingus Struct
bingus = unpack('>bbbb', bingusData)
print("Bingus: " + str(bingus))