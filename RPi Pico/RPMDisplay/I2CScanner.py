# Import all board pins.
import board
import busio

# Create the I2C interface.
#                    (SCL, SDA)
i2c = busio.I2C(board.GP5, board.GP4)

print('Scan i2c bus...')
devices = i2c.scan()

if len(devices) == 0:
  print("No i2c device !")
else:
  print('i2c devices found:',len(devices))

  for device in devices:  
    print("Decimal address: ",device," | Hexa address: ",hex(device))