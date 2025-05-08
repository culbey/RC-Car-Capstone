#include "task_dcmot.h"

QueueHandle_t q_dcmot;

static void task_dcmot(void *param);

cyhal_pwm_t pwm_left1;
cyhal_pwm_t pwm_left2;

cyhal_pwm_t pwm_right1;
cyhal_pwm_t pwm_right2;

TimerHandle_t motor_timer;

volatile int motor_sel = 0;

static BaseType_t cli_handler_dcmot(char *pcWriteBuffer,
                                    size_t xWriteBufferLen,
                                    const char *pcCommandString);

static const CLI_Command_Definition_t xDcmot = {
    "dcmot",
    "\r\n dcmot <>",
    cli_handler_dcmot,
    2
};


pwm_type_t set_pwms(ble_message_type_t *message) {
 
  pwm_type_t duties;
  
  // This function will analyze the message and return the appropriate command
  // For now, we will just return the message as is
  int8_t right_speed = message->right_motor;
  int8_t left_speed = message->left_motor;

  if (right_speed == 0 && left_speed == 0) {
    duties.right_duty = 0;
    duties.left_duty = 0;
    return duties;
  }

  if (right_speed > MOTOR_LIMIT) {
    right_speed = MOTOR_LIMIT;
  } else if (right_speed < -MOTOR_LIMIT) {
    right_speed = -MOTOR_LIMIT;
  }

  if (left_speed > MOTOR_LIMIT) {
    left_speed = MOTOR_LIMIT;
  } else if (left_speed < -MOTOR_LIMIT) {
    left_speed = -MOTOR_LIMIT;
  }

    //if (left_speed > 80) left_speed -= 10;  // left motor is stronger

    duties.right_duty = (right_speed);
    duties.left_duty = (left_speed);

    // MORE CASES ARE GOING TO BE NEEDED



  return duties;
}

static void set_left_motor(int8_t duty, bool stop) {
    
      //Set Pin1 to 1 all time for forward
      //Set Pin2 to Duty cycle of 30% for forward slow

      /* Stop the PWM output */
      cyhal_pwm_stop(&pwm_left1);
      cyhal_pwm_stop(&pwm_left2);


      if (stop) {
        /* Stop car */
        cyhal_pwm_set_duty_cycle(&pwm_left1, 100, MOTOR_FREQ);
        cyhal_pwm_set_duty_cycle(&pwm_left2, 100, MOTOR_FREQ);
      } else if (duty < 0) {
        /* Reverse */
        cyhal_pwm_set_duty_cycle(&pwm_left1, abs(duty), MOTOR_FREQ);
        cyhal_pwm_set_duty_cycle(&pwm_left2, 0, MOTOR_FREQ);
      } else {
        /* Forward */
        cyhal_pwm_set_duty_cycle(&pwm_left1, 0, MOTOR_FREQ);
        cyhal_pwm_set_duty_cycle(&pwm_left2, abs(duty), MOTOR_FREQ);
      }

      /* Start the PWM output */
      vTaskDelay(pdMS_TO_TICKS(5));
      cyhal_pwm_start(&pwm_left1);
      //vTaskDelay(50);
      cyhal_pwm_start(&pwm_left2);

}

static void set_right_motor(int8_t duty, bool stop) {
    //Set Pin1 to 1 all time for forward
      //Set Pin2 to Duty cycle of 30% for forward slow

      /* Stop the PWM output */
      cyhal_pwm_stop(&pwm_right1);
      cyhal_pwm_stop(&pwm_right2);

      if (stop) {
        /* Stop car */
        cyhal_pwm_set_duty_cycle(&pwm_right1, 100, MOTOR_FREQ);
        cyhal_pwm_set_duty_cycle(&pwm_right2, 100, MOTOR_FREQ);
      } else if (duty < 0) {
        /* Reverse */
        cyhal_pwm_set_duty_cycle(&pwm_right1, 0, MOTOR_FREQ);
        cyhal_pwm_set_duty_cycle(&pwm_right2, abs(duty), MOTOR_FREQ);
      } else {
        /* Forward */
        cyhal_pwm_set_duty_cycle(&pwm_right1, abs(duty), MOTOR_FREQ);
        cyhal_pwm_set_duty_cycle(&pwm_right2, 0, MOTOR_FREQ);
      }
      

      /* Start the PWM output */
      vTaskDelay(pdMS_TO_TICKS(5));
      cyhal_pwm_start(&pwm_right1);
      //vTaskDelay(pdMS_TO_TICKS(50));
      cyhal_pwm_start(&pwm_right2);
}

