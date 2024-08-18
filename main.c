/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include <stdio.h>  // For sprintf function

#define SAMPLE_SIZE 50
#define PWM_MAX 255

uint16 sens = 0;
uint16 x = 0;
uint16 total = 0;
uint16 basis = 0;
uint32 startTime;
uint32 endTime;
uint8 executeFlag = 1;

// Function to send a number over UART
void UART_PutNumber(uint32 number) {
    char buffer[12];  // Buffer to hold the string representation of the number
    sprintf(buffer, "%lu", number);  // Convert number to string
    UART_PutString(buffer);  // Send the string over UART
}

// Function for calibration phase
void Calibration(void) {
    uint16 i;
    for (i = 0; i < SAMPLE_SIZE; i++) {
        ADC_StartConvert();                             // Start ADC conversion
        ADC_IsEndConversion(ADC_RETURN_STATUS);         // Wait for conversion to finish
        sens = ADC_GetResult16();                       // Get ADC result
        total += sens;
        CyDelay(10);                                    // Small delay between samples
    }
    sens = total / SAMPLE_SIZE;  // Calculate the average of the samples
    total = 0;
    CyDelay(5000);               // Delay to allow the system to stabilize
    basis = sens;                // Set the sensitivity threshold
}

int main(void) {
    CyGlobalIntEnable;  // Enable global interrupts

    PWM_Start();    // Start PWM component
    ADC_Start();    // Start ADC component
    Timer_Start();  // Start Timer component

    detectingLED_Write(0); // Initialise detectingLED to low
    FlagPin_Write(0);      // Initialise FlagPin to low

    UART_Start();   // Start UART for debugging

    Calibration();  // Run calibration

    UART_PutString("PWM, Measured Intensity(");
    UART_PutNumber(basis);
    UART_PutString("), Response Time\r\n");

    while (1) {
        uint16 ii = 0;

        while (executeFlag) {
            for (ii = 0; ii < PWM_MAX; ii++) {
                PWM_WriteCompare1(ii);  // Set PWM duty cycle for emittingLED

                total = 0;
                for (x = 0; x < SAMPLE_SIZE; x++) {
                    ADC_StartConvert();
                    ADC_IsEndConversion(ADC_RETURN_STATUS);
                    sens = ADC_GetResult16();
                    total += sens;
                    CyDelay(10);
                }
                sens = total / SAMPLE_SIZE;  // Average sensor value

                StartTimePin_Write(1);        // Set the start time pin high 
                startTime = Timer_ReadCounter(); // Record start time
                FlagPin_Write(1);            // Set flag pin high

                // Wait until detectingLED reads 0
                while (detectingLED_Read() != 0);

                FlagPin_Write(0);              // Set flag pin low
                endTime = Timer_ReadCounter(); // Record end time

                uint32 responseTime = endTime - startTime; // Calculate response time
                UART_PutString("Current Intensity: ");
                UART_PutNumber(ii);  // Print the current intensity
                UART_PutString(", ");
                UART_PutString("Sensing Intensity: ");
                UART_PutNumber(sens);
                UART_PutString(", ");
                UART_PutString("Response Time: ");
                UART_PutNumber(responseTime);
                UART_PutString("\r\n");

                CyDelay(10);  // Small delay
            }
            executeFlag = 0;  // Stop execution after one run
        }
    }
}
