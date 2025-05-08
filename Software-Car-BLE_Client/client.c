/******************************************************************************
 * File Name: cts_server.c
 *
 * Description: This file contains the Bluetooth LE stack event handler, button
 *              interrupt handler and button task. Scanning is started on button
 *              press and notifications will be sent to the client when it enabled.
 *
 * Related Document: See README.md
 *
 *******************************************************************************
 * Copyright 2020-2024, Cypress Semiconductor Corporation (an Infineon company) or
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
 *        Header Files
 *******************************************************************************/
#include "wiced_bt_stack.h"
#include "cybsp.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "cycfg_gatt_db.h"
#include "cycfg_bt_settings.h"
#include "cycfg_gap.h"
#include "wiced_bt_dev.h"
#include "app_bt_utils.h"
#include "client.h"
#include <stdlib.h>

/*******************************************************************************
 *        Variable Definitions
 *******************************************************************************/
static uint16_t bt_connection_id = 0;
static bool notify_val = false;
static bool sensor = false;
static cts_discovery_data_t cts_discovery_data;
volatile bool STOP = false;

/*******************************************************************************
 *        Function Prototypes
 *******************************************************************************/
static void ble_app_init(void);
static void ctss_scan_result_cback(wiced_bt_ble_scan_results_t *p_scan_result,
                                   uint8_t *p_adv_data);

/* GATT Event Callback Functions */

static wiced_bt_gatt_status_t ble_app_connect_handler(wiced_bt_gatt_connection_status_t *p_conn_status);

static wiced_bt_gatt_status_t ble_app_gatt_event_callback(wiced_bt_gatt_evt_t event,
                                                          wiced_bt_gatt_event_data_t *p_event_data);

static wiced_bt_gatt_status_t ble_app_discovery_result_handler(wiced_bt_gatt_discovery_result_t *discovery_result);
static wiced_bt_gatt_status_t ble_app_service_discovery_handler(wiced_bt_gatt_discovery_complete_t *discovery_complete);

static void *app_alloc_buffer(int len);
static void app_free_buffer(uint8_t *p_event_data);
gatt_db_lookup_table_t *app_get_attribute(uint16_t handle);

/* Configure GPIO interrupt. */
cyhal_gpio_callback_data_t button_cb_data =
    {
        .callback = button_interrupt_handler,
        .callback_arg = NULL};

static const uint8_t hello_notify_uuid[] = {
    0x38, 0x28, 0x2E, 0x5F,
    0xA5, 0x1E,
    0xC7, 0xA4,
    0xC2, 0x46,
    0x47, 0x74,
    0xB6, 0xC7,
    0x81, 0x2F};

static const uint8_t notify_characteristic_uuid[] = {
    0xBD, 0xED, 0x5E, 0xC7, 0x28, 0x3A,
    0x86, 0xAD,
    0xAF, 0x44,
    0x45, 0x9F,
    0x3D, 0x62, 0xEA, 0x29};

static const uint8_t sensor_stop_uuid[] = {
    0x8E, 0xB1, 0x5D, 0xFA, 0x97, 0x33,
    0x5A, 0xA1,
    0x61, 0x41,
    0xA7, 0x13,
    0x50, 0x36, 0xAD, 0xD2};
/********************************************************************************
 * Function Name: app_bt_management_callback
 *********************************************************************************
 * Summary:
 *   This is a Bluetooth stack event handler function to receive management events from
 *   the BLE stack and process as per the application.
 *
 * Parameters:
 *   wiced_bt_management_evt_t event             : BLE event code of one byte length
 *   wiced_bt_management_evt_data_t *p_event_data: Pointer to BLE management event
 *                                                 structures
 *
 * Return:
 *  wiced_result_t: Error code from WICED_RESULT_LIST or BT_RESULT_LIST
 *
 *********************************************************************************/
