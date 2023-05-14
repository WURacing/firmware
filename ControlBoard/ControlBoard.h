/*
* Blinks the LED on the board to indicate that the board is running.
*/
void blink();

/*
* Checks the shift paddles and updates their rolling data.
*/
void checkShiftPaddles(unsigned long &upData, unsigned long &downData, unsigned short &dataCount);

/*
* Performs an upshift by pulsing the upshift output pin.
* @param pulse The length of the pulse in microseconds.
*/
void upshift(int pulse);

/*
* Performs a downshift by pulsing the downshift output pin.
* @param pulse The length of the pulse in microseconds.
*/
void downshift(int pulse);

/*
* Performs a neutral shift by pulsing half of a upshift or downshift.
*/
void neutralshift(int gearPos);

/*
* Returns the maximum position of the clutch paddles between 0 and 1.
*/
double getClutchPaddlePosition();

/*
* Gets the positions of the clutch paddles and stores them in the passed variables.
*/
void getClutchPaddlePositions(double &clutch1, double &clutch2);

/*
* Sets the position of the clutch.
*/
void setClutchPosition(unsigned int position);

/*
* Changes the bit at position c in data b to value v
*/
void modifyBit(unsigned long &d, unsigned short c, bool v);

/*
* Updates the gear position from the CAN bus.
*/
void updateGearPosition(unsigned short &gearPos);

/*
* Checks the DRS button and updates its rolling data.
*/
void getDRSButtonState(unsigned long &drsData, unsigned short &dataCount);

/*
* Sets the DRS servo to open or close. 
* true = open, false = close
*/
void setDRS(bool state);

/*
* Checks if the car is not shifting.
*/
bool isNotShifting();