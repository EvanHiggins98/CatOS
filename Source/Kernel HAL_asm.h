
/******************************************************************************
*******************************************************************************
ECE 270 –Fall2021
Evan Higgins
Kernel HAL_asm.h
create a TCB.
This file is the header for the assembly based scheduler functions
Homework #: 4
Copyright DigiPen (USA) Corporation
All Rights Reserved
*******************************************************************************
******************************************************************************/
/******************************************************************************
      Contains gcc symbol definitions that must be shared between both the
    assembly and C source files.
******************************************************************************/

/******************************************************************************
*******************************************************************************
    Definitions
*******************************************************************************
******************************************************************************/

#define   OS_READY            0x00
#define   OS_RUNNING          0x01
#define   OS_DONE             0x02
#define   OS_BLOCKED          0x03

//   /
// \/ pt:
//  Left to the student to determine what these values should be
#define   BYTES_PER_WORD    4
#define   TCB_STATE_OFFSET  (BYTES_PER_WORD*2)  
#define   TIMER6_SR         0x40001010
#define   TIMER_IF          1  

// EOF    Kernel HAL_asm.h
// Note: Some IDEs generate warnings if a file doesn't end in whitespace,
//  but Embedded Studio doesn't seem to be one of them.



