/******************************************************************************
*******************************************************************************
ECE 270 –Fall2021
Evan Higgins
Kernel HAL.c
create a TCB.
This file is the kernal HAL implementation
Homework #: 4
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

/*
        This code provides the Hardware Abstraction Layer (HAL) for the
    kernel.  This HAL only supports the STM32F429ZI microcontroller. 
*/

/******************************************************************************
*******************************************************************************
    Includes
*******************************************************************************
******************************************************************************/
#include 	"Kernel HAL.h"
#include 	"stm32f429xx.h"

/******************************************************************************
*******************************************************************************
    Definitions
*******************************************************************************
******************************************************************************/

// Optional definitions not required to produce working code
#ifndef			MAX_WAIT
	#define		BON(X)			|=(X)
	#define		BOFF(X)			&=~(X)
	#define		BTOG(X)			^=(X)
	#define		MAX_WAIT		0xFFFF
#endif		//	MAX_WAIT

//Bit Operations

#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define CLEAR_REG(REG)        ((REG) = (0x0))

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define READ_REG(REG)         ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

#define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))

/******************************************************************************
*******************************************************************************
    Prototypes
*******************************************************************************
******************************************************************************/

/******************************************************************************
*******************************************************************************
    Declarations & Types
*******************************************************************************
******************************************************************************/

/******************************************************************************
*******************************************************************************
    Helper Functions
*******************************************************************************
******************************************************************************/

/******************************************************************************
    OSp_InitGPIOG
		
      The clock to the PORTG module is enabled and the two pins 
    attached to the Red/Green LEDs have their digital outputs enabled. 
******************************************************************************/
unsigned int
OSp_InitGPIOG (void) {
    volatile unsigned int wait = MAX_WAIT;
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOGEN);

    for (wait = 0x00; wait < MAX_WAIT; ) {
      ++wait;
    } 

    SET_BIT(GPIOG->MODER, GPIO_MODER_MODE2_0|GPIO_MODER_MODE3_0|GPIO_MODER_MODE9_0);

    SET_BIT(GPIOG->OTYPER, GPIO_OTYPER_OT2|GPIO_OTYPER_OT3|GPIO_OTYPER_OT9);

    SET_BIT(GPIOG->OSPEEDR, GPIO_OSPEEDR_OSPEED2_1|GPIO_OSPEEDR_OSPEED3_1|GPIO_OSPEEDR_OSPEED9_1);

    CLEAR_BIT(GPIOG->PUPDR, GPIO_PUPDR_PUPD2);

    OS_SetLEDs(RED | GREEN | BLUE);

    return 1;
} // end OSp_InitGPIOG

/******************************************************************************
    OSp_InitGPIOA
		
      The clock to the PORTA module is enabled and the pin
    attached to the Blue pushbutton has its digital input enabled. 
******************************************************************************/
unsigned int
OSp_InitGPIOA (void) {
    volatile unsigned int wait = MAX_WAIT;	

    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);

    for (wait = 0x00; wait < MAX_WAIT; ) {
      ++wait;
    } 

    CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MODE0);

    SET_BIT(GPIOA->OSPEEDR, GPIO_OSPEEDR_OSPEED0_1);

    CLEAR_BIT(GPIOA->PUPDR, GPIO_PUPDR_PUPD0);

    return 1;
} // end OSp_InitGPIOA

/******************************************************************************
    OSp_InitTIM6
		
      The clock to the TIM6 module is enabled and configured to run at a 1ms
    cycle, causing an interrupt if global interrupts are enabled. 
******************************************************************************/
unsigned int
OSp_InitTIM6 (void) {
    volatile unsigned int wait = MAX_WAIT;	
    SET_BIT(RCC->APB1ENR,RCC_APB1ENR_TIM6EN);

    for (wait = 0x00; wait < MAX_WAIT; ) {
      ++wait;
    } 

    SET_BIT(TIM6->CR1,TIM_CR1_ARPE);

    CLEAR_BIT(TIM6->CR1, TIM_CR1_OPM);

    SET_BIT(TIM6->CR1, TIM_CR1_URS);

    CLEAR_BIT(TIM6->CR1, TIM_CR1_UDIS);

    SET_BIT(TIM6->DIER, TIM_DIER_UIE);

    CLEAR_BIT(TIM6->PSC, TIM_PSC_PSC);
    
    WRITE_REG(TIM6->ARR,16000);

    NVIC_SetPriority(TIM6_DAC_IRQn,0);
    
    SET_BIT(TIM6->CR1, TIM_CR1_CEN);

    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    return 1;
    
} // end OSp_InitTIM6

/******************************************************************************
    OS_SetLEDs
		
      Parameter is the bitwise OR of all the colors that should be set
    to ON.  This does not turn any LEDs OFF (requires OS_ClearLEDs).
******************************************************************************/
unsigned int
OS_SetLEDs (unsigned int LEDs) {
    return CLEAR_BIT(GPIOG->ODR, LEDs);
} // end OS_SetLEDs

/******************************************************************************
    OS_ClearLEDs
		
      Parameter is the bitwise OR of all the colors that should be set
    to OFF.  This does not turn any LEDs ON (requires OS_SetLEDs).
******************************************************************************/
unsigned int
OS_ClearLEDs (unsigned int LEDs) {

    return SET_BIT(GPIOG->ODR, LEDs);

} // end OS_ClearLEDs

/******************************************************************************
    OS_GetButton
		
      Returns nonzero if the button is pushed, otherwise returns zero. 
******************************************************************************/
unsigned int
OS_GetButton (void) {

    return READ_BIT(GPIOA->IDR, GPIO_IDR_ID0);

} // end OS_GetButton

/******************************************************************************
    OS_InitKernelHAL
		
      Prepares the system hardware for use.
******************************************************************************/
unsigned int
OS_InitKernelHAL (void) {

    return OSp_InitGPIOG() && OSp_InitGPIOA() && OSp_InitTIM6();

} // end OS_InitKernelHAL

void
OS_Critical_Begin(void){
  __disable_irq();
}

void
OS_Critical_End(void){
  __enable_irq();
}

// EOF    Kernel HAL.c
// Note: Some IDEs generate warnings if a file doesn't end in whitespace,
//  but Embedded Studio doesn't seem to be one of them.

