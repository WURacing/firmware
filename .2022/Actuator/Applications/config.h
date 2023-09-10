/* 
 * File:   config.h
 * Author: Connor
 *
 * Created on March 9, 2021, 12:11 PM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#define EL_04002_22			40020
#define EL_04002_22_REV_A	40021
#ifndef BOARD_ID
#warning BOARD_ID defaulting to EL_04002_22
#define BOARD_ID EL_04002_22
#endif

#define CTRL		6
#ifndef BOARD_USAGE
#warning BOARD_USAGE defaulting to CTRL
#define BOARD_USAGE CTRL
#endif



#endif	/* CONFIG_H */

