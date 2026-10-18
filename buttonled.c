/*
 * Copyright (c) 2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *    ======== buttonled.c ========
 */
#include "common.h"
#include "buttonled.h"

static Button_Handle buttonHandle;
LED_Handle led_status = NULL;

/*
 *  ======== handleButtonCallback ========
 */
void handleButtonCallback(Button_Handle handle, Button_EventMask events)
{
    if (events & Button_EV_CLICKED)
    {
    }
    else if (events & Button_EV_LONGCLICKED)
    {
    }
    else if (events & Button_EV_DOUBLECLICKED)
    {
    }
}

void ledbutton()
{
    Button_Params buttonParams;
    LED_Params ledParams;

    /* Open button */
    Button_Params_init(&buttonParams);
    buttonParams.doublePressDetectiontimeout = 500000;
    buttonParams.buttonEventMask = Button_EV_CLICKED | Button_EV_DOUBLECLICKED | Button_EV_LONGCLICKED;
    buttonHandle = Button_open(BUTTON_1, handleButtonCallback, &buttonParams);

    /* Check if the button open is successful */
    if (buttonHandle == NULL)
    {
        // Button Open Failed
    }

    /* Open status LED with default params */
    LED_Params_init(&ledParams);
    led_status = LED_open(LED_STATUS_RED, &ledParams);
}

void ledstate(UArg arg0, UArg arg1)
{
    // create array with as many items as ledstate enum
    bool s[LS_ITEMS] = { false };
    msg_led_t l;
    uint8_t i, a = 0, m;

    while (1)
    {
        if (a)
        {
            // Active signals
            for (i = 0; i < LS_ITEMS; i++)
            {
                // If there are new messages, stop blink loop and update first
                m = Mailbox_pend(m_led, &l, BIOS_NO_WAIT);
                if (m)
                {
                    break;
                }

                if (s[i])
                {
                    // If tag is set, blink i+1 times
                    led_blink(i + 1);
                    // Wait 2 seconds before next signal
                    Task_sleep(2000);
                }
            }
        }
        else
        {
            // No signals at the moment
            LED_setOn(led_status, 100);
            m = Mailbox_pend(m_led, &l, BIOS_WAIT_FOREVER);
            LED_setOff(led_status);
            Task_sleep(250);
        }

        // New message
        if (m)
        {
            // Only react if state changes
            l.led--;
            if (s[l.led] != l.state)
            {
                s[l.led] = l.state;

                if (l.state)
                {
                    a++;
                }
                else
                {
                    a--;
                }
            }
        }
    }
}

static void led_blink(uint8_t t)
{
    uint8_t i;

    // Blink t-times
    for (i = 0; i < t; i++)
    {
        LED_setOff(led_status);
        Task_sleep(300);
        LED_setOn(led_status, 100);
        Task_sleep(300);
    }

    LED_setOff(led_status);
}
