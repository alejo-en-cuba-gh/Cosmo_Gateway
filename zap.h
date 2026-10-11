/*
 * zap.h
 *
 *  Created on: Aug 16, 2024
 *      Author: HelierDavidCamejoAra
 */

#ifndef ZAP_H_
#define ZAP_H_

#include "common.h"

#include "rpc.h"
#include "mtSys.h"
#include "mtZdo.h"
#include "mtUtil.h"
#include "mtAf.h"
#include "mtParser.h"
#include "mtSapi.h"
#include "mtApp_config.h"

/*********************************************************************
 * LOCAL FUNCTIONS
 */
//App Config Callbacks
//SAPI Callbacks
//helper functions
/*********************************************************************
 * CALLBACK FUNCTIONS
 */
/*MACROS*/
#define CONFIG_DEVICE_NRETRY    5
#define THERMB01_NSTEPS         11
#define HTHERMB1_NSTEPS         20

// CLUSTERS
#define BASIC                   0x0000
#define POWER_CONFIGURATION     0x0001
#define ON_OFF                  0x0006
#define LEVEL_CONTROL           0x0008
#define BINARY_INPUT_BASIC      0x000F // Cl�ster Binary Input
#define THERMOSTAT              0X0201
#define TEMPERATURE_MEASUREMENT 0x0402
#define HUMIDITY_MEASUREMENT    0x0405
#define DIAGNOSTIC              0x0B05
#define IAS_ZONE                0x0500
#define IAS_WD                  0x0502
#define METERING                0x0702
#define ASTREL_COMMON_FCC4      0xFCC4
#define ILLUMINANCE_MEASUREMENT 0x0400
#define WINDOW_COVERING         0x0102
#define HTHERMB1_CONFIG         0xFC06

// ATRIBUTOS
#define Firmware_Version            0x0001
#define Manufacterer_Name           0x0004
#define Model_Id                    0x0005
#define Power_Source                0x0007
#define On_Off                      0x0000
#define Level_Control               0x0000
#define PresentValue                0x0055 // Atributo PresentValue para Binary Input

#define Local_Temperature           0x0000
#define Outdoor_Temperature         0x0001
#define Occupied_Heating_Set        0x0012
#define Target_Temperature          0x0018
#define Control_Sequence_Operation  0x001B
#define System_Mode                 0x001C

#define Battery_Percent             0x0021
#define Battery_Alarm_State         0x003E
#define Temperature_Measurement     0x0000
#define Humidity_Measurement        0x0405

#define Diagnostic                  0x0B05
#define Last_LQI                    0x011C

#define Ias_Zone                    0x0000
#define Ias_WD                      0x0000
#define Instantaneous_demand        0x0400
#define Current_Summation_Delivered 0x0000
#define Iluminance_Measurement      0x0000
#define Rollershutter_Lift          0x0008


#endif /* ZAP_H_ */
