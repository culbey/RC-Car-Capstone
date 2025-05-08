#include "task_motor.h"

// Structure for stepper motor command
typedef struct {
    int steps;
    motor_message_type_t dir;
} stepper_command_t;

// Queue for stepper motor commands
QueueHandle_t q_motor;

cyhal_pwm_t step_pwm;

static void task_step(void *param);

static BaseType_t cli_handler_stepper(
    char *pcWriteBuffer, 
    size_t xWriteBufferLen,
    const char *pcCommandString);


static const CLI_Command_Definition_t xMotor = 
{
    "motor",
    "\r\nmotor <f | b> <int>",
    cli_handler_stepper,
    2
};

volatile int step_counter = 0;

// Function to initialize GPIO for the stepper motor
static void pwm_callback_step(void *callback_arg, cyhal_pwm_event_t event) {
    if (event == CYHAL_PWM_IRQ_COMPARE) {
        step_counter++;
    }
}

// Stepper motor control task
static void task_step(void *param) {
    stepper_command_t command;

    //void(param);

    for (;;) {
        // Wait for a command from the queue
        xQueueReceive(q_motor, &command, portMAX_DELAY);
        cyhal_gpio_write(PIN_DIR, command.dir);
        cyhal_pwm_start(&step_pwm);
        while (step_counter < command.steps) {
            continue;
        }
        step_counter = 0;
        cyhal_pwm_stop(&step_pwm);
        cyhal_gpio_write(PIN_STEP, 0);
    }
}

// CLI command handler
static BaseType_t cli_handler_stepper(
    char *pcWriteBuffer, 
    size_t xWriteBufferLen,
    const char *pcCommandString) {
        
    const char *pcParameter;
    const char *pcParameter_steps;

    stepper_command_t command;

    BaseType_t xParameterStringLength, xParameterStringLength_steps, xReturn;

    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString, 
        1, 
        &xParameterStringLength
    );
    pcParameter_steps = FreeRTOS_CLIGetParameter(
        pcCommandString,
        2,
        &xParameterStringLength_steps
    );

    configASSERT(pcParameter);
    configASSERT(pcParameter_steps);

    command.steps = atoi(pcParameter_steps);

    if (strncmp(pcParameter, "f", xParameterStringLength) == 0) {
        command.dir = FORWARD;
    }
    else if (strncmp(pcParameter, "b", xParameterStringLength) == 0) {
        command.dir = BACKWARD;
    }
    else {
        memset(pcWriteBuffer, 0x00, xWriteBufferLen);
        sprintf(pcWriteBuffer, "Error: Invalid direction, use 'f' or 'b'.\n");
        xReturn = pdFALSE;
        return xReturn;
    }

    xQueueSendToBack(q_motor, &command, portMAX_DELAY);
    xReturn = pdFALSE;
    return xReturn;
}


void task_motor_init(void) {
    cy_rslt_t rslt;

    // rslt = cyhal_gpio_configure(PIN_DIR, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG);
    // CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    // rslt = cyhal_gpio_configure(PIN_STEP, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG);
    // CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    q_motor = xQueueCreate(1, sizeof(stepper_command_t));

    FreeRTOS_CLIRegisterCommand(&xMotor);


    xTaskCreate(
        task_step, 
        "Task_Step", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        configMAX_PRIORITIES - 6 , 
        NULL);

    rslt = cyhal_pwm_init(&step_pwm, PIN_STEP, NULL);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    rslt = cyhal_pwm_set_duty_cycle(&step_pwm, 50, 500);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    cyhal_pwm_register_callback(&step_pwm, pwm_callback_step, NULL);

    cyhal_pwm_enable_event(&step_pwm, CYHAL_PWM_IRQ_COMPARE, 3, true);

}