static void task_dcmot(void *param) {
  ble_message_type_t message;
  pwm_type_t duty_cycles;

  int8_t last_left = 0;
  int8_t last_right = 0;
  int8_t dif;

  message.left_motor = 0;
  message.right_motor = 0;
  duty_cycles = set_pwms(&message);
  set_right_motor(duty_cycles.right_duty, false);
  set_left_motor(duty_cycles.left_duty, false);

  initialize_connection();
  vTaskDelay(100);
  ble_app_write_notification_cccd(true);
  vTaskDelay(100);
  
  xTaskNotifyGive(Sensor_Task);

  for (;;)
  {
    //RECEIVE MESSAGE FROM BLE
    //NOT NECESSARILY xQueueReceive
    xQueueReceive(q_dcmot, &message, portMAX_DELAY);

    if (motor_sel != 0) {
        // CLI CMD
        if (motor_sel == 1) {
            set_left_motor(message.left_motor, false);
        } else if (motor_sel == 2) {
            set_right_motor(message.right_motor, false);
        }
        motor_sel = 0;

    } else if ( (message.left_motor == (int8_t) DUTY_STOP) && (message.right_motor == (int8_t) DUTY_STOP) ) {
        // normal controller should never be able to send -128 R and -128 L
        set_right_motor(DUTY_STOP, true);
        set_left_motor(DUTY_STOP, true);

        last_left = 0;
        last_right = 0;
    } else {

        duty_cycles = set_pwms(&message);

        // dif = duty_cycles.left_duty - last_left;
        // if (abs(dif) > MOTOR_MAX_CHANGE) {
        //   if (duty_cycles.left_duty > 0) {
        //     duty_cycles.left_duty -= MOTOR_MAX_CHANGE;
        //   } else {
        //     duty_cycles.left_duty += MOTOR_MAX_CHANGE;
        //   }
        // }

        // dif = duty_cycles.right_duty - last_right;
        // if (abs(dif) > MOTOR_MAX_CHANGE) {
        //   if (duty_cycles.right_duty > 0) {
        //     duty_cycles.right_duty -= MOTOR_MAX_CHANGE;
        //   } else {
        //     duty_cycles.right_duty += MOTOR_MAX_CHANGE;
        //   }
        // }

        set_right_motor(duty_cycles.right_duty, false);
        set_left_motor(duty_cycles.left_duty, false);

        last_left = duty_cycles.left_duty;
        last_right = duty_cycles.right_duty;
    }
    
    // task_print("Left Motor: %03d\n", message.left_motor);
    // task_print("Right Motor: %03d\n", message.right_motor);
    vTaskDelay(pdMS_TO_TICKS(5));
    
  }
}

