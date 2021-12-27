
/******************************************************************************
*******************************************************************************
ECE 270 –Fall2021
Evan Higgins
Kernel HAL_asm.s
create a TCB.
This file is the assembly to start the OS running the idle task
Homework #: 4
Copyright DigiPen (USA) Corporation
All Rights Reserved
*******************************************************************************
******************************************************************************/
/******************************************************************************
*******************************************************************************
  References:

  [5] Real-Time Operating Systems for
  ARM Cortex-M Microcontrollers

*******************************************************************************
******************************************************************************/
#include "Kernel HAL_asm.h"
  .syntax unified                   // Required to execute Thumb-2 code

  .global OS_Start                  // Required to expose function to .c files
  .global TIM6_DAC_IRQHandler
  .extern OS_TaskRUNNING            // Defined in Kernel.c
  .extern OS_TaskNEW

  .section .text                    // Where to locate this code
  .align 0

  .thumb_func
OS_Start:
    LDR R0, =OS_TaskRUNNING
    LDR R1, [R0]
    LDR SP, [R1]
    MOV R3, #OS_RUNNING
    STR R3, [R1,TCB_STATE_OFFSET]
    POP {R4-R11}
    POP {R0-R3}
    POP {R12}
    ADD SP, SP, #4
    POP {LR}
    ADD SP, SP, #4
    CPSIE I
    BX LR

    .weak TIM6_DAC_IRQHandler
    .thumb_func
TIM6_DAC_IRQHandler:
  CPSID I
  PUSH {R4-R11}
  ldr R0, =TIMER6_SR
  WAIT_FLAG_LOOP:
    ldr r1, [r0]
    bic r1, r1, #1
    str r1, [r0]
    ldr r1, [r0]
    lsls r1, r1, #31
    bne WAIT_FLAG_LOOP
  PUSH {LR}
  bl OSp_ScheduleTask
  POP {LR}
  CMP R0, #1
  BNE SameTask
  LDR R0, =OS_TaskRUNNING
  LDR R1, [R0]
  STR SP,[R1]
  LDR R1, =OS_TaskNEW
  LDR R1, [R1]
  STR R1, [R0]
  LDR SP, [R1]
SameTask:
  POP {R4-R11}
  CPSIE I
  BX LR
 
// EOF    Kernel HAL_asm.s
// Note: Some IDEs generate warnings if a file doesn't end in whitespace,
//  but Embedded Studio doesn't seem to be one of them.



