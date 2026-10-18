/*
 * rpcTransportUart.c
 *
 * This module contains the API for the zll SoC Host Interface.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*********************************************************************
 * INCLUDES
 */
#ifndef _RPCTRANSPORTUART_C_
#define _RPCTRANSPORTUART_C_

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>

#include <ti/drivers/UART.h>
#include "common.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
extern UART_Handle uart;

/*********************************************************************
 * API FUNCTIONS
 */

/*********************************************************************
 * @fn      rpcTransportOpen
 *
 * @brief   opens the serial port to the CC253x.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
int32_t rpcTransportOpen(char *_devicePath, uint32_t port)
{
    UART_Params uartParams;
    int32_t ret = -1;

    if (uart == NULL)
    {
        /* Create a UART with data processing off. */
        UART_Params_init(&uartParams);
        uartParams.readMode = UART_MODE_BLOCKING;
        uartParams.writeMode = UART_MODE_BLOCKING;
        uartParams.readTimeout = UART_WAIT_FOREVER;
        uartParams.writeTimeout = UART_WAIT_FOREVER;
        uartParams.readCallback = NULL;
        uartParams.writeCallback = NULL;
        uartParams.readReturnMode = UART_RETURN_FULL;
        uartParams.writeDataMode = UART_DATA_BINARY;
        uartParams.readDataMode = UART_DATA_BINARY;
        uartParams.readEcho = UART_ECHO_OFF;
        uartParams.baudRate = 115200;
        uartParams.dataLength = UART_LEN_8;
        uartParams.stopBits = UART_STOP_ONE;
        uartParams.parityType = UART_PAR_NONE;

        // init UART driver
        uart = UART_open(ZNP_UART, &uartParams);
        if (uart != NULL)
        {
            // return success
            ret = 0;
        }
    }

    return ret;
}

/*********************************************************************
 * @fn      rpcTransportClose
 *
 * @brief   closes the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
void rpcTransportClose(void)
{
    if (uart != NULL)
    {
        // call TI-RTOS driver
        UART_close(uart);
    }

    return;
}

/*********************************************************************
 * @fn      rpcTransportWrite
 *
 * @brief   Write to the the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
void rpcTransportWrite(uint8_t *buf, uint8_t len)
{
    if (uart != NULL)
    {
        // call TI-RTOS driver function
        UART_write(uart, (void*) buf, (size_t) len);
    }

    return;
}

/*********************************************************************
 * @fn      rpcTransportRead
 *
 * @brief   Reads from the the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
uint8_t rpcTransportRead(uint8_t *buf, uint8_t len)
{
    uint8_t ret = 0;
    int bytes;

    if (uart != NULL)
    {
        // call TI-RTOS driver function
        bytes = UART_read(uart, (void*) buf, (size_t) len);
        if (bytes != UART_ERROR)
        {
            // return number of read bytes
            ret = (uint8_t) bytes;
        }
    }

    return ret;
}

#endif
