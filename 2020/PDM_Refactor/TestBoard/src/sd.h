/*
 * sd.h
 *
 * Created: 1/8/2020 3:25:04 PM
 *  Author: etshr
 */ 

#include <asf.h>
#include <stdint.h>

#ifndef SD_H_
#define SD_H_

typedef struct {
	FIL file;
	char buf[2200];
	uint16_t buf_len = 0;
} sd_instance;

void shift_array_left(char *arr, const size_t size, const size_t bits);

int sd_create_file(struct sd_instance *const sd_inst);

int sd_write(char *data, struct sd_instance *const sd_inst);

void sd_close(struct sd_instance *const sd_inst);

extern volatile sd_instance sd_inst;


#endif /* SD_H_ */