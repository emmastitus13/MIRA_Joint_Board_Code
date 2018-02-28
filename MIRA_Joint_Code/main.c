/*
 *
 */


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
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
// #include <ti/drivers/I2C.h>
// #include <ti/drivers/SDSPI.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>
// #include <ti/drivers/WiFi.h>


/************ Local Header files ***********/
#include "include/Board.h"


/***************** Defines *****************/
// Pins
#define SCLK_PIN GPIO_PIN_2      // Pin A2
#define CS_PIN GPIO_PIN_3        // Pin A3
#define MISO_PIN GPIO_PIN_4      // Pin A4
#define MOSI_PIN GPIO_PIN_5      // Pin A5
#define MOTOR_DIR_PIN GPIO_PIN_5 // Pin B5
#define MOTOR_PWM_PIN GPIO_PIN_6 // Pin B6
#define CAN0RX_PIN GPIO_PIN_4    // Pin E4
#define CAN0TX_PIN GPIO_PIN_5    // Pin E5
#define HEARTBEAT_PIN GPIO_PIN_2 // Pin F2

#define TASKSTACKSIZE   512


/*********** Function Prototypes ***********/
void Heartbeat(void);
void Motor_Timer(void);
void Motor_Drive(void);
void Motor_Out(int spd);
void Pin_Setup(void);
void Motor_Setup(void);


/************* Global Variables ************/
bool heartbeat = false;
int g_iMotorCtrl;


/**************** Clock SWIs ***************/
void Heartbeat(void) {
    if (heartbeat) {
        GPIOPinWrite(GPIO_PORTF_BASE, HEARTBEAT_PIN, HEARTBEAT_PIN);
        heartbeat = false;
    } else {
        GPIOPinWrite(GPIO_PORTF_BASE, HEARTBEAT_PIN, 0);
        heartbeat = true;
    }
}

void Motor_Timer(void) {
    Semaphore_post(Motor_Semaphore);
}


/****************** Tasks *****************/
void Motor_Drive(void) {
    while(1) {
        Semaphore_pend(Motor_Semaphore, BIOS_WAIT_FOREVER);
        Motor_Out(g_iMotorCtrl);
    }
}


/************* Helper Functions ************/
void Motor_Out(int spd) {
    // Constrain spd to -5120 to +5120
    if (spd > 5120)
        spd = 5120;
    if (spd < -5120)
        spd = -5120;

    // Set direction pin and PWM compare value
    if (spd >= 0) {
        // Set direction pin
        GPIOPinWrite(GPIO_PORTB_BASE, MOTOR_DIR_PIN, MOTOR_DIR_PIN);
        // Set PWM value to spd maximum
        PWMPulseWidthSet(PWM0_BASE, PWM_GEN_0, spd);
    } else {
        // Set direction pin
        GPIOPinWrite(GPIO_PORTB_BASE, MOTOR_DIR_PIN, 0);
        // Set PWM value to spd maximum
        PWMPulseWidthSet(PWM0_BASE, PWM_GEN_0, (-spd));
    }
}


/************* Setup Functions *************/
void Pin_Setup(void) {
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
    GPIOPinTypeCAN(GPIO_PORTE_BASE, CAN0RX_PIN | CAN0TX_PIN);

}
void Motor_Setup(void) {
    // Enable the peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    // Set clock to system clock with 32x input divider
    SysCtlPWMClockSet(SYSCTL_PWMDIV_32);
//    PWMClockSet(PWM_BASE, PWM_SYSCLK_DIV_64);

    //
    PWMDeadBandDisable(PWM0_BASE, PWM_GEN_0);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, 5120);

    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_0, 0);

    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);

    PWMOutputUpdateMode(PWM0_BASE, PWM_OUT_0_BIT, PWM_OUTPUT_MODE_NO_SYNC);

    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}

// Main, call all setup functions and start BIOS
int main(void) {
    // Disable interrupts
    IntMasterDisable();

    /* Call board init functions */
    //Board_initGeneral();
    //Board_initGPIO();
    // Board_initI2C();
    // Board_initSDSPI();
    // Board_initSPI();
    // Board_initUART();
    // Board_initUSB(Board_USBDEVICE);
    // Board_initWatchdog();
    // Board_initWiFi();

    // Call all setup functions
    Pin_Setup();
    Motor_Setup();

    // Enable interrupts
    IntMasterEnable();

    // Start the BIOS
    BIOS_start();

    return (0);
}
