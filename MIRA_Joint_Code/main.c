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
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
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
#define SCLK_PIN       GPIO_PIN_2 // Pin A2
#define CS_PIN         GPIO_PIN_3 // Pin A3
#define MISO_PIN       GPIO_PIN_4 // Pin A4
#define MOSI_PIN       GPIO_PIN_5 // Pin A5
#define MOTOR_DIR_PIN  GPIO_PIN_5 // Pin B5
#define MOTOR_PWM_PIN  GPIO_PIN_6 // Pin B6
#define ISEN_PIN       GPIO_PIN_0 // Pin E0
#define CAN0RX_PIN     GPIO_PIN_4 // Pin E4
#define CAN0TX_PIN     GPIO_PIN_5 // Pin E5
#define HEARTBEAT_PIN  GPIO_PIN_2 // Pin F2

#define MOTOR_MAX_SPD  4096       // Max speed of motor (PWM timer reset value)
#define READ_ANGLE     0x3FFF     // Data to send over SSI to read angle from AS5055

#define TASKSTACKSIZE   512


/*********** Function Prototypes ***********/
// HWIs
void ADC_ISR(void);
void UART_ISR(void);

// SWIs
void Heartbeat(void);
void Motor_Timer(void);
void Motor_Drive(void);
void SSI_Timer(void);

// Helper Functions
void Motor_Out(int spd);
void UART_Print(char* str, uint8_t len);
void UART_Print_Num(int num);
void SSI_Send(uint16_t num);

// Setup Functions
void Pin_Setup(void);
void Motor_Setup(void);
void ADC_Setup(void);
void UART_Setup(void);
void SSI_Setup(void);


/************* Global Variables ************/
bool heartbeat = false;
int g_iMotorCtrl;
volatile int g_iInByte;


// Strings for printing
char* g_pcStartupMsg = "Initialized!\n\r";
uint8_t g_ui8StartupMsgLen = 15;
char* g_pcReceivedMsg = "Message received!\n\r";
uint8_t g_ui8ReceivedMsgLen = 20;
char* g_pcReceivedMsgError = "Error receiving message\n\r";
uint8_t g_ui8ReceivedMsgErrorLen = 26;


/******************* HWIs ******************/
void ADC_ISR(void) {

}

void UART_ISR(void) {
    int i = 4;
    g_iInByte = 0;
    while (UARTCharsAvail(UART0_BASE)) {
        switch (i) {
        case 4:
            g_iInByte += (UARTCharGetNonBlocking(UART0_BASE) - 0x30) * 1000;
            break;

        case 3:
            g_iInByte += (UARTCharGetNonBlocking(UART0_BASE) - 0x30) * 100;
            break;

        case 2:
            g_iInByte += (UARTCharGetNonBlocking(UART0_BASE) - 0x30) * 10;
            break;

        case 1:
            g_iInByte += (UARTCharGetNonBlocking(UART0_BASE) - 0x30);
            break;

        default:
            UART_Print(g_pcReceivedMsgError, g_ui8ReceivedMsgErrorLen);
            break;
        }
        i--;
    }
    g_iInByte -= 4096;
    UART_Print_Num(g_iInByte);
    UART_Print(g_pcReceivedMsg, g_ui8ReceivedMsgLen);
    UARTIntClear(UART0_BASE, UART_INT_RX);
}

void SSI_ISR(void) {

}


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

void ADC_Timer(void) {
    Semaphore_post(ADC_Semaphore);
}

void SSI_Timer(void) {
    Semaphore_post(SSI_Semaphore);
}


/****************** Tasks *****************/
void Motor_Drive(void) {
    while(1) {
        Semaphore_pend(Motor_Semaphore, BIOS_WAIT_FOREVER);
        g_iMotorCtrl = g_iInByte;
        Motor_Out(g_iMotorCtrl);
        g_iMotorCtrl++;
        if (g_iMotorCtrl > 5120)
            g_iMotorCtrl = -5120;
    }
}

void ADC_Read(void) {
    while(1) {
        Semaphore_pend(ADC_Semaphore, BIOS_WAIT_FOREVER);
    }
}

void Init_Stuff(void) {
    while(1) {
        Semaphore_pend(Init_Semaphore, BIOS_WAIT_FOREVER);
    }
}

void SSI_Transmit(void) {
    while(1) {
        Semaphore_pend(SSI_Semaphore, BIOS_WAIT_FOREVER);
    }
}


