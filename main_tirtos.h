/*
 * main_tirtos.h
 *
 *  Created on: 6 dic 2023
 */

#ifndef MAIN_TIRTOS_H_
#define MAIN_TIRTOS_H_

#include <ti/drivers/gpio/GPIOMSP432E4.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/power/PowerMSP432E4.h>
#include <ti/devices/msp432e4/driverlib/inc/hw_gpio.h>
#include <ti/devices/msp432e4/driverlib/sysctl.h>


#ifndef HWREG
#define HWREG(x) (*((volatile uint32_t *)(x)))
#endif

static const uint_fast16_t powerMgrIds[] = {
    PowerMSP432E4_PERIPH_GPIOA,     /* PORTA */
    PowerMSP432E4_PERIPH_GPIOB,     /* PORTB */
    PowerMSP432E4_PERIPH_GPIOC,     /* PORTC */
    PowerMSP432E4_PERIPH_GPIOD,     /* PORTD */
    PowerMSP432E4_PERIPH_GPIOE,     /* PORTE */
    PowerMSP432E4_PERIPH_GPIOF,     /* PORTF */
    PowerMSP432E4_PERIPH_GPIOG,     /* PORTG */
    PowerMSP432E4_PERIPH_GPIOH,     /* PORTH */
    PowerMSP432E4_PERIPH_GPIOJ,     /* PORTJ */
    PowerMSP432E4_PERIPH_GPIOK,     /* PORTK */
    PowerMSP432E4_PERIPH_GPIOL,     /* PORTL */
    PowerMSP432E4_PERIPH_GPIOM,     /* PORTM */
    PowerMSP432E4_PERIPH_GPION,     /* PORTN */
    PowerMSP432E4_PERIPH_GPIOR,     /* PORTR */
    PowerMSP432E4_PERIPH_GPIOS,     /* PORTS */
    PowerMSP432E4_PERIPH_GPIOT,     /* PORTT */
    PowerMSP432E4_PERIPH_GPIOP,     /* PORTP */
    PowerMSP432E4_PERIPH_GPIOQ,     /* PORTQ */
};

#define getPowerResource(port) (powerMgrIds[port])


#ifdef _LOGTASK_
// Add this to your global variables section (outside any function)
#define MAX_TASK_NAME_LEN 32

typedef struct {
    char taskName[MAX_TASK_NAME_LEN];
    uint32_t timestamp;
} TaskHistoryEntry;

typedef struct {
    TaskHistoryEntry history[TASK_HISTORY_SIZE];
    volatile uint32_t historyIndex;
    volatile uint32_t totalSwitches;
    char lastTask[MAX_TASK_NAME_LEN];
    char currentTask[MAX_TASK_NAME_LEN];
} TaskMonitor;

#ifdef _DEBUG_
// Add these global variables to track problematic tasks
typedef struct {
    Task_Handle handle;
    uint32_t firstSeen;
    uint32_t occurrenceCount;
    String identifiedName;
} ProblematicTask;

#define MAX_PROBLEMATIC_TASKS 10
ProblematicTask g_problematicTasks[MAX_PROBLEMATIC_TASKS];
uint32_t g_problematicTaskCount = 0;
#endif

#pragma RETAIN(g_taskMonitor)
volatile TaskMonitor g_taskMonitor = {0};
bool updateSwitchTaskName(Task_Handle taskHandle,  const char* context);
#ifdef _DEBUG_
void captureProblematicTask(Task_Handle task, const char* context);
#endif
#endif
void taskSwitchHook(Task_Handle prevTask, Task_Handle nextTask);
void taskCreateHook(Task_Handle task, Error_Block *eb);
void taskDeleteHook(Task_Handle task);
bool checkTaskToLog(char* taskNameToCheck);

static void GPIO_unlock(GPIO_PinConfig pinConfig);
static void reboot_system(UArg arg0, UArg arg1);
void err_sys_reboot ();
void err_sys_reboot_int (int status);

#endif /* MAIN_TIRTOS_H_ */
