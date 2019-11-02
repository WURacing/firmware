# ifndef UTIL_H
# define UTIL_H

void configure_i2c(void);

int configure_tmp75a(void);

uint16_t read_tmp75a(void);

#endif