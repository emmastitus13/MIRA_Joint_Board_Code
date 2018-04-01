/*
 * MIRA_UART.h
 *
 *  Created on: Mar 17, 2018
 *   Edited on: Mar 27, 2018
 *      Author: Ben Titus
 */

#ifndef INCLUDE_MIRA_UART_H_
#define INCLUDE_MIRA_UART_H_


/********** General C Header files *********/
#include <stdbool.h>
#include <stdint.h>


/********** XDCtools Header files **********/
#include <xdc/std.h>
#include <xdc/cfg/global.h>


/************ BIOS Header files ************/
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>


/*********** TI RTOS Header files **********/
#include <inc/hw_gpio.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <ti/drivers/GPIO.h>
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"


/************ Local Header files ***********/
#include "include/MIRA_CAN.h"
#include "include/MIRA_GPIO.h"


/***************** Defines *****************/


/*********** Function Prototypes ***********/
// HWIs
void UART_ISR(void);

// SWIs
void UART_Timer(void);

// Tasks
void UART_Init(void);
void UART_Transmit(void);
void UART_Update_Position(void);

// Helper Functions
void UART_Print(char* str, uint8_t len);
void UART_Print_Num(int num);

// Setup Functions
void UART_Setup(void);


/************* Global Variables ************/
static uint32_t UART_State = 0;

uint8_t Init_Data[20];
volatile uint8_t Global_Init_Data[20];
uint32_t Init_Data_Index;

uint8_t Update_Data[4];
volatile uint8_t Global_Update_Data[4];
uint32_t Update_Data_Index;

uint32_t Init_Joint_Number; // Joint index for initialization
uint32_t Joint_Index;       // Joint index for updates


// Strings for printing
static char* g_pcStartupMsg = "Initialized!\n\r";
static uint8_t g_ui8StartupMsgLen = 15;
static char* g_pcReceivedMsg = "Message received!\n\r";
static uint8_t g_ui8ReceivedMsgLen = 20;
static char* g_pcReceivedMsgError = "Error receiving message\n\r";
static uint8_t g_ui8ReceivedMsgErrorLen = 26;



#endif /* INCLUDE_MIRA_UART_H_ */