wiced_result_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                          wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    wiced_bt_device_address_t bda = {0};

    switch (event)
    {
    case BTM_ENABLED_EVT:
        /* Bluetooth Controller and Host Stack Enabled */

        if (WICED_BT_SUCCESS == p_event_data->enabled.status)
        {
            /* Bluetooth is enabled */
            wiced_bt_dev_read_local_addr(bda);
            //printf("Local Bluetooth Address: ");
            print_bd_address(bda);

            /* Perform application-specific initialization */
            ble_app_init();
        }

        break;

    case BTM_BLE_SCAN_STATE_CHANGED_EVT:

        if (p_event_data->ble_scan_state_changed == BTM_BLE_SCAN_TYPE_HIGH_DUTY)
        {
            //printf("Scan State Change: BTM_BLE_SCAN_TYPE_HIGH_DUTY\n");
        }
        else if (p_event_data->ble_scan_state_changed == BTM_BLE_SCAN_TYPE_LOW_DUTY)
        {
            //printf("Scan State Change: BTM_BLE_SCAN_TYPE_LOW_DUTY\n");
        }
        else if (p_event_data->ble_scan_state_changed == BTM_BLE_SCAN_TYPE_NONE)
        {
            //printf("Scan stopped\n");
        }
        else
        {
            //printf("Invalid scan state\n");
        }
        break;

    default:
        //printf("Unhandled Bluetooth Management Event: 0x%x %s\n", event,
               //get_btm_event_name(event));
        break;
    }
    return result;
}

/*******************************************************************************
 * Function Name: ble_app_init
 ********************************************************************************
 * Summary:
 *   This function handles application level initialization tasks and is called
 *   from the BT management callback once the BLE stack enabled event
 *   (BTM_ENABLED_EVT) is triggered. This function is executed in the
 *   BTM_ENABLED_EVT management callback.
 *
 * Parameters:
 *   None
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void ble_app_init(void)
{
    cy_rslt_t cy_result = CY_RSLT_SUCCESS;
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;

    /* Initialize GPIO for button interrupt*/
    cy_result = cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT,
                                CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
    /* GPIO init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != cy_result)
    {
        //printf("Button GPIO init failed! \n");
        CY_ASSERT(0);
    }

    /* Configure GPIO interrupt. */
    cyhal_gpio_register_callback(CYBSP_USER_BTN, &button_cb_data);
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL,
                            BUTTON_INTERRUPT_PRIORITY, true);


    cyhal_gpio_init(CYBSP_USER_LED1 , CYHAL_GPIO_DIR_OUTPUT,
        CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_ON);

    /* Disable pairing for this application */
    wiced_bt_set_pairable_mode(WICED_FALSE, 0);

    /* Register with BT stack to receive GATT callback */
    status = wiced_bt_gatt_register(ble_app_gatt_event_callback);
    //printf("GATT event Handler registration status: %s \n",
           //get_bt_gatt_status_name(status));

    /* Initialize GATT Database */
    status = wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);
    //printf("GATT database initialization status: %s \n",
           //get_bt_gatt_status_name(status));

    //printf("Press User button to start scanning.....\n");
}

/********************************************************************************
 * Function Name: void ctss_scan_result_cback
 *********************************************************************************
 * Summary:
 *   This function is registered as a callback to handle the scan results.
 *   When the desired device is found, it will try to establish connection with
 *   that device.
 *
 * Parameters:
 *   wiced_bt_ble_scan_results_t *p_scan_result: Details of the new device found.
 *   uint8_t                     *p_adv_data      : Advertisement data.
 *
 * Return:
 *   None
 *
 *********************************************************************************/
void ctss_scan_result_cback(wiced_bt_ble_scan_results_t *p_scan_result,
                            uint8_t *p_adv_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    uint8_t length = 0u;
    uint8_t *adv_name;
    uint8_t client_device_name[9] = {
        'R',
        'C',
        ' ',
        'C',
        'n',
        't',
        'r',
        'l',
        '\0',
    };

    if (p_scan_result)
    {
        adv_name = wiced_bt_ble_check_advertising_data(p_adv_data,
                                                       BTM_BLE_ADVERT_TYPE_NAME_COMPLETE,
                                                       &length);
        if (NULL == adv_name)
        {
            return;
        }
        /* Check if the peer device's name is "RC Cntrl" */
        if (0 == memcmp(adv_name, client_device_name, strlen((const char *)client_device_name)))
        {
            //printf("\nFound the peer device! BD Addr: ");
            print_bd_address(p_scan_result->remote_bd_addr);

            /* Device found. Stop scan. */
            if ((result = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_NONE, WICED_TRUE,
                                            ctss_scan_result_cback)) != WICED_BT_SUCCESS)
            {
                //printf("\r\nscan off status %d\n", result);
            }
            else
            {
                //printf("Scan completed\n\n");
            }

            //printf("Initiating connection\n");
            /* Initiate the connection */
            if (wiced_bt_gatt_le_connect(p_scan_result->remote_bd_addr,
                                         p_scan_result->ble_addr_type,
                                         BLE_CONN_MODE_HIGH_DUTY,
                                         WICED_TRUE) != WICED_TRUE)
            {
                //printf("\rwiced_bt_gatt_connect failed\n");
            }
        }
        else
        {
            //printf("BD Addr: ");
            print_bd_address(p_scan_result->remote_bd_addr);
            return; // Skip - This is not the device we are looking for.
        }
    }
}

