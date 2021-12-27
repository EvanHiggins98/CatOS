/******************************************************************************
*******************************************************************************
  ECE 270 Fall 2020

  main.c
	
  Homework #:7

  Copyright DigiPen (USA) Corporation
        All Rights Reserved
	
*******************************************************************************
******************************************************************************/

/******************************************************************************
*******************************************************************************
  References:

  [1] RM0090 Reference manual 
  STM32F405/415, STM32F407/417, STM32F427/437 and STM32F429/439 
    advanced ARM®-based 32-bit MCUs

  [2] UM1670 User manual
  Discovery kit with STM32F429ZI MCU

  [3] STM32F427xx/STM32F429xx Datasheet
  ARM Cortex-M4 32b MCU+FPU, 225DMIPS, up to 2MB Flash/256+4KB RAM, USB
    OTG HS/FS, Ethernet, 17 TIMs, 3 ADCs, 20 comm. interfaces, camera & LCD-TFT

  [4] PM0214 Programming manual
  STM32F3, STM32F4 and STM32L4 Series
    Cortex®-M4 programming manual

*******************************************************************************
******************************************************************************/

/******************************************************************************
*******************************************************************************
	Includes
*******************************************************************************
******************************************************************************/
#include "Kernel.h"

/******************************************************************************
*******************************************************************************
	Definitions
*******************************************************************************
******************************************************************************/
#define   EVENT_RED           0x01
#define   EVENT_BLUE          0x02
#define   EVENT_GREEN         0x04
#define   NO_ANDs             0x00
#define   NO_ORs              0x00
#define   MAX_POINTERS        0x80

#define   ONE_SECOND          0x2000//0x00081B32

#define   USER_TASKS          0x03
#define   USER_STACK          0x20

#define   PR_LOW              0x01
#define   PR_HIGH             0x02
#define   DL_3                0x03
#define   DL_6                0x06
#define   EX_1                0x01

/******************************************************************************
*******************************************************************************
	Prototypes
*******************************************************************************
******************************************************************************/

void
TaskBlue (void);

void
TaskRed (void);

void
TaskGreen (void);

unsigned int
LFSR (unsigned int);

/******************************************************************************
*******************************************************************************
	Globals
*******************************************************************************
******************************************************************************/
unsigned int whoTurn = EVENT_BLUE;
unsigned int semTurn;

/******************************************************************************
*******************************************************************************
	Main.  Note that `startup_TM4C123.s` is looking for __main
*******************************************************************************
******************************************************************************/
int
main (void) {
    // WARNING: I will compile your code with different seed values!!!!!!
    LFSR(0x1234);
	
    if (OS_InitKernel(USER_TASKS, USER_STACK)) {
        if (OS_CreateTask(&TaskBlue, PR_HIGH, EX_1, DL_3)) {
            if (OS_CreateTask(&TaskRed, PR_HIGH, EX_1, DL_3)) {
                if (OS_CreateTask(&TaskGreen, PR_LOW, EX_1, DL_6)) {
                    if (semTurn = OS_SemCreate(MUTEX, 0x01, 0x01)) {
                        OS_Start();
                    } // end if
                } // end if
            } // end if
        } // end if 
    } // end if

    while (0x01) {
        ;
    } // end while
	
} // end main

/******************************************************************************
*******************************************************************************
	Helper Functions
*******************************************************************************
******************************************************************************/

void
TaskBlue (void) {
    unsigned int i;

    while (0x01) {
        while (whoTurn != EVENT_BLUE) { ; }

        for (i = 0x00; i < ONE_SECOND; ++i) {
            OS_Free(OS_Malloc(LFSR(0x00)));
        } // end for

        OS_SemAcquire(semTurn);
        whoTurn = EVENT_RED;
        OS_ClearLEDs(RED);	
        OS_SemRelease(semTurn);

    } // end while

} // end TaskBlue

void
TaskRed (void) {
    unsigned int i;

    while (0x01) {
        while (whoTurn != EVENT_RED) { ; }

        for (i = 0x00; i < ONE_SECOND; ++i) {
            OS_Free(OS_Malloc(LFSR(0x00)));
        } // end for

        OS_SemAcquire(semTurn);
        whoTurn = EVENT_GREEN;f
        OS_SetLEDs(RED);	
        OS_SemRelease(semTurn);

    } // end while	

} // end TaskRed

void
TaskGreen (void) {
    static void *memPtr[MAX_POINTERS];
    unsigned int i;
	
    while (0x01) {
        while (whoTurn != EVENT_GREEN) { ; }

        if (OS_GetButton()) {
            OS_ClearLEDs(GREEN);
        } // end if
        else {
            OS_SetLEDs(GREEN);
        } // end else
        for (i = 0x00; i < MAX_POINTERS; ++i) {
            memPtr[i] = OS_Malloc(sizeof(char) * i);
            if (!memPtr[i]) {
                break;
            } // end if
        } // end for
        for (i = 0x00; i < MAX_POINTERS; ++i) {
            if (memPtr[i]) {
                OS_Free(memPtr[i]);
            } // end if
        } // end for		

        OS_SemAcquire(semTurn);
        whoTurn = EVENT_BLUE;
        OS_SemRelease(semTurn);
    } // end while

} // end TaskGreen

/*
        Implementation of Fibboncacci linear feedback shift register for
    pseudorandom number generation.
*/
unsigned int
    LFSR (unsigned int seed) {
    static unsigned int LFSR = 0x01;
    unsigned int newBit;
	
    if (seed & 0xFFFF) {
        LFSR = seed & 0xFFFF;
    } // end if
    else {
        if (LFSR) {
            // = feedback polynomial x^16 + x^14 + x^13 + x^11 + 0x01
            newBit = (	(LFSR >> 0x00) ^ 
                        (LFSR >> 0x02) ^ 
                        (LFSR >> 0x03) ^ 
                        (LFSR >> 0x05)) & 0x01;
            LFSR = (LFSR >> 0x01) | (newBit << 0x0F);
        } // end if
        else {
            LFSR = 0x01;
        } // end if
    } // end else	
    return LFSR % 0x100;

} // end LFSR

// EOF    main.c
// Note: Some IDEs generate warnings if a file doesn't end in whitespace,
//  but Embedded Studio doesn't seem to be one of them.