static BaseType_t cli_handler_dcmot(char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString) {

const char *pcParameter;
const char *pcParameter_mot;

dcmot_message_type_t drive_speed;


BaseType_t xParameterStringLen, xReturn, xParameterStringLen_mot;

(void)pcCommandString;
(void)xWriteBufferLen;
configASSERT(pcWriteBuffer);    

pcParameter_mot = FreeRTOS_CLIGetParameter(
pcCommandString,
1,
&xParameterStringLen_mot);

pcParameter = FreeRTOS_CLIGetParameter(
pcCommandString,
2,
&xParameterStringLen);

configASSERT(pcParameter_mot);
configASSERT(pcParameter);

if (atoi(pcParameter_mot) == 1)
{
    motor_sel = 1;
}
else if (atoi(pcParameter_mot) == 2)
{
    motor_sel = 2;
}
else {
    task_print_error("Command format is: dcmot <1/2 forward/backward_slow/medium/fast>\n");
    return pdFALSE;
}

if (strcmp(pcParameter, "off") == 0)
{
    drive_speed = DRIVE_OFF;
}
else if (strcmp(pcParameter, "forward_slow") == 0)
{
    drive_speed = DRIVE_FORWARD_SLOW;
}
else if (strcmp(pcParameter, "forward_medium") == 0)
{
    drive_speed = DRIVE_FORWARD_MEDIUM;
}
else if (strcmp(pcParameter, "forward_fast") == 0)
{
    drive_speed = DRIVE_FORWARD_FAST;
}
else if (strcmp(pcParameter, "backwards_slow") == 0)
{
    drive_speed = DRIVE_BACKWARDS_SLOW;
}
else if (strcmp(pcParameter, "backwards_medium") == 0)
{
    drive_speed = DRIVE_BACKWARDS_MEDIUM;
}
else if (strcmp(pcParameter, "backwards_fast") == 0)
{
    drive_speed = DRIVE_BACKWARDS_FAST;
}
else {
    task_print_error("Command format is: dcmot <1/2 forward/backward_slow/medium/fast>\n");
    return pdFALSE;
}

ble_message_type_t motor_speeds;
if (motor_sel == 1) {
    motor_speeds.left_motor = (int8_t) drive_speed;
    motor_speeds.right_motor = 0x00;
} else {
    motor_speeds.right_motor = (int8_t) drive_speed;
    motor_speeds.left_motor = 0x00;
}
xQueueSendToBack(q_dcmot, &motor_speeds, portMAX_DELAY);

xReturn = pdFALSE;
return xReturn;
}

void task_dcmot_init(void) {

cy_rslt_t rslt; 

q_dcmot = xQueueCreate(1, sizeof(ble_message_type_t));
configASSERT(q_dcmot);

// Initialize the pins that controls the drive controller
// Pin 1
/* Initialize PWM on the supplied pin and assign a new clock */
rslt = cyhal_pwm_init(&pwm_left1, PIN_DRIVE1, NULL);
CY_ASSERT(CY_RSLT_SUCCESS == rslt);

// Pin 2
/* Initialize PWM on the supplied pin and assign a new clock */
rslt = cyhal_pwm_init(&pwm_left2, PIN_DRIVE2, NULL);
CY_ASSERT(CY_RSLT_SUCCESS == rslt);

/* Second motor */
// Pin 1
rslt = cyhal_pwm_init(&pwm_right1, PIN_2DRIVE1, NULL);
CY_ASSERT(CY_RSLT_SUCCESS == rslt);

// Pin 2
/* Initialize PWM on the supplied pin and assign a new clock */
rslt = cyhal_pwm_init(&pwm_right2, PIN_2DRIVE2, NULL);
CY_ASSERT(CY_RSLT_SUCCESS == rslt);



FreeRTOS_CLIRegisterCommand(&xDcmot);

xTaskCreate(task_dcmot, 
"task_dcmot", 
configMINIMAL_STACK_SIZE * 15, 
NULL, 
configMAX_PRIORITIES - 6, 
NULL);

//   motor_timer = xTimerCreate(
//     "Motor Timer",
//     pdMS_TO_TICKS(500),
//     true,
//     0,
//     timer_callback_motor);

}   


// static void timer_callback_motor(TimerHandle_t xTimer) {
//     cy_rslt_t rslt = CY_RSLT_SUCCESS;
//     EventBits_t sensor_bits = xEventGroupGetBits(sensor_event_group);
//     // task_print("Timer Callback: %d\n", sensor_bits);
//     if (sensor_bits & MOTOR_STOP_BIT) {
//       //Test Enters Function
//       task_print("Entering Drive Off motor stop");
//       //Set Pins to 1/1 all time to brake
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_left1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_left1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_left1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_left2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_left2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_left2);
//        //Test Enters Function
//       task_print("Entering Drive2 Off motor stop");
//       //Set Pins to 1/1 all time to brake
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_right1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_right1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_right1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_right2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_right2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_right2);