/********************************************************************************
 * Function Name: ble_app_gatt_event_callback
 *********************************************************************************
 * Summary:
 *   This function handles GATT events from the BT stack.
 *
 * Parameters:
 *   wiced_bt_gatt_evt_t event                   : BLE GATT event code of one byte length
 *   wiced_bt_gatt_event_data_t *p_event_data    : Pointer to BLE GATT event structures
 *
 * Return:
 *  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e
 *                          in wiced_bt_gatt.h
 *
 *********************************************************************************/
static wiced_bt_gatt_status_t ble_app_gatt_event_callback(wiced_bt_gatt_evt_t event,
                                                          wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;


    /* Call the appropriate callback function based on the GATT event type,
       and pass the relevant event parameters to the callback function */

    switch (event)
    {
    case GATT_CONNECTION_STATUS_EVT:
        gatt_status = ble_app_connect_handler(&p_event_data->connection_status);
        break;
    case GATT_DISCOVERY_RESULT_EVT:
        gatt_status = ble_app_discovery_result_handler(&p_event_data->discovery_result);
        break;
    case GATT_DISCOVERY_CPLT_EVT:
        gatt_status = ble_app_service_discovery_handler(&p_event_data->discovery_complete);
        break;
    case GATT_OPERATION_CPLT_EVT:
        switch (p_event_data->operation_complete.op)
        {
        case GATTC_OPTYPE_WRITE_WITH_RSP:
        if ((p_event_data->operation_complete.response_data.handle) == (cts_discovery_data.notify_cccd_handle) && (WICED_BT_SUCCESS == p_event_data->operation_complete.status))
        {
            if(notify_val)
            {
                //printf("Notifications enabled\n");
            }
            else
            {
                //printf("Notifications disabled\n");
            }
        }
        else if ((p_event_data->operation_complete.response_data.handle) == (cts_discovery_data.stop_car_val_handle) && (WICED_BT_SUCCESS == p_event_data->operation_complete.status))
        {
            //printf("Updated Sensor Stop with value: %d\n",
                   //sensor);
        }
        else {
            //printf("CCCD update failed. Error code: %d\n",
                //p_event_data->operation_complete.status);
        }
            break;
        case GATTC_OPTYPE_NOTIFICATION:
            ble_message_type_t message;

            if (STOP) break;

            message.left_motor = (int8_t) p_event_data->operation_complete.response_data.att_value.p_data[0];
            message.right_motor = (int8_t) p_event_data->operation_complete.response_data.att_value.p_data[1];

            // printf("XXLeft Motor: %03d\n", message.left_motor);
            // //printf("XXRight Motor: %03d\n", message.right_motor);


            xQueueSendToBack(q_dcmot, &message, portMAX_DELAY);
            break;

        }
    case GATT_APP_BUFFER_TRANSMITTED_EVT:
        // //printf("Buffer transmitted event received\n");
        // //printf("app data %02X\n", p_event_data->buffer_xmitted.p_app_data);
        break;

    default:
        //printf("unknown gatt event encountered :%d\n", event);
        gatt_status = WICED_BT_GATT_ERROR;
        break;
    }

    return gatt_status;
}

/*******************************************************************************
 * Function Name: ble_app_connect_handler
 *********************************************************************************
 * Summary:
 *   This callback function handles connection status changes.
 *
 * Parameters:
 *   wiced_bt_gatt_connection_status_t *p_conn_status  : Pointer to data that has
 *                                                       connection details
 *
 * Return:
 *  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e
 *                                in wiced_bt_gatt.h
 *
 *********************************************************************************/
