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
*/
void upshift();

/*
* Performs a downshift by pulsing the downshift output pin.
*/
void downshift();

/*
* Returns the position of the clutch paddle between 0 and 1.
*/
double getClutchPaddlePosition();

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