/************* Helper Functions ************/
void Motor_Out(int spd) {
    // Constrain spd to -5120 to +5120
    if (spd > MOTOR_MAX_SPD)
        spd = MOTOR_MAX_SPD;
    if (spd < -MOTOR_MAX_SPD)
        spd = -MOTOR_MAX_SPD;
    if (spd == 0)
        spd = 1;

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

void UART_Print(char* str, uint8_t len) {
    int i;
    for (i = 0; i < len; i++) {
        UARTCharPut(UART0_BASE,str[i]);
    }
}

void UART_Print_Num(int num) {
    char numArray[5];
    int i;
    if (num < 0) {
        numArray[0] = '-';
        num = -num;
    } else {
        numArray[0] = ' ';
    }
    for (i = 5; i > 1; i--) {
        numArray[i-1] = (num % 10) + 0x30;
        num /= 10;
    }
    UART_Print(numArray, 5);
}

void SSI_Send(uint16_t num) {
    SSIDataPut(SSI0_BASE, num);
}


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
    GPIOPinTypeCAN(GPIO_PORTE_BASE, CAN0RX_PIN | CAN0TX_PIN);

    // Initialize PE0 as ADC input
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeADC(GPIO_PORTE_BASE, ISEN_PIN);
}

void Motor_Setup(void) {
    // Enable the peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    // Set clock to system clock with 32x input divider
    SysCtlPWMClockSet(SYSCTL_PWMDIV_16);
//    PWMClockSet(PWM_BASE, PWM_SYSCLK_DIV_64);

    //
    PWMDeadBandDisable(PWM0_BASE, PWM_GEN_0);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, MOTOR_MAX_SPD);

    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_0, 1);

    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);

    PWMOutputUpdateMode(PWM0_BASE, PWM_OUT_0_BIT, PWM_OUTPUT_MODE_NO_SYNC);

    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}

void ADC_Setup(void) {
//    // Enable Timer1
//    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
//    TimerDisable(TIMER1_BASE, TIMER_BOTH);
//    // Set clock source
//    TimerClockSourceSet(TIMER1_BASE, TIMER_CLOCK_SYSTEM);
//    // Set to periodic
//    TimerConfigure(TIMER1_BASE, TIMER_CFG_A_PERIODIC);
//    // Set the prescaler to 1
//    TimerPrescaleSet(TIMER1_BASE, TIMER_A, 0);
//    // Set load value
//    TimerLoadSet(TIMER1_BASE, TIMER_A, 0x0FFFF);
//    // Set compare value
////    TimerMatchSet(TIMER1_BASE, TIMER_A, 0x07FFF);
//    // Enable timeout interrupt
//    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
//    // Register ISR
//    TimerIntRegister(TIMER1_BASE, TIMER_A, &timerISR);
//    // Enable Timer1A
//    TimerEnable(TIMER1_BASE, TIMER_A);

    // Initialize ADC0 Module
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

//    // Wait for the module to be ready
//    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {
//    }

    ADCSequenceDisable(ADC0_BASE, 0);
//    // ADC_PC_SR_125k, ADC_PC_SR_250k, ADC_PC_SR_500k, ADC_PC_SR_1M
//    ADC0_BASE + ADC_O_PC = (ADC_PC_SR_1M);

    // ADC clock
    // Source - internal PIOSC at 16MHz, clock rate full for now
    // Divider - 1 for now, could change later
    // Maybe use PLL if the frequency isn't high enough
//    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_FULL, 2);

    // Trigger when the processor tells it to (one shot)
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_ALWAYS, 0);

    // Take a sample and interrupt
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH3 | ADC_CTL_IE | ADC_CTL_END);

    // Oversample at 16x (could go higher maybe? lower?)
    ADCHardwareOversampleConfigure(ADC0_BASE, 16);

    // Enable sequence0
    ADCSequenceEnable(ADC0_BASE, 0);

    // Enable interrupts for sequence 0
    ADCIntEnable(ADC0_BASE, 0);
//    ADCIntRegister(ADC0_BASE, 0, &getADC);
}

void UART_Setup(void) {
    // Enable peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Set UART clock
//    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    // Configure UART for 115200 baud, 8n1 operation
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                                (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                 UART_CONFIG_PAR_NONE));

    // Enable transmit and receive FIFOs
    UARTFIFOEnable(UART0_BASE);

    // Set FIFO receive level to 2
    UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX7_8, UART_FIFO_RX2_8);

    // Enable UART receive interrupts
    UARTIntEnable(UART0_BASE, UART_INT_RX);

    // Enable UART0
//    UARTEnable(UART0_BASE);

    // Put character in output buffer
    UART_Print(g_pcStartupMsg, g_ui8StartupMsgLen);
}

void SSI_Setup(void) {
    // Enable peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    // Set clock to system clock
    SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);

    /*  Configure SSI:
     *  Clock speed -> System clock
     *  Mode -> polarity 0, phase 0
     *
     */
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 5000000, 16);
    SSIEnable(SSI0_BASE);
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
    UART_Setup();

    // Enable interrupts
    IntMasterEnable();

    // Start the BIOS
    BIOS_start();

    return (0);
}