static wiced_bt_gatt_status_t ble_app_connect_handler(wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    wiced_bt_gatt_discovery_param_t service_discovery_setup = {0};
    wiced_result_t result;

    if (NULL != p_conn_status)
    {
        if (p_conn_status->connected)
        {
            /* Device has connected */
            //printf("Connected : BD Addr: ");
            print_bd_address(p_conn_status->bd_addr);
            //printf("Connection ID '%d'\n", p_conn_status->conn_id);

            /* Store the connection ID */
            bt_connection_id = p_conn_status->conn_id;

            service_discovery_setup.s_handle = 0x01;
            service_discovery_setup.e_handle = 0xFFFF;
            service_discovery_setup.uuid.len = LEN_UUID_128;
            memcpy(service_discovery_setup.uuid.uu.uuid128, hello_notify_uuid, sizeof(hello_notify_uuid));

            status = wiced_bt_gatt_client_send_discover(bt_connection_id,
                                                        GATT_DISCOVER_SERVICES_ALL,
                                                        &service_discovery_setup);

            if (WICED_BT_GATT_SUCCESS != status)
            {
                //printf("GATT Discovery request failed. Error code: %d, "
                      // "Conn id: %d\n",
                       //status, bt_connection_id);
            }
            else
            {
                //printf("Service Discovery Started\n");
            }

            cyhal_gpio_write(CYBSP_USER_LED1 , CYBSP_LED_STATE_OFF);
        }
        else
        {
            /* Device has disconnected */
            //printf("\nDisconnected : BD Addr: ");
            //print_bd_address(p_conn_status->bd_addr);
            //printf("Connection ID '%d', Reason '%s'\n", p_conn_status->conn_id,
                   //get_bt_gatt_disconn_reason_name(p_conn_status->reason));

            /* Set the connection id to zero to indicate disconnected state */
            bt_connection_id = 0;

            /* Service discovery is performed upon reconnection, so reset the
             * status of service found flag */
            cts_discovery_data.cts_service_found = false;

            cyhal_gpio_write(CYBSP_USER_LED1 , CYBSP_LED_STATE_ON);

            /*restart the scan*/
            result = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY,
                                       WICED_TRUE,
                                       ctss_scan_result_cback);
            if (WICED_BT_PENDING != result)
            {
                //printf("Cannot restart scanning. Error: %d \n", result);
            }
            else
            {
                //printf("\r\nScanning.....\n");
            }
        }

        status = WICED_BT_GATT_ERROR;
    }

    return status;
}

static wiced_bt_gatt_status_t ble_app_discovery_result_handler(wiced_bt_gatt_discovery_result_t *discovery_result)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
    wiced_bt_gatt_discovery_type_t discovery_type;
    discovery_type = discovery_result->discovery_type;

    switch (discovery_type)
    {
    case GATT_DISCOVER_SERVICES_ALL:
        if (discovery_result->discovery_data.group_value.service_type.len == LEN_UUID_128 &&
            memcmp(discovery_result->discovery_data.group_value.service_type.uu.uuid128,
                   hello_notify_uuid, 16) == 0)
        {
            cts_discovery_data.cts_start_handle = discovery_result->discovery_data.group_value.s_handle;
            cts_discovery_data.cts_end_handle = discovery_result->discovery_data.group_value.e_handle;
            //printf("Hello Notify Service Found, Start Handle = %d, End Handle = %d \n",
                   //cts_discovery_data.cts_start_handle,
                   //cts_discovery_data.cts_end_handle);
        }
        else
        {
            //printf("Discovered service does not match Hello Notify UUID\n");
        }
        break;
    case GATT_DISCOVER_CHARACTERISTICS:
        if (discovery_result->discovery_data.characteristic_declaration.char_uuid.len == LEN_UUID_128)
        {
            if (memcmp(discovery_result->discovery_data.characteristic_declaration.char_uuid.uu.uuid128,
                       sensor_stop_uuid, 16) == 0)
            {
                cts_discovery_data.stop_car_handle = discovery_result->discovery_data.characteristic_declaration.handle;
                cts_discovery_data.stop_car_val_handle = discovery_result->discovery_data.characteristic_declaration.val_handle;
                //printf("Sensor Stop characteristic handle = %d, "
                       //"Sensor Stop characteristic value handle = %d\n",
                       //cts_discovery_data.stop_car_handle,
                       //cts_discovery_data.stop_car_val_handle);
            }
            else if (memcmp(discovery_result->discovery_data.characteristic_declaration.char_uuid.uu.uuid128,
                            notify_characteristic_uuid, 16) == 0)
            {
                cts_discovery_data.notify_char_handle = discovery_result->discovery_data.characteristic_declaration.handle;
                cts_discovery_data.notify_char_val_handle = discovery_result->discovery_data.characteristic_declaration.val_handle;
                //printf("Notify characteristic handle = %d, "
                      // "Notify characteristic value handle = %d\n",
                       //cts_discovery_data.notify_char_handle,
                       //cts_discovery_data.notify_char_val_handle);
            }
        }
        break;
    case GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS:
        if (UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION ==
            discovery_result->discovery_data.char_descr_info.type.uu.uuid16)
        {
            cts_discovery_data.notify_cccd_handle = discovery_result->discovery_data.char_descr_info.handle;
            cts_discovery_data.cts_service_found = true;
            //printf("Notify CCCD found, Handle = %d\n",
                   //cts_discovery_data.notify_cccd_handle);
        }
        break;
    default:
        //printf("Some other kind of discovery result handler found\n");
        break;
    }

    return gatt_status;
}