//       /****** ROTATE THE CAR ******/
//       //Test Enters Function
//       task_print("Entering Drive Forward Slow for motor 1");
//       //Set Pin1 to 1 all time for forward
//       //Set Pin2 to Duty cycle of 30% for forward slow
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_left1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_left1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_left1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_left2);
//       /* Set a duty cycle of 30% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_left2, 70, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_left2);

//       task_print("Entering Drive2 Backwards Slow for motor 2");
//       //Set Pin1 to Duty cycle of 30% for backwards slow
//       //Set Pin2 to 1 all time for backwards
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_right1);
//       /* Set a duty cycle of 30% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_right1, 70, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_right1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_right2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_right2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_right2);

//       vTaskDelay(pdMS_TO_TICKS(1000)); //1000 will change

//       /* STOP MOTOR AND WAIT FOR NEXT COMMAND */

//       //Test Enters Function
//       task_print("Entering Drive Off motor stop");
//       //Set Pins to 1/1 all time to brake
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_left1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_left1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_left1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_left2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_left2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_left2);
//        //Test Enters Function
//       task_print("Entering Drive2 Off motor stop");
//       //Set Pins to 1/1 all time to brake
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_right1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_right1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_right1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_right2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_right2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_right2);


//     }
//     xEventGroupClearBits(sensor_event_group, MOTOR_STOP_BIT);
//     if (rslt != CY_RSLT_SUCCESS) {
//         task_print_error("Error in timer callback: %d\n", rslt);
//     }
// }




// static void task_dcmot(void *param) {
//   dcmot_message_type_t message_type;

//   cy_rslt_t rslt = CY_RSLT_SUCCESS;

//   /* Suppress warning for unused parameter */
// //   void(param);
//   /* Repeatedly running part of the task */
//   for (;;)
//   {
//     // Check the Queue.  If nothing was in the queue, we should return pdFALSE
//     xQueueReceive(q_dcmot, &message_type, portMAX_DELAY);
//     if (motor_sel == 0) {
//         continue;
//     }

