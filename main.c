/******************************************************************************
* File Name: main.c
*This is the source code for the PMG1 12-bit SAR ADC with OpAmp as buffer Example
*             for ModusToolbox.
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2022-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/
    

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cybsp.h"
#include "cy_pdl.h"
#include "stdio.h"
#include <inttypes.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define CY_ASSERT_FAILED            (0u)
#define UART_PRINT_DELAY            (500u)    /* milliseconds */
#define CHANNEL_0                   (0u)
#define ADC_REFERENCE               (3.3)
#define ADC_MAX_RESULT              (4095)

/* Debug print macro to enable UART print */
#define DEBUG_PRINT                 (1u)

/*******************************************************************************
* Global Variable
*******************************************************************************/
cy_stc_scb_uart_context_t CYBSP_UART_context;

/* String containing output to be displayed on the UART terminal */
char ADC_string[128];


#if DEBUG_PRINT
/* Variable used for tracking the print status */
volatile bool ENTER_LOOP = true;

/*******************************************************************************
* Function Name: check_status
********************************************************************************
* Summary:
*  Prints the error message.
*
* Parameters:
*  error_msg - message to print if any error encountered.
*  status - status obtained after evaluation.
*
* Return:
*  void
*
*******************************************************************************/
void check_status(char *message, cy_rslt_t status)
{
    char error_msg[50];

    sprintf(error_msg, "Error Code: 0x%08" PRIX32 "\n", status);

    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n=====================================================\r\n");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\nFAIL: ");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, message);
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, error_msg);
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n=====================================================\r\n");
}
#endif



/*******************************************************************************
* Function Name: main
********************************************************************************
 Summary:
*  System entrance point. This function performs
*  - initial setup of device
*  - configure 12-bit SAR ADC
*  - configure OpAmp in CTBm block
*  - configure the SCB block as UART interface
*  - checks the configured ADC mode
*  - convert ADC output codes into corresponding voltage
*  - prints the value of input voltage via UART interface
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_en_sar_status_t sarStatus;

    /* Variables for processing SAR ADC output */
    int16_t Ch0;                /* Variable holding ADC output code */
    int voltage_integer;        /* Integer part of ADC input voltage */
    int voltage_decimal;        /* Decimal part of ADC input voltage */
    float ADC_voltage = 0.0;    /* ADC input voltage (mV) */

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        /* insert error handling here */
        CY_ASSERT(CY_ASSERT_FAILED);
    }

#if DEBUG_PRINT
    /* Configure and enable the UART peripheral */
    Cy_SCB_UART_Init(CYBSP_UART_HW, &CYBSP_UART_config, &CYBSP_UART_context);
    Cy_SCB_UART_Enable(CYBSP_UART_HW);

    /* Send a string through UART peripheral to display during program start-up */
    Cy_SCB_UART_PutString(CYBSP_UART_HW,"\n\n12-bit SAR ADC with Op-Amp as buffer \r\n\n");

    /* Send a string through UART peripheral */
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "--------------------------------------------------------------\r\n");

#endif

    /* Initialize PASS 0 CTB 0 OpAmp 1 under CTBm block */
    cy_en_ctb_status_t status = Cy_CTB_OpampInit(CTBM0, CY_CTB_OPAMP_1, &CYBSP_CTB0_OPAMP1_config);

    if (CY_CTB_SUCCESS != status)
    {

#if DEBUG_PRINT
        check_status("API Cy_CTB_OpampInit failed with error code", status);
#endif

        /* Insert error handling here */
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    else
    {
        /* Continuous Time Block mini (CTBm) enabling */
        Cy_CTB_Enable(CTBM0);
    }

    /* Global interrupt enable */
    __enable_irq();



    /* SAR ADC initialization */
    sarStatus = Cy_SAR_Init(SAR0, &SARADC0_config);
    if (sarStatus != CY_SAR_SUCCESS)
    {
#if DEBUG_PRINT
        check_status("API Cy_SAR_Init failed with error code", sarStatus);
#endif
        /* insert error handling here */
        CY_ASSERT(CY_ASSERT_FAILED);
    }
    else
    {
        /* Enable SAR ADC */
        Cy_SAR_Enable(SAR0);
    }

    for(;;)
    {
        /* Start SAR conversion of analog sample values */
        Cy_SAR_StartConvert(SAR0, CY_SAR_START_CONVERT_SINGLE_SHOT);

        /* Delay of 2.304 milliseconds is required to finish conversion of an analog sample as detailed in README file.
         * This delay is implemented using a while loop and may be replaced by codes that can perform some useful task
         * in 2.304 milliseconds time interval. */

        /* 'CY_SAR_RETURN_STATUS' immediately returns the conversion status.
         * This is a non-blocking read mode
         * Hence, here a while-loop is used to check the conversion status before reading the ADC code */
        while(CY_SAR_SUCCESS != Cy_SAR_IsEndConversion(SAR0, CY_SAR_RETURN_STATUS))
        {
            /* This loop executes until ADC sample conversion completes, thereby providing the required conversion delay */
        }
        Ch0 = Cy_SAR_GetResult16(SAR0, CHANNEL_0);

        /* now 'Ch0' contains the ADC channel 0 result */

        ADC_voltage = (ADC_REFERENCE * 2) * Ch0/ADC_MAX_RESULT * 1000; /* ADC code to voltage conversion formula */

        voltage_integer = (int)ADC_voltage;
        voltage_decimal = (int)(10000 * (ADC_voltage - voltage_integer));
        if(voltage_decimal<0)
        {
            voltage_decimal = - voltage_decimal; /* Negative to positive conversion */
        }

#if DEBUG_PRINT
        /* conversion from uint32_t to char_t for UART transmit */
        sprintf(ADC_string,"Raw ADC data = %d ; Voltage measured = %d.%04d mV\r\n\n",Ch0, voltage_integer, voltage_decimal);

        /* Send a string through UART peripheral */
        Cy_SCB_UART_PutString(CYBSP_UART_HW, ADC_string);
#endif
        /* Blink the onboard LED*/
        Cy_GPIO_Inv(CYBSP_USER_LED_PORT,CYBSP_USER_LED_PIN);

        /* Delay required to slow down terminal display rate */
        /* Here, a delay of 500 milliseconds is added for better visibility of ADC codes on UART terminal */
        Cy_SysLib_Delay(UART_PRINT_DELAY);
#if DEBUG_PRINT
        if (ENTER_LOOP)
        {
            Cy_SCB_UART_PutString(CYBSP_UART_HW, "Entered for loop\r\n");
            ENTER_LOOP = false;
        }
#endif
    }
}
/* [] END OF FILE */