static wiced_bt_gatt_status_t ble_app_service_discovery_handler(wiced_bt_gatt_discovery_complete_t *discovery_complete)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
    wiced_bt_gatt_discovery_param_t char_discovery_setup = {0};
    wiced_bt_gatt_discovery_type_t discovery_type;
    discovery_type = discovery_complete->discovery_type;
    switch (discovery_type)
    {
    case GATT_DISCOVER_SERVICES_ALL:
    {
        char_discovery_setup.s_handle = cts_discovery_data.cts_start_handle;
        char_discovery_setup.e_handle = cts_discovery_data.cts_end_handle;

        gatt_status = wiced_bt_gatt_client_send_discover(bt_connection_id,
                                                         GATT_DISCOVER_CHARACTERISTICS,
                                                         &char_discovery_setup);
        // if (WICED_BT_GATT_SUCCESS != gatt_status)
        //     //printf("GATT characteristics discovery failed! Error code = %d\n", gatt_status);
        // else
        //     //printf("Started characteristic discovery\n");
        break;
    }
    case GATT_DISCOVER_CHARACTERISTICS:
    {
        char_discovery_setup.s_handle = cts_discovery_data.cts_start_handle;
        char_discovery_setup.e_handle = cts_discovery_data.cts_end_handle;
        char_discovery_setup.uuid.uu.uuid16 = 0x2902;
        gatt_status = wiced_bt_gatt_client_send_discover(bt_connection_id,
                                                         GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS,
                                                         &char_discovery_setup);

        // if (WICED_BT_GATT_SUCCESS != gatt_status)
        //     //printf("GATT CCCD discovery failed! Error code = %d\n", gatt_status);
        // else
        //     //printf("Started characteristic descriptor discovery\n");
        break;
    }
    case GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS:
    {
        //printf("finished discovering characteristic descriptors\n");
        // wiced_bt_gatt_write_hdr_t write_hdr = {0};
        // wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
        // uint8_t *notif_val = NULL;

        // notif_val = pvPortMalloc(sizeof(uint16_t));
        // if (notif_val)
        // {
        //     notif_val[0] = 0x01;
        //     notif_val[1] = 0;
        //     write_hdr.auth_req = GATT_AUTH_REQ_NONE;
        //     write_hdr.handle = cts_discovery_data.notify_cccd_handle;
        //     write_hdr.len = LEN_UUID_16;
        //     write_hdr.offset = 0;
        //     gatt_status = wiced_bt_gatt_client_send_write(bt_connection_id,
        //                                                   GATT_REQ_WRITE,
        //                                                   &write_hdr,
        //                                                   notif_val,
        //                                                   NULL);
        // }
        break;
    }
    default:
        //printf("Some other kind of discovery handler found, type: %d\n", discovery_type);
        break;
    }
    return gatt_status;
}

/*******************************************************************************
* Function Name: ble_app_write_notification_cccd()
********************************************************************************
* Summary:
*   Enable/Disable GATT notification from the server.
*
* Parameters:
*   bool notify : true - to enable notifications
*                 false - to disable notifications
*
* Return:
*   wiced_bt_gatt_status_t  : Status code from wiced_bt_gatt_status_e.
*
*******************************************************************************/
bool ble_app_write_notification_cccd(bool notify)
{
    if (bt_connection_id == 0) return false;

    notify_val = notify;
    wiced_bt_gatt_write_hdr_t write_hdr = {0};
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
    uint8_t *notif_val = NULL;

    notif_val = pvPortMalloc(sizeof(uint16_t));
    if (notif_val)
    {
        notif_val[0] = notify;
        notif_val[1] = 0;
        write_hdr.auth_req = GATT_AUTH_REQ_NONE;
        write_hdr.handle = cts_discovery_data.notify_cccd_handle;
        write_hdr.len = LEN_UUID_16;
        write_hdr.offset = 0;
        gatt_status = wiced_bt_gatt_client_send_write(bt_connection_id,
                                                        GATT_REQ_WRITE,
                                                        &write_hdr,
                                                        notif_val,
                                                        NULL);
    }
    return !(gatt_status == WICED_BT_GATT_SUCCESS);
}

