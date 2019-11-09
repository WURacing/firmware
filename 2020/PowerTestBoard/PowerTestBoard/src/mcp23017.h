/*
 * mcp23017.h
 *
 * Created: 11/9/2019 12:31:30 PM
 *  Author: connor
 */ 


#ifndef MCP23017_H_
#define MCP23017_H_

void configure_i2c(void);
int initialize_mcp23017(void);
int mcp23017_send_data(uint8_t dataA, uint8_t dataB);



#endif /* MCP23017_H_ */