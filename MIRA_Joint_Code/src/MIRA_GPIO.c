/*
 * MIRA_GPIO.c
 *
 *  Created on: Mar 17, 2018
 *      Author: Ben Titus
 */

#include "include/MIRA_GPIO"


/******************* HWIs ******************/


/**************** Clock SWIs ***************/


/****************** Tasks *****************/


/************* Helper Functions ************/


/************* Setup Functions *************/
void Pin_Setup(void) {
    // Set GPIO A0 and A1 as UART pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);


    // Pf2 is Heartbeat LED (Blue)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, HEARTBEAT_PIN);


    // Initialize PB6 as PWM output
    // PB6 is motor PWM
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB6_M0PWM0);
    GPIOPinTypePWM(GPIO_PORTB_BASE, MOTOR_PWM_PIN);


    // Initialize PB0, PB1, PB5 as GPIO output
    // PB5 is motor direction
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1 | MOTOR_DIR_PIN);


    // Initialize SSI
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);          // PA2 -> SCLK
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);          // PA3 -> CS
    GPIOPinConfigure(GPIO_PA4_SSI0RX);           // PA4 -> MISO
    GPIOPinConfigure(GPIO_PA5_SSI0TX);           // PA5 -> MOSI
    GPIOPinTypeSSI(GPIO_PORTA_BASE, SCLK_PIN | CS_PIN | MISO_PIN | MOSI_PIN);


    // Initialize PE4 and PE5 as CAN0Rx and CAN0Tx respectively
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinConfigure(GPIO_PE4_CAN0RX);
    GPIOPinConfigure(GPIO_PE5_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);


    // Initialize PE0 as ADC input
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeADC(GPIO_PORTE_BASE, ISEN_PIN);
}