/*******************************************************************************
* Function Name: ble_app_write_sensor_stop()
********************************************************************************
* Summary:
*   Write a 1 or 0 to stop value.
*
* Parameters:
*   bool notify : true - to enable notifications
*                 false - to disable notifications
*
* Return:
*   wiced_bt_gatt_status_t  : Status code from wiced_bt_gatt_status_e.
*
*******************************************************************************/
bool ble_app_write_sensor_stop(bool sensor_val) {
    if (bt_connection_id == 0) return false;
    
    sensor = sensor_val;
    wiced_bt_gatt_write_hdr_t write_hdr = {0};
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
    uint8_t *notif_val = NULL;

    notif_val = pvPortMalloc(sizeof(bool));
    
    notif_val[0] = sensor;
    write_hdr.auth_req = GATT_AUTH_REQ_NONE;
    write_hdr.handle = cts_discovery_data.stop_car_val_handle;
    write_hdr.len = 1;
    write_hdr.offset = 0;
    gatt_status = wiced_bt_gatt_client_send_write(bt_connection_id,
                                                    GATT_REQ_WRITE,
                                                    &write_hdr,
                                                    notif_val,
                                                    NULL);
                
    return !(gatt_status == WICED_BT_GATT_SUCCESS);
}

/*******************************************************************************
 * Function Name: button_interrupt_handler
 ********************************************************************************
 *
 * Summary:
 *   This interrupt handler enables or disables GATT notifications upon button
 *   press.
 *
 * Parameters:
 *   void *handler_arg:                     Not used
 *   cyhal_gpio_irq_event_t event:          Not used
 *
 * Return:
 *   None
 *
 *******************************************************************************/
void button_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(button_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
 * Function Name: button_task
 ********************************************************************************
 *
 * Summary:
 *   This task enables or disables notifications from the server upon button press.
 *
 * Parameters:
 *   void *pvParameters:                Not used
 *
 * Return:
 *   None
 *
 *******************************************************************************/
void button_task(void *pvParameters)
{
    wiced_result_t result;
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (bt_connection_id == 0) {
            initialize_connection();
        } else {
            ble_app_write_notification_cccd(true);
        }

    }
}

void initialize_connection() {
    wiced_result_t result;
    if (bt_connection_id == 0) {
        result = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE,
                                ctss_scan_result_cback);
        if (!((WICED_BT_PENDING == result) || (WICED_BT_BUSY == result)))
        {
            return;
        }
    } 
}

/*******************************************************************************
 * Function Name: app_get_attribute
 ********************************************************************************
 * Summary:
 * This function searches through the GATT DB to point to the attribute
 * corresponding to the given handle
 *
 * Parameters:
 *  uint16_t handle: Handle to search for in the GATT DB
 *
 * Return:
 *  gatt_db_lookup_table_t *: Pointer to the correct attribute in the GATT DB
 *
 *******************************************************************************/
gatt_db_lookup_table_t *app_get_attribute(uint16_t handle)
{
    /* Search for the given handle in the GATT DB and return the pointer to the
    correct attribute */
    uint8_t array_index = 0;

    for (array_index = 0; array_index < app_gatt_db_ext_attr_tbl_size; array_index++)
    {
        if (app_gatt_db_ext_attr_tbl[array_index].handle == handle)
        {
            return (&app_gatt_db_ext_attr_tbl[array_index]);
        }
    }
    return NULL;
}

/*******************************************************************************
 * Function Name: app_free_buffer
 *******************************************************************************
 * Summary:
 *  This function frees up the memory buffer
 *
 *
 * Parameters:
 *  uint8_t *p_data: Pointer to the buffer to be free
 *
 ******************************************************************************/
static void app_free_buffer(uint8_t *p_buf)
{
    vPortFree(p_buf);
}

/*******************************************************************************
 * Function Name: app_alloc_buffer
 *******************************************************************************
 * Summary:
 *  This function allocates a memory buffer.
 *
 *
 * Parameters:
 *  int len: Length to allocate
 *
 ******************************************************************************/
static void *app_alloc_buffer(int len)
{
    return pvPortMalloc(len);
}
/* [] END OF FILE */
