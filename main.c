//
// Libraries
//
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "utils/ustdlib.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"

#include "platform/board.h"

#include "simplelink.h"
#include "sl_common.h"
#include "sl_config.h"

//
// Define Section
//
#define MSG_BUFFER_SIZE 64

//
// Global Variables
//
extern _u32 g_SysClock;
_i16 sckt_id = -1;
_i16 sckt_status = -1;

//
// Function prototypes
//


//
// Driverlib error routine
//
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{

}
#endif


void onboardleds_init(void)
{
        // Enable GPIO used by UART0
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

        // Check if peripherals are enabled
        while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) ;

        // Enable GPIO F and GPIO N
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

        // Check if peripherals are enabled
        while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPION) && !MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) ;

        // Enable GPIO pins as digital output
        MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);
        MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
}


//
// ADC0 configuration
//
void adc0_init(void)
{
        // Enable GPIO PE3 - AN0 pin
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

        // Enable ADC0
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

        // Wait for the peripherals to be ready
        while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0) && !MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) ;

        // Set analog function to PE3 (AN0)
        MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);

        // ADC0 clock @32MHz (2msps)
        ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, 15); // 480 / 15 = 32

        // Enable Sampler Sequencer 3 - single sample
        // Triggered by Timer 0A
        // Priority 0
        MAP_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);

        // Configure a step of the sample sequencer
        MAP_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_END | ADC_CTL_IE);
        MAP_ADCSequenceEnable(ADC0_BASE, 3);

        // Enables adc interrupt
        // ADCIntEnable(ADC0_BASE, 3);
        // IntEnable(INT_ADC0SS3);
}

//
// Timer0A configuration: triggerring ADC0
//
void timer_init(void)
{
        // Enable Timer0
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

        // Wait for the peripherals to be ready
        while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) ;

        // Configures the Timer0
        MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC | TIMER_CFG_SPLIT_PAIR);

        // Sets system clk as clk source to the timer module
        MAP_TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);

        // Sets the timer prescale value
        MAP_TimerPrescaleSet(TIMER0_BASE, TIMER_A, 100);

        // Sets the timer load value ~@10 Hz
        MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, 1200);

        // Enable triggering
        MAP_TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

        MAP_TimerEnable(TIMER0_BASE, TIMER_A);

        // Enable timer interrupt...
        // MAP_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
        // MAP_IntEnable(INT_TIMER0A);
}


//
// Main function
//
int main(int argc, char** argv)
{
        _i32 retVal = -1;

        retVal = initializeAppVariables();
        ASSERT_ON_ERROR(retVal);

        // Clk and WDT
        stopWDT();
        initClk();

        // Enable processor interrupt
        MAP_IntMasterEnable();

        // Console init
        CLI_Configure();

        // Initialize onboard LEDs
        onboardleds_init();

        // Timer0 init
        timer_init();

        // ADC0 init
        adc0_init();

        // Enable processor interrupt

        CLI_Write((_u8 *)" Adc initialized successfully! \n\r");

        retVal = configureSimpleLinkToDefaultState();
        if(retVal < 0)
        {
                if (DEVICE_NOT_IN_STATION_MODE == retVal)
                        CLI_Write((_u8 *)" Failed to configure the device in its default state \n\r");

                LOOP_FOREVER();
        }

        CLI_Write((_u8 *)" Device is configured in default state \n\r");

        // device is in STA mode
        retVal = sl_Start(0, 0, 0);
        if ((retVal < 0) || (ROLE_STA != retVal))
        {
                CLI_Write((_u8 *)" Failed to start the device \n\r");
                LOOP_FOREVER();
        }

        CLI_Write((_u8 *)" Device started as STATION \n\r");

        // Connect to AP
        retVal = establishConnectionWithAP();
        if(retVal < 0)
        {
                CLI_Write((_u8 *)" Failed to establish connection w/ an AP \n\r");
                LOOP_FOREVER();
        }

        CLI_Write((_u8 *)" Connection established w/ AP and IP is acquired \n\r");

        retVal = checkLanConnection();
        if(retVal < 0)
        {
                CLI_Write((_u8 *)" Device couldn't connect to LAN \n\r");
                LOOP_FOREVER();
        }

        CLI_Write((_u8 *)" Device successfully connected to the LAN\r\n");

        // setTime
        SetTime();

        SlSockAddrIn_t srv_addr;
        srv_addr.sin_family = SL_AF_INET; // IPv4
        srv_addr.sin_port = sl_Htons((_u16) SERVER_PORT);
        srv_addr.sin_addr.s_addr = sl_Htonl((_u32) SERVER_IP);

        // Create the client socket
        sckt_id = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET); // IPv4 / SSL
        // sckt_id = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0); // IPv4 / SSL
        if(sckt_id < 0)
        {
                CLI_Write((_u8 *) " *Could not open the socket*\r\n");
        }

        sckt_status = sl_SetSockOpt(sckt_id, SL_SOL_SOCKET, SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME, "/cert/clikey.der", strlen("/cert/clikey.der"));
        if(sckt_status < 0)
        {
                CLI_Write((_u8 *) " *Could not read the SSL certificate*\r\n");
        }

        sckt_status = sl_Connect(sckt_id, (SlSockAddr_t *) &srv_addr, sizeof(SlSockAddrIn_t));
        if(sckt_status < 0)
        {
                CLI_Write((_u8 *) " *Could not connect to the server*\r\n");
        }

        CLI_Write((_u8 *)" Socket was created successfully\r\n");

        // main loop
        char msg_buf[MSG_BUFFER_SIZE];
        _u32 counter = 0;
        _u32 ADC0ResultBuf[1];
        while(true)
        {
                // gets the current raw interrupt status
                while(!MAP_ADCIntStatus(ADC0_BASE, 3, false)) ;

                // Enables sample sequencer 3 and interrupt
                MAP_ADCIntClear(ADC0_BASE, 3);

                // Read the ADC0 value0
                MAP_ADCSequenceDataGet(ADC0_BASE, 3, ADC0ResultBuf);

                // Format message
                usprintf(msg_buf, "#%u: %u\r\n", counter, ADC0ResultBuf[0]);

                // Sending the value of the AN0 pin through the SSL socket
                if(sckt_id > 0)
                {
                        sckt_status = sl_Send(sckt_id, msg_buf, MSG_BUFFER_SIZE, 0);
                        counter++;
                        // CLI_Write((_u8 *) "Sending message...\r\n");
                }
        }
        // Close socket
        sl_Close(sckt_id);
        CLI_Write((_u8*) " Socket closed! \r\n");
        return 0;
}

//
// Timer0A Interrupt Handler
//
//
// _u16 led_status = 0;
// _u16 tmr_count = 0;
// void Timer0AIntHandler(void)
// {
//         TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
//         tmr_count++;
//         if(tmr_count == 100)
//         {
//                 // Toggle LEDs
//                 led_status = (led_status == 0) ? 0xFFFF : 0x0000;
//                 GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, led_status & (GPIO_PIN_0 | GPIO_PIN_1));
//                 GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, led_status & (GPIO_PIN_0 | GPIO_PIN_4));
//                 tmr_count = 0;
//         }
// }


// void ADC0SS3IntHandler(void)
// {
//         // gets the current raw interrupt status
//         while(!MAP_ADCIntStatus(ADC0_BASE, 3, false)) ;
//
//         // Enables sample sequencer 3 and interrupt
//         MAP_ADCIntClear(ADC0_BASE, 3);
//
//         // Read the ADC0 value0
//         MAP_ADCSequenceDataGet(ADC0_BASE, 3, ADC0ResultBuf);
//
//         // Format message
//         usprintf(msg_buf, "#%u: %u\r\n", counter, ADC0ResultBuf[0]);
//
//         // Sending the value of the AN0 pin through the SSL socket
//         if(sckt_id > 0)
//         {
//                 sckt_status = sl_Send(sckt_id, msg_buf, MSG_BUFFER_SIZE, 0);
//                 counter++;
//                 // CLI_Write((_u8 *) "Sending message...\r\n");
//         }
//
// }
