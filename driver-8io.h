/*
 * driver-8io.h
 *
 *  Created on: 6 nov 2023
 */

#ifndef DRIVER_8IO_H_
#define DRIVER_8IO_H_

void get_8io(uint_least8_t index);
static void event_msg (uint8_t port, bool state);
void pause_gpio(UArg arg0, UArg arg1);

#endif /* DRIVER_8IO_H_ */
