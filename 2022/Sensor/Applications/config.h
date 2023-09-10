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

#define SBFront1	1
#define SBFront2	2
#define SBRear1		3
#define SBRear2		4
#define SBCG		5
#ifndef BOARD_USAGE
#warning BOARD_USAGE defaulting to SBFront1
#define BOARD_USAGE SBFront1
#endif



#endif	/* CONFIG_H */

