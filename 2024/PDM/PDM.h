
/*
* Gets current measurement from sensor at pin as 16-bit unsigned integer
* @param pin Pin identifier for current sensor
*/
uint16_t currSense(int pin);

/*
* Sets relay state using SPI communication
* @param fd File descriptor associated with SPI device
* @param relayState Desired relay state
*/
void setRelayState(int fd, uint8_t relayState);

/*
* Initializes SPI device with SPI mode, bits per word, SPI speed
*
* @return 0 if successful
*/
int relay();

/*
* Continuously reads measurements from temperature sensor and converts to integer values
*/
void temperature();

