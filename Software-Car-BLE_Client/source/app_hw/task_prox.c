#include "task_prox.h"

typedef struct
{
    int num;
    int step;
} prox_command_t;

QueueHandle_t q_prox;

VL53L4CD_ResultsData_t prox_data;

static void task_prox(void *param);

static BaseType_t cli_handler_prox(char *pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char *pcCommandString);

volatile int read_counter = 0;
volatile int timer_counter = 0;
int timer_mod = 1;

static void timer_callback_prox(TimerHandle_t xTimer);

TimerHandle_t prox_timer;

VL53L4CD_ResultsData_t prox_output;

Dev_inst_t dev_inst = 0x29;

static const CLI_Command_Definition_t xProx =
    {
        "prox",
        "\r\n prox <(int) num reads> <(int) step time(ms>100)>",
        cli_handler_prox,
        2};

static void timer_callback_prox(TimerHandle_t xTimer)
{
    // read_i2c_value
    timer_counter++;
}

static void task_prox(void *param)
{
    prox_command_t command;
    // void(param);
    // cy_rslt_t rslt = i2c_init();
    // CY_ASSERT(CY_RSLT_SUCCESS == rslt);
    uint8_t status = VL53L4CD_SensorInit(dev_inst);
    if (status)
    {
        task_print_error("Unable to initialze sensor");
    }

    for (;;)
    {
        // Wait for command
        if (xQueueReceive(q_prox, &command, portMAX_DELAY) == pdFALSE) {
            continue;
        }
        timer_mod = command.step / 100;
        VL53L4CD_Error err = VL53L4CD_StartRanging(dev_inst);
        cyhal_system_delay_ms(1000);
        xTimerStart(prox_timer, portMAX_DELAY);
        uint16 output = 0;
        while (read_counter < command.num) {
                if ((timer_counter % timer_mod) == 0)
                {
                    uint8_t data_rdy;
                    err = VL53L4CD_CheckForDataReady(dev_inst, &data_rdy);
                    if (data_rdy)
                    {
                        VL53L4CD_ClearInterrupt(dev_inst);
                        err = VL53L4CD_GetResult(dev_inst, &prox_output);
                        output = (((prox_output.distance_mm & 0xFF00) >> 8) | ((prox_output.distance_mm & 0x00FF) << 8));
                        task_print("\nProximity Sensor Output in millimeters: %u\n", output);
                    }
                    else {
                        continue;
                    }
                        if (err)
                    {
                        task_print_error("Error while getting data from sensor.\n");
                    }
                    read_counter++;
                }
        }
        err = VL53L4CD_StopRanging(dev_inst);
        if (err)
        {
            task_print_error("Error while starting/stopping ranging of sensor.\n");
        }
        xTimerStop(prox_timer, portMAX_DELAY);
        read_counter = 0;
        timer_counter = 0;
    }
}

static BaseType_t cli_handler_prox(char *pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char *pcCommandString)
{
    const char *pcParameter_num;
    const char *pcParameter_step;

    prox_command_t command;

    BaseType_t xParameterStringLen_num, xParameterStringLen_step, xReturn;

    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    pcParameter_num = FreeRTOS_CLIGetParameter(
        pcCommandString,
        1,
        &xParameterStringLen_num);

    pcParameter_step = FreeRTOS_CLIGetParameter(
        pcCommandString,
        2,
        &xParameterStringLen_step);

    configASSERT(pcParameter_num);
    configASSERT(pcParameter_step);

    command.num = atoi(pcParameter_num);
    command.step = atoi(pcParameter_step);

    if (command.num < 1)
    {
        memset(pcWriteBuffer, 0x00, xWriteBufferLen);
        sprintf(pcWriteBuffer, "Error: Invalid num, must be positive integer > 0.\n");
        xReturn = pdFALSE;
        return xReturn;
    }

    if (command.step < 100 || ((command.step % 100) != 0))
    {
        memset(pcWriteBuffer, 0x00, xWriteBufferLen);
        sprintf(pcWriteBuffer, "Error: Invalid step, must be integer multiple of 100 greater than 100.\n -> (1000 = One read per second)\n");
        xReturn = pdFALSE;
        return xReturn;
    }

    xQueueSendToBack(q_prox, &command, portMAX_DELAY);
    xReturn = pdFALSE;
    return xReturn;
}

void task_prox_init(void)
{
    FreeRTOS_CLIRegisterCommand(&xProx);

    q_prox = xQueueCreate(1, sizeof(prox_command_t));

    prox_timer = xTimerCreate(
        "Prox Timer",
        pdMS_TO_TICKS(100),
        true,
        0,
        timer_callback_prox);
    
    xTaskCreate(
        task_prox,
        "Task_Prox",
        5*configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL);


}
