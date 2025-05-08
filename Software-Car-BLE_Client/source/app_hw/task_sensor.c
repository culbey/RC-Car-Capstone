#include "task_sensor.h"

VL53L4CD_ResultsData_t sensor_data;

uint16 running_avg = 0;

// holds the last 5 sensor samples to compute the average
#define NUM_SAMPLES 10
#define SENSOR_DIST 600
uint16 samples[NUM_SAMPLES];

TaskHandle_t Sensor_Task;


volatile sensor_message_type_t command;

static void task_sensor(void *param);

static BaseType_t cli_handler_sensor(char *pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char *pcCommandString);

Dev_inst_t dev_addr = 0x29;

static const CLI_Command_Definition_t xSensor =
    {
        "sensor",
        "\r\n sensor <(1/on) or (0/off)>",
        cli_handler_sensor,
        1
    };


void update_running_avg(uint16 new_val) {
    static int index = 0;

    int total = 0;
    int max = 0;
    int min = 0xFFFF;

    samples[index] = (new_val < 50) ? (0xFFFF) : new_val;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        if (samples[i] > max) max = samples[i];
        else if (samples[i] < min) min = samples[i];



        total += samples[i];
    }

    /* 
    If this doesnt work, calculate the slop using max - min / samples
    check to ensure slope is negative at around -50
    check a couple points throughout the samples to ensure they are around where they should be
    */
    bool valid_avg = true;
    int first_sample_ind = (index + 1) % NUM_SAMPLES;

    if (samples[first_sample_ind] != max) valid_avg = false;
    if (samples[index] != min) valid_avg = false;
    if ( (max - min) > 200 ) valid_avg = false;

    running_avg = (valid_avg) ? total / NUM_SAMPLES : 0xFFFF;
    index = first_sample_ind;
    
}

void stop_car(void) {
    // stop car
    ble_message_type_t message;
    message.left_motor = DUTY_STOP;
    message.right_motor = DUTY_STOP;

    xQueueSendToBack(q_dcmot, &message, portMAX_DELAY);
    // wait a little
    // send motor speeds to motor task to rotate car
    // wait for some time until car has turned around
    // notify sensor task to resume reading
}

void turn_car(void) {
    ble_message_type_t message;

    // Spin car
    message.left_motor = -75;
    message.right_motor = 75;
    xQueueSendToBack(q_dcmot, &message, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(1150));    // Update this value to get correct turn amount

    // stop motors (free spin, not hard stop)
    message.left_motor = 0;
    message.right_motor = 0;
    xQueueSendToBack(q_dcmot, &message, portMAX_DELAY);
}

static void task_sensor(void *param)
{
    // void(param);

    uint8_t status = VL53L4CD_SensorInit(dev_addr);
    if (status)
    {
        task_print_error("Unable to initialze sensor\n");
    }



    uint8_t data_rdy;
    VL53L4CD_Error err;
    uint16 output = 0;

    err = VL53L4CD_StartRanging(dev_addr);
    if (err) {
        task_print_error("error starting ranging\n");
    }

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 

    uint8_t iteration = 0;

    for (;;)
    {

        vTaskDelay(10);
        
        err = VL53L4CD_CheckForDataReady(dev_addr, &data_rdy);

        if (err) {
            task_print_error("Unable to check for data ready\n");
            continue;
        } 
        if (data_rdy == 0) continue;


        VL53L4CD_ClearInterrupt(dev_addr);
        err = VL53L4CD_GetResult(dev_addr, &sensor_data);

        if (err) {
            task_print_error("Unable to get result from sensor\n");
            continue;
        }

        output = (((sensor_data.distance_mm & 0xFF00) >> 8) | ((sensor_data.distance_mm & 0x00FF) << 8));

        if (command == SENSOR_ON && (iteration % 5) == 0) {
            task_print("\nProximity Sensor Output in millimeters: %u\n", output);
        }
        
        update_running_avg(output);

        if (output != 0 && output < SENSOR_DIST && running_avg != 0 && running_avg < SENSOR_DIST)
        {
            if (command == SENSOR_ON) {
                task_print("\nProximity Sensor Car Stopped\n");
            }
            // SEND NOTIFICATION TO CONTROLLER BOARD
            STOP = true;
            stop_car();

            ble_app_write_notification_cccd(false);
            vTaskDelay(pdMS_TO_TICKS(100));
            ble_app_write_sensor_stop(true);
            vTaskDelay(pdMS_TO_TICKS(2000));
            turn_car();
            
            // insert an invalid sample to ensure we dont stop right away again
            update_running_avg(0);
            ble_app_write_sensor_stop(false);
            vTaskDelay(pdMS_TO_TICKS(100));
            ble_app_write_notification_cccd(true);
            STOP = false;
            
        }
        iteration++;

    }
}

static BaseType_t cli_handler_sensor(char *pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char *pcCommandString)
{
    const char *pcParameter;

    BaseType_t xParameterStringLen, xReturn;

    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,
        1,
        &xParameterStringLen);

    configASSERT(pcParameter);

    if ((strcmp(pcParameter, "on") == 0) || (atoi(pcParameter) == 1))
    {
        command = SENSOR_ON;
    }
    else if ((strcmp(pcParameter, "off") == 0) || (atoi(pcParameter) == 0))
    {
        command = SENSOR_OFF;
    }
    else {
        task_print_error("Command format is: sensor <on/off> XOR sensor <1/0>\n");
    }


    xReturn = pdFALSE;
    return xReturn;
}

void task_sensor_init(void)
{
    FreeRTOS_CLIRegisterCommand(&xSensor);

    // clear samples array
    for (int i = 0; i < NUM_SAMPLES; i++) samples[i] = 0xFFFF;
    
    
    xTaskCreate(
        task_sensor,
        "Task_Sensor",
        5*configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 6,
        &Sensor_Task);
   

}