//     if (motor_sel == 1) {
//     if (message_type == DRIVE_OFF)
//     {
//       //Test Enters Function
//       task_print("Entering Drive Off");
//       //Set Pins to 1/1 all time to brake
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive2);
//     }
//     else if (message_type == DRIVE_FORWARD_SLOW)
//     {
//       //Test Enters Function
//       task_print("Entering Drive Forward Slow");
//       //Set Pin1 to 1 all time for forward
//       //Set Pin2 to Duty cycle of 30% for forward slow
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive2);
//       /* Set a duty cycle of 30% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive2, 70, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive2);
//     }
//     else if (message_type == DRIVE_FORWARD_MEDIUM)
//     {
//       //Test Enters Function
//       task_print("Entering Drive Forward Medium");
//       //Set Pin1 to 1 all time for forward
//       //Set Pin2 to Duty cycle of 60% for forward medium
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive2);
//       /* Set a duty cycle of 60% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive2, 60, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive2);
//     }
//     else if (message_type == DRIVE_FORWARD_FAST)
//     {
//       //Test Enters Function
//       task_print("Entering Drive Forward Fast");
//       //Set Pin1 to 1 all time for forward
//       //Set Pin2 to Duty cycle of 90% for forward fast
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive2);
//       /* Set a duty cycle of 90% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive2, 30, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive2);
//     }
//     else if (message_type == DRIVE_BACKWARDS_SLOW)
//     {
//       //Test Enters Function
//       task_print("Entering Drive Backwards Slow");
//       //Set Pin1 to Duty cycle of 30% for backwards slow
//       //Set Pin2 to 1 all time for backwards
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive1);
//       /* Set a duty cycle of 30% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive1, 70, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive2);
//     }
//     else if (message_type == DRIVE_BACKWARDS_MEDIUM)
//     {
//       //Test Enters Function
//       task_print("Entering Drive Backwards Medium");
//       //Set Pin1 to Duty cycle of 60% for backwards medium
//       //Set Pin2 to 1 all time for backwards
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive1);
//       /* Set a duty cycle of 60% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive1, 60, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive2);
//     }
//     else if (message_type == DRIVE_BACKWARDS_FAST)
//     {
//       //Test Enters Function
//       task_print("Entering Drive Backwards Fast");
//       //Set Pin1 to Duty cycle of 90% for backwards fast
//       //Set Pin2 to 1 all time for backwards
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive1);
//       /* Set a duty cycle of 90% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive1, 30, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_drive2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_drive2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_drive2);
//     }
//   }
//   else if (motor_sel == 2) {
//         if (message_type == DRIVE_OFF)
//     {
//       //Test Enters Function
//       task_print("Entering Drive2 Off");
//       //Set Pins to 1/1 all time to brake
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive2);
//     }
//     else if (message_type == DRIVE_FORWARD_SLOW)
//     {
//       //Test Enters Function
//       task_print("Entering Drive2 Forward Slow");
//       //Set Pin1 to 1 all time for forward
//       //Set Pin2 to Duty cycle of 30% for forward slow
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive2);
//       /* Set a duty cycle of 30% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive2, 70, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive2);
//     }
//     else if (message_type == DRIVE_FORWARD_MEDIUM)
//     {
//       //Test Enters Function
//       task_print("Entering Drive2 Forward Medium");
//       //Set Pin1 to 1 all time for forward
//       //Set Pin2 to Duty cycle of 60% for forward medium
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive2);
//       /* Set a duty cycle of 60% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive2, 60, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive2);
//     }
//     else if (message_type == DRIVE_FORWARD_FAST)
//     {
//       //Test Enters Function
//       task_print("Entering Drive2 Forward Fast");
//       //Set Pin1 to 1 all time for forward
//       //Set Pin2 to Duty cycle of 90% for forward fast
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive1);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive1, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive2);
//       /* Set a duty cycle of 90% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive2, 30, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive2);
//     }
//     else if (message_type == DRIVE_BACKWARDS_SLOW)
//     {
//       //Test Enters Function
//       task_print("Entering Drive2 Backwards Slow");
//       //Set Pin1 to Duty cycle of 30% for backwards slow
//       //Set Pin2 to 1 all time for backwards
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive1);
//       /* Set a duty cycle of 30% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive1, 70, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive2);
//     }
//     else if (message_type == DRIVE_BACKWARDS_MEDIUM)
//     {
//       //Test Enters Function
//       task_print("Entering Drive2 Backwards Medium");
//       //Set Pin1 to Duty cycle of 60% for backwards medium
//       //Set Pin2 to 1 all time for backwards
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive1);
//       /* Set a duty cycle of 60% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive1, 60, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive2);
//     }
//     else if (message_type == DRIVE_BACKWARDS_FAST)
//     {
//       //Test Enters Function
//       task_print("Entering Drive2 Backwards Fast");
//       //Set Pin1 to Duty cycle of 90% for backwards fast
//       //Set Pin2 to 1 all time for backwards
//       // Pin 1
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive1);
//       /* Set a duty cycle of 90% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive1, 30, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive1);

//       // Pin 2
//       /* Stop the PWM output */
//       rslt = cyhal_pwm_stop(&pwm_2drive2);
//       /* Set a duty cycle of 100% and frequency of 1Hz */
//       rslt = cyhal_pwm_set_duty_cycle(&pwm_2drive2, 100, 100);
//       /* Start the PWM output */
//       rslt = cyhal_pwm_start(&pwm_2drive2);
//     }
//   }

//     if (rslt != CY_RSLT_SUCCESS) {
//         task_print_error("Error setting PWM output\n");
//     }
//   }
// }

