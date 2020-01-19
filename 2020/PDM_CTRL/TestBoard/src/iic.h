/*
 * iic.h
 *
 * Created: 1/18/2020 6:36:07 PM
 *  Author: etshr
 */ 


#ifndef IIC_H_
#define IIC_H_

#include <asf.h>

#define TMP75A_PINMUX_PAD0 PINMUX_PA22C_SERCOM3_PAD0 // SDA
#define TMP75A_PINMUX_PAD1 PINMUX_PA23C_SERCOM3_PAD1 // SCL
#define TMP75A_MODULE SERCOM3


extern struct i2c_master_module i2c_master_instance;

void configure_i2c(void);

enum status_code write_packet_with_timeout(struct i2c_master_packet *i2cpacket, int timeout);

enum status_code read_packet_with_timeout(struct i2c_master_packet *i2cpacket, int timeout);

enum status_code write_packet_with_timeout_no_stop(struct i2c_master_packet *i2cpacket, int timeout);

enum status_code read_packet_with_timeout_no_stop(struct i2c_master_packet *i2cpacket, int timeout);



#endif /* IIC_H_ */