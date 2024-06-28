/*
 * Copyright 2024, Cypress Semiconductor Corporation (an Infineon company) or
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
*/

/**
* @file cy_ecm.c
* @brief Ethernet Connection Manager (ECM) provides set of APIs that are useful
* to establish and monitor Ethernet connection on Infineon platforms that support Ethernet connectivity.
* ECM APIs are easy to use compared to Ethernet PDL APIs, and provide additional features.
* See individual APIs for more details.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "cy_ecm.h"
#include "cy_ecm_error.h"
#include "cyabs_rtos.h"
#include "cy_worker_thread.h"
#include "cy_network_mw_core.h"
#include "cy_nw_helper.h"

#include "cycfg.h"
#include "eth_internal.h"
#include "cy_sysint.h"

#include "cy_log.h"

/******************************************************
 *                      Macros
 ******************************************************/
#ifdef ENABLE_ECM_LOGS
#define cy_ecm_log_msg cy_log_msg
#else
#define cy_ecm_log_msg(a,b,c,...)
#endif

#define DHCP_TIMEOUT_COUNT                          (6000) /* 6000 times */
#define CY_ECM_ETH_INTERFACE_MAX                    (2)
#define CY_POLL_ETHERNET_PHY_STATUS_TIME            (1000) /* Interval to poll the physical connection status in milliseconds*/
#define WAIT_CHECK_ETHERNET_PHY_STATUS              (100) /* Interval to check the Ethernet PHY status in milliseconds. The driver takes ~1 second to update the register. */
#define RETRY_WAIT_TIME_GET_IP_ADDR                 (10) /* Interval to check the IP address assigned for every 10ms */

#ifdef ENABLE_ECM_LOGS
    #define CY_ECM_EVENT_THREAD_STACK_SIZE          ((1024 * 1) + (1024 * 3)) /* Additional 3 KB of the stack is added for enabling  prints */
#else
    #define CY_ECM_EVENT_THREAD_STACK_SIZE          (1024 * 1)
#endif
#define CY_ECM_EVENT_THREAD_PRIORITY                (CY_RTOS_PRIORITY_NORMAL)

/** Maximum number of callbacks that can be registered with the ECM library */
#define CY_ECM_MAXIMUM_CALLBACKS_COUNT              (3)

/* MAC address*/
#define MAC_ADDR0                                (0x00U)
#define MAC_ADDR1                                (0x03U)
#define MAC_ADDR2                                (0x19U)
#define MAC_ADDR3                                (0x45U)
#define MAC_ADDR4                                (0x00U)
#define MAC_ADDR5                                (0x00U)

/******************************************************
 *             Structures
 ******************************************************/
/*
 * Ethernet Connection Manager handle
 */

typedef struct ecm_object
{
    cy_ecm_interface_t            eth_idx;
    ETH_Type                     *eth_base_type;
    cy_network_interface_context *iface_context;
    cy_ecm_phy_callbacks_t        eth_phy_cb;
    void*                         user_data;            /* Argument to be passed back to the user while invoking the callback */
    bool                          isobjinitialized;     /* Indicates that the ECM object is initialized      */
    cy_mutex_t                    obj_mutex;            /* Mutex to serialize object access in multi-threading mode  */
    bool                          network_up;
    uint8_t                       mac_address[CY_ECM_MAC_ADDR_LEN];
} cy_ecm_object_t;

/******************************************************
 *                 Static variables
 ******************************************************/
static bool                     is_ecm_initialized = false;
static cy_mutex_t               ecm_mutex;
static cy_thread_t              ecm_event_thread = NULL;
static cy_ecm_event_callback_t  ecm_event_handler[CY_ECM_MAXIMUM_CALLBACKS_COUNT];

static bool                     is_tcp_initialized = false;

/* Interface init status */
static bool                    is_ethernet_initiated[CY_ECM_ETH_INTERFACE_MAX] = {0};

/* PHY up/down status*/
static bool                    is_ethernet_link_up[CY_ECM_ETH_INTERFACE_MAX] = {0};

/* ECM event thread create status */
static uint8_t                 is_ecm_thread_created = 0;

/******************************************************
 *                 Static functions
 ******************************************************/
static void invoke_app_callbacks( cy_ecm_event_t event_type, cy_ecm_event_data_t* arg )
{
    int i = 0;

    for ( i = 0; i < CY_ECM_MAXIMUM_CALLBACKS_COUNT; i++ )
    {
        if( ecm_event_handler[i] != NULL )
        {
            ecm_event_handler[i]( event_type, arg );
        }
    }
}

static void ip_change_callback( cy_network_interface_context *iface_context, void *user_data )
{
    CY_UNUSED_PARAMETER( user_data );

    cy_ecm_event_data_t link_event_data;
    cy_rslt_t res = CY_RSLT_SUCCESS;
    cy_nw_ip_address_t ipv4_addr;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Notify application that ip has changed!\n" );
    memset( &link_event_data, 0, sizeof( cy_ecm_event_data_t ) );

    res = cy_network_get_ip_address( iface_context, &ipv4_addr );
    if( res == CY_RSLT_SUCCESS )
    {
        link_event_data.ip_addr.version = CY_ECM_IP_VER_V4;
        link_event_data.ip_addr.ip.v4   = ipv4_addr.ip.v4;
        invoke_app_callbacks( CY_ECM_EVENT_IP_CHANGED, &link_event_data );
    }
}

static void ecm_event_thread_func( cy_thread_arg_t arg )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    uint32_t linkstatus =0;
    cy_ecm_phy_callbacks_t *phy_cb = (cy_ecm_phy_callbacks_t *)arg;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    while( true )
    {
        if( is_ethernet_initiated[CY_ECM_INTERFACE_ETH0] == true )
        {
            // Read from the ETH0 register set
            result = phy_cb->phy_get_linkstatus((uint8_t)CY_ECM_INTERFACE_ETH0, &linkstatus);
            if(result == CY_RSLT_SUCCESS)
            {
                if(linkstatus == 1)
                {
                    if( is_ethernet_link_up[CY_ECM_INTERFACE_ETH0] == false )
                    {
                        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "get Link status : UP \n" );
                        is_ethernet_link_up[CY_ECM_INTERFACE_ETH0] = true;

                        /*Call the application callback function*/
                        invoke_app_callbacks( CY_ECM_EVENT_CONNECTED, NULL );
                    }
                }
                else
                {
                    if( is_ethernet_link_up[CY_ECM_INTERFACE_ETH0] == true )
                    {
                        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "get Link status : DOWN \n" );
                        is_ethernet_link_up[CY_ECM_INTERFACE_ETH0] = false;
                        /*Call the application callback function*/
                        invoke_app_callbacks( CY_ECM_EVENT_DISCONNECTED, NULL );
                    }
                }
            }
        }
        else if( is_ethernet_initiated[CY_ECM_INTERFACE_ETH1] == true )
        {
            //Read from the ETH1 register set
            result = phy_cb->phy_get_linkstatus((uint8_t)CY_ECM_INTERFACE_ETH1, &linkstatus);
            if(result == CY_RSLT_SUCCESS)
            {
                if(linkstatus == 1)
                {
                    if( is_ethernet_link_up[CY_ECM_INTERFACE_ETH1] == false )
                    {
                        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "get Link status : UP \n" );
                        is_ethernet_link_up[CY_ECM_INTERFACE_ETH1] = true;
                        /*Call the application callback function*/
                        invoke_app_callbacks( CY_ECM_EVENT_CONNECTED, NULL );
                    }
                }
                else
                {
                    if( is_ethernet_link_up[CY_ECM_INTERFACE_ETH1] == true )
                    {
                        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "get Link status : DOWN \n" );
                        is_ethernet_link_up[CY_ECM_INTERFACE_ETH1] = false;
                        /*Call the application callback function*/
                        invoke_app_callbacks( CY_ECM_EVENT_DISCONNECTED, NULL );
                    }
                }
            }
        }
        cy_rtos_delay_milliseconds( CY_POLL_ETHERNET_PHY_STATUS_TIME );
    }
}

cy_rslt_t cy_ecm_init( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nECM Library is already initialized \n" );
        return CY_RSLT_ECM_INIT_ERROR;
    }

    /** Initialize the network stack **/
    if( !is_tcp_initialized )
    {
        result = cy_network_init();
        if( result != CY_RSLT_SUCCESS )
        {
            return CY_RSLT_ECM_NW_INIT_ERROR;
        }
        is_tcp_initialized = true;
    }

    result = cy_rtos_init_mutex2( &ecm_mutex, false );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Creating new mutex failed with result = 0x%X\n", (unsigned long)result );
        is_tcp_initialized = false;
        result = CY_RSLT_ECM_MUTEX_ERROR;
        goto exit;
    }

     is_ecm_initialized = true;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;

exit:
    if( !is_tcp_initialized )
    {
        (void)cy_network_deinit();
    }
    return result;
}

cy_rslt_t cy_ecm_deinit( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Ethernet connection manager Library is not initialized (or) already de-initialized\n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }
    else
    {
        is_ecm_initialized = false;
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "De-initialize ethernet connection manager library\n" );

        is_tcp_initialized = false;

        (void)cy_network_deinit(); /* Fall through */
        cy_rtos_deinit_mutex( &ecm_mutex );
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Global Mutex Deinit..!\n" );
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_ethif_init( cy_ecm_interface_t eth_idx,
                             cy_ecm_phy_callbacks_t *phy_callbacks,
                             cy_ecm_t *ecm_handle )
{
    cy_rslt_t                     result = CY_RSLT_SUCCESS;
    cy_ecm_object_t              *ecm_obj       = NULL;
    bool                          obj_mutex_init_status = false;
    cy_ecm_phy_config_t           phy_interface_type;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || eth_idx >= CY_ECM_INTERFACE_INVALID )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid arguments passed \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( eth_idx == CY_ECM_INTERFACE_ETH1 )
    {
#if !(defined (eth_1_ENABLED) && (eth_1_ENABLED == 1u))
        return CY_RSLT_MODULE_ECM_BADARG;
#else
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n Ethernet interface 1 is enabled in configurator \n" );
#endif
    }
    else
    {
#if !(defined (eth_0_ENABLED) && (eth_0_ENABLED == 1u))
        return CY_RSLT_MODULE_ECM_BADARG;
#else
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n Ethernet interface 0 is enabled in configurator \n" );
#endif
    }

    if( phy_callbacks == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid arguments passed \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }
    else
    {
        if( (phy_callbacks->phy_init == NULL) || (phy_callbacks->phy_configure == NULL) ||
            (phy_callbacks->phy_discover == NULL) || (phy_callbacks->phy_enable_ext_reg == NULL) ||
            (phy_callbacks->phy_get_auto_neg_status == NULL) || (phy_callbacks->phy_get_link_partner_cap == NULL) ||
            (phy_callbacks->phy_get_linkspeed == NULL) || (phy_callbacks->phy_get_linkstatus == NULL) ||
            (phy_callbacks->phy_reset == NULL) )
        {
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid arguments passed \n" );
            return CY_RSLT_MODULE_ECM_BADARG;
        }
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    //Check whether the Ethernet interface is already initialized
    if( is_ethernet_initiated[eth_idx] == true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nECM interface is already initialized for eth_idx: [%d] \n",eth_idx );
        result = CY_RSLT_ECM_INIT_ERROR;
        goto exit;
    }

    ecm_obj = ( cy_ecm_object_t * )malloc( sizeof( cy_ecm_object_t ) );
    if( ecm_obj == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nMalloc for ECM handle failed..!\n" );
        result = CY_RSLT_ECM_ERROR_NOMEM;
        goto exit;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\necm_obj : %p..!\n", ecm_obj );
    memset( ecm_obj, 0x00, sizeof( cy_ecm_object_t ) );

    result = cy_rtos_init_mutex2( &(ecm_obj->obj_mutex), true );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Creating new mutex failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
        goto exit;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nobj_mutex created!\n" );

    eth_index_internal = eth_idx;

    obj_mutex_init_status = true;
    ecm_obj->isobjinitialized = true;
    ecm_obj->eth_idx = eth_idx;
    ecm_obj->eth_base_type = ETH_INTERFACE_TYPE;
    ecm_obj->network_up = false;
    ecm_obj->iface_context = NULL;

    /* Callbacks */
    ecm_obj->eth_phy_cb.phy_init = phy_callbacks->phy_init;
    ecm_obj->eth_phy_cb.phy_configure = phy_callbacks->phy_configure;
    ecm_obj->eth_phy_cb.phy_reset = phy_callbacks->phy_reset;
    ecm_obj->eth_phy_cb.phy_discover = phy_callbacks->phy_discover;
    ecm_obj->eth_phy_cb.phy_enable_ext_reg = phy_callbacks->phy_enable_ext_reg;
    ecm_obj->eth_phy_cb.phy_get_auto_neg_status = phy_callbacks->phy_get_auto_neg_status;
    ecm_obj->eth_phy_cb.phy_get_link_partner_cap = phy_callbacks->phy_get_link_partner_cap;
    ecm_obj->eth_phy_cb.phy_get_linkspeed = phy_callbacks->phy_get_linkspeed;
    ecm_obj->eth_phy_cb.phy_get_linkstatus = phy_callbacks->phy_get_linkstatus;

    *ecm_handle = (cy_ecm_t *)ecm_obj;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Assigning User MAC address...\n" );
    if(eth_idx == CY_ECM_INTERFACE_ETH1)
    {
#if (defined (eth_1_ENABLED) && (eth_1_ENABLED == 1u))
        ecm_obj->mac_address[0] = (uint8_t)eth_1_MAC_ADDR0;
        ecm_obj->mac_address[1] = (uint8_t)eth_1_MAC_ADDR1;
        ecm_obj->mac_address[2] = (uint8_t)eth_1_MAC_ADDR2;
        ecm_obj->mac_address[3] = (uint8_t)eth_1_MAC_ADDR3;
        ecm_obj->mac_address[4] = (uint8_t)eth_1_MAC_ADDR4;
        ecm_obj->mac_address[5] = (uint8_t)eth_1_MAC_ADDR5;
#endif
    }

    if(eth_idx == CY_ECM_INTERFACE_ETH0)
    {
#if (defined (eth_0_ENABLED) && (eth_0_ENABLED == 1u))
        ecm_obj->mac_address[0] = (uint8_t)eth_0_MAC_ADDR0;
        ecm_obj->mac_address[1] = (uint8_t)eth_0_MAC_ADDR1;
        ecm_obj->mac_address[2] = (uint8_t)eth_0_MAC_ADDR2;
        ecm_obj->mac_address[3] = (uint8_t)eth_0_MAC_ADDR3;
        ecm_obj->mac_address[4] = (uint8_t)eth_0_MAC_ADDR4;
        ecm_obj->mac_address[5] = (uint8_t)eth_0_MAC_ADDR5;
#endif
    }

    if(eth_idx == CY_ECM_INTERFACE_ETH0)
    {
#if (defined (eth_0_ENABLED) && (eth_0_ENABLED == 1u))
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Initializing ETH0 interface PHY properties...\n" );
        phy_interface_type.interface_speed_type = (cy_ecm_speed_type_t)eth_0_PHY_INTERFACE;
        phy_interface_type.phy_speed = (cy_ecm_phy_speed_t)eth_0_PHY_SPEED;
        phy_interface_type.mode = (cy_ecm_duplex_t)eth_0_PHY_MODE;
#endif
    }
    else
    {
#if (defined (eth_1_ENABLED) && (eth_1_ENABLED == 1u))
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Initializing ETH1 interface PHY properties...\n" );
        phy_interface_type.interface_speed_type = (cy_ecm_speed_type_t)eth_1_PHY_INTERFACE;
        phy_interface_type.phy_speed = (cy_ecm_phy_speed_t)eth_1_PHY_SPEED;
        phy_interface_type.mode = (cy_ecm_duplex_t)eth_1_PHY_MODE;
#endif
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "PHY interface type  : %d \n", (int)phy_interface_type.interface_speed_type );
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "PHY interface speed : %d \n", (int)phy_interface_type.phy_speed );
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "PHY interface mode  : %d \n", (int)phy_interface_type.mode );

    /* Prevent system to enter into deep sleep during ethernet initialization */
    cyhal_syspm_lock_deepsleep();

    result = cy_eth_driver_initialization( ecm_obj->eth_idx, ecm_obj->eth_base_type, &phy_interface_type, &(ecm_obj->eth_phy_cb) );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "ECM driver initialization failed with result = 0x%X\n", (unsigned long)result );
        /* Unlock to enter into deep sleep */
        cyhal_syspm_unlock_deepsleep();
        result = CY_RSLT_ECM_ERROR;
        goto exit;
    }

    /* Enable/Disable Promiscuous Mode */
#if (defined (eth_0_ENABLED) && (eth_0_ENABLED == 1u))
#if (eth_0_PROMISCUOUS_MODE == true)
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Setting Promiscuous Mode for ETH0 interface...\n" );
    Cy_ETHIF_SetPromiscuousMode( ecm_obj->eth_base_type, true );
#endif
#if (eth_0_ACCEPT_BROADCASR_FRAMES == false)
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Setting No BroadCast for ETH0 interface...\n" );
    Cy_ETHIF_SetNoBroadCast( ecm_obj->eth_base_type, true );
#endif
#endif

#if (defined (eth_1_ENABLED) && (eth_1_ENABLED == 1u))
#if (eth_1_PROMISCUOUS_MODE == true)
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Setting Promiscuous Mode for ETH1 interface...\n" );
    Cy_ETHIF_SetPromiscuousMode( ecm_obj->eth_base_type, true );
#endif
#if (eth_1_ACCEPT_BROADCASR_FRAMES == false)
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Setting No BroadCast for ETH1 interface...\n" );
    Cy_ETHIF_SetNoBroadCast( ecm_obj->eth_base_type, true );
#endif
#endif

    is_ethernet_initiated[ecm_obj->eth_idx] = true;

    /* Unlock to enter into deep sleep */
    cyhal_syspm_unlock_deepsleep();

    if(is_ecm_thread_created == 0)
    {
        /* Create the thread to handle connect/disconnect events */
         result = cy_rtos_create_thread( &ecm_event_thread, ecm_event_thread_func, "ECMEventThread", NULL,
                                         CY_ECM_EVENT_THREAD_STACK_SIZE, CY_ECM_EVENT_THREAD_PRIORITY, (cy_thread_arg_t)(&(ecm_obj->eth_phy_cb)) );
         if( result != CY_RSLT_SUCCESS )
         {
             cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\ncy_rtos_create_thread failed with Error : [0x%X]\n", (unsigned int)result );
             result = CY_RSLT_ECM_ERROR;
             goto exit;
         }
    }

    is_ecm_thread_created++;

    result = cy_rtos_set_mutex( &ecm_mutex );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;

exit:
    if( ecm_obj != NULL )
    {
        if( obj_mutex_init_status == true )
        {
            cy_rtos_deinit_mutex( &( ecm_obj->obj_mutex ) );
        }

        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n Free ecm_obj : %p..!\n", ecm_obj );
        free( ecm_obj );
    }

    if( cy_rtos_set_mutex( &ecm_mutex ) )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_ethif_deinit( cy_ecm_t *ecm_handle )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || *ecm_handle == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    ecm_obj = (cy_ecm_object_t *)*ecm_handle;

    if( ecm_obj->isobjinitialized != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    is_ecm_thread_created--;

    /* Terminate the connect/disconnect event thread */
    if( is_ecm_thread_created == 0 )
    {
        if( ecm_event_thread != NULL )
        {
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nTerminating ECM event thread %p..!\n", ecm_event_thread );
            result = cy_rtos_terminate_thread( &ecm_event_thread );
            if( result != CY_RSLT_SUCCESS )
            {
                cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nTerminate ECM event thread failed with Error : [0x%X] ", (unsigned int)result );
                /* Fall-through. It's intentional. */
            }

            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nJoining ECM event thread %p..!\n", ecm_event_thread );
            result = cy_rtos_join_thread( &ecm_event_thread );
            if( result != CY_RSLT_SUCCESS )
            {
                cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nJoin ECM event thread failed with Error : [0x%X] ", (unsigned int)result );
                /* Fall-through. It's intentional. */
            }
            ecm_event_thread = NULL;
        }
    }

    if( cy_rtos_deinit_mutex( &( ecm_obj->obj_mutex ) ) != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Mutex deinit failed with result = 0x%X\n", (unsigned long)result );
        /* Intentional fallthrough */
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Deinit object mutex : %p..!\n", ecm_obj->obj_mutex );

    deregister_cb(ecm_obj->eth_base_type);

    is_ethernet_initiated[ecm_obj->eth_idx] = false;
    ecm_obj->iface_context = NULL;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "ecm_obj : %p..!\n", ecm_obj );

    free( ecm_obj );

    *ecm_handle = NULL;

    result = cy_rtos_set_mutex( &ecm_mutex );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_set_promiscuous_mode(cy_ecm_t ecm_handle, bool is_promiscuous_mode)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    if( ecm_obj->isobjinitialized != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not initialized \n" );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    /* Check if ethernet is up */
    if( is_ethernet_initiated[ecm_obj->eth_idx] == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nECM is not initiated for eth_idx: [%d] \n",ecm_obj->eth_idx );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "ecm_obj->eth_base_type:[%p]\n",ecm_obj->eth_base_type );

    /* Disable/Enable copy all frames. If 1 enables copy all frames mode, if 0 then this is disabled */
    Cy_ETHIF_SetPromiscuousMode( ecm_obj->eth_base_type, is_promiscuous_mode );

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed\n" );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_broadcast_disable(cy_ecm_t ecm_handle, bool is_broadcast_disable)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    if( ecm_obj->isobjinitialized != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not initialized \n" );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    //Check whether Ethernet is up
    if( is_ethernet_initiated[ecm_obj->eth_idx] == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nECM is not initiated for eth_idx: [%d] \n",ecm_obj->eth_idx );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "ecm_obj->eth_base_type:[%p]\n",ecm_obj->eth_base_type );

    /* Reject/Accept Broad cast frames. If 0 broadcasts are accepted, if 1 they are rejected */
    Cy_ETHIF_SetNoBroadCast( ecm_obj->eth_base_type, is_broadcast_disable );

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed\n" );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_set_filter_address(cy_ecm_t ecm_handle, cy_ecm_filter_address_t *filter_address, uint8_t filter_address_count)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    cy_en_ethif_status_t ethStatus;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || filter_address == NULL || filter_address_count == 0
            || ( filter_address_count > CY_ECM_MAX_FILTER_ADDRESS ) )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    if( ecm_obj->isobjinitialized != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not initialized \n" );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    /* Check if ethernet is up */
    if( is_ethernet_initiated[ecm_obj->eth_idx] == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nECM is not initiated for eth_idx: [%d] \n",ecm_obj->eth_idx );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "ecm_obj->eth_base_type:[%p]\n",ecm_obj->eth_base_type );

    for( int i = 0; i < filter_address_count; i++ )
    {
        cy_stc_ethif_filter_config_t filter_config;
        filter_config.typeFilter = ( cy_en_ethif_filter_type_t )filter_address[i].filter_type;
        memcpy( (void*)&(filter_config.filterAddr.byte[0]), (void*)&(filter_address[i].filter_addr[0]), CY_ECM_MAC_ADDR_LEN );
        filter_config.ignoreBytes = filter_address->ignoreBytes;

        /* Filter settings */
        ethStatus = Cy_ETHIF_SetFilterAddress( ecm_obj->eth_base_type, (cy_en_ethif_filter_num_t)(i+1), &filter_config );
        if( CY_ETHIF_SUCCESS != ethStatus )
        {
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n setting filter address failed \n" );
            result = CY_RSLT_ECM_ERROR;
            goto exit;
        }
    }

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed\n" );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_connect( cy_ecm_t ecm_handle, cy_ecm_ip_setting_t *ecm_static_ip_addr, cy_ecm_ip_address_t *ip_addr )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    cy_network_static_ip_addr_t nw_static_ipaddr, *static_ipaddr = NULL;
    cy_nw_ip_address_t ipv4_addr;
    uint32_t total_wait_time = 0, linkstatus = 0;
#ifdef ENABLE_ECM_LOGS
    char ip_str[15];
#endif

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid handle Arguments \n" );
        return  CY_RSLT_MODULE_ECM_BADARG;
     }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    if( ecm_obj->isobjinitialized != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not initialized \n" );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    //Check whether the connection is already established
    if( ecm_obj->network_up == true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library already connected \n" );
        result = CY_RSLT_MODULE_ECM_ALREADY_CONNECTED;
        goto exit;
    }

    if( ecm_static_ip_addr != NULL )
    {
        if( ecm_static_ip_addr->gateway.version == CY_ECM_IP_VER_V4 )
        {
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n Static IP address not NULL\n" );
            nw_static_ipaddr.gateway.ip.v4 = ecm_static_ip_addr->gateway.ip.v4;
            nw_static_ipaddr.addr.ip.v4    = ecm_static_ip_addr->ip_address.ip.v4;
            nw_static_ipaddr.netmask.ip.v4 = ecm_static_ip_addr->netmask.ip.v4;
            static_ipaddr = &nw_static_ipaddr;
        }
        else
        {
            // TO DO : Copy the IPv6 address
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Static IP address not supported\n" );
            result = CY_RSLT_ECM_STATIC_IP_NOT_SUPPORTED;
            goto exit;
        }
    }

    //Add the Ethernet interface
    result = cy_network_add_nw_interface( CY_NETWORK_ETH_INTERFACE, ecm_obj->eth_idx, (void *)ecm_obj->eth_base_type, ecm_obj->mac_address, static_ipaddr, &ecm_obj->iface_context );
    if(result != CY_RSLT_SUCCESS)
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "failed to add the network interface \n" );
        result = CY_RSLT_ECM_INTERFACE_ERROR;
        goto exit;
    }

    /* Register to IP address change callback from the lwIP stack. All other internal callbacks are in ECM */
    cy_network_register_ip_change_cb( ecm_obj->iface_context, ip_change_callback, NULL );

    //Check whether the Ethernet link status is up; call interface network_up()
    if( is_ethernet_link_up[ecm_obj->eth_idx] != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Waiting for Link up... \n" );
        while( total_wait_time < MAX_WAIT_ETHERNET_PHY_STATUS )
        {
            result = ecm_obj->eth_phy_cb.phy_get_linkstatus((uint8_t)ecm_obj->eth_idx, &linkstatus);
            if(result == CY_RSLT_SUCCESS)
            {
                if(linkstatus == 1)
                {
                    break;
                }
            }
            cy_rtos_delay_milliseconds(WAIT_CHECK_ETHERNET_PHY_STATUS);
            total_wait_time += WAIT_CHECK_ETHERNET_PHY_STATUS;
            continue;
        }

        if( total_wait_time >= MAX_WAIT_ETHERNET_PHY_STATUS )
        {
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Ethernet link is not up :[%d] \n", total_wait_time );
            result = CY_RSLT_ECM_ERROR;
            goto exit;
        }

        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Ethernet Link is up \n" );
        is_ethernet_link_up[ecm_obj->eth_idx] = true;
    }

    result = cy_network_ip_up( ecm_obj->iface_context );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "failed to bring up the network stack :[%d] \n",result );
        if( (cy_network_remove_nw_interface( ecm_obj->iface_context )) != CY_RSLT_SUCCESS )
        {
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "failed to remove the network interface \n" );
        }
        if(result == CY_RSLT_NETWORK_ERROR_STARTING_DHCP)
        {
            result = CY_RSLT_MODULE_ECM_ERROR_STARTING_DHCP;
        }
        else if(result == CY_RSLT_NETWORK_DHCP_WAIT_TIMEOUT)
        {
            result = CY_RSLT_ECM_DHCP_TIMEOUT;
        }
        goto exit;
    }

    total_wait_time = 0;
    /** Wait in a busy loop until DHCP starts and IP address is assigned **/
    while ( true )
    {
        result = cy_network_get_ip_address( ecm_obj->iface_context, &ipv4_addr );
        if( result == CY_RSLT_SUCCESS )
        {
#ifdef ENABLE_ECM_LOGS
            cy_nw_ntoa( &ipv4_addr, ip_str );
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "IPV4 Address %s assigned \n", ip_str );
#endif
            if( ip_addr != NULL )
            {
                ip_addr->version = CY_ECM_IP_VER_V4;
                ip_addr->ip.v4 = ipv4_addr.ip.v4;
            }
            break;
        }
        /* TO DO : get ipv6 address */
        /* Delay of 10 ms */
        cy_rtos_delay_milliseconds( RETRY_WAIT_TIME_GET_IP_ADDR );
        /* Increment the count for every 10 ms */
        total_wait_time += RETRY_WAIT_TIME_GET_IP_ADDR;
        /* Return DHCP timeout error when it exceeds 6000 * 10 ms = 60 seconds */
    }
    if( total_wait_time > DHCP_TIMEOUT_COUNT )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "DHCP Timeout \n" );

        // Bring network down and remove network interface as DHCP failed
        cy_network_ip_down(ecm_obj->iface_context);
        cy_network_remove_nw_interface(ecm_obj->iface_context);

        /* Return DHCP timeout error when DHCP discovery failed and disconnect is done properly */
        result = CY_RSLT_ECM_DHCP_TIMEOUT;
        goto exit;
    }

    ecm_obj->network_up = true;

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed\n" );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_disconnect( cy_ecm_t ecm_handle )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    if( ecm_obj->isobjinitialized != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not initialized \n" );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    /* Check connection established for disconnection */
    if( ecm_obj->network_up == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not connected \n" );
        result = CY_RSLT_MODULE_ECM_NOT_CONNECTED;
        goto exit;
    }

    /* Register to ip change callback from LwIP, all other internal callbacks are in ECM */
    cy_network_register_ip_change_cb( ecm_obj->iface_context, NULL, NULL );

    //Bring down the Ethernet interface
    cy_network_ip_down( ecm_obj->iface_context );
    cy_network_remove_nw_interface( ecm_obj->iface_context );

    ecm_obj->network_up = false;

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed\n" );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_register_event_callback( cy_ecm_t ecm_handle, cy_ecm_event_callback_t event_callback )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || event_callback == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    if( ecm_obj->isobjinitialized != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not initialized \n" );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    for ( int i = 0; i < CY_ECM_MAXIMUM_CALLBACKS_COUNT; i++ )
    {
        if( ecm_event_handler[i] == NULL )
        {
            ecm_event_handler[i] = event_callback;
            break;
        }
    }

exit:
    result = cy_rtos_set_mutex( &ecm_mutex );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;

}

cy_rslt_t cy_ecm_deregister_event_callback( cy_ecm_t ecm_handle, cy_ecm_event_callback_t event_callback )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || event_callback == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    if( ecm_obj->isobjinitialized != true )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n ECM library not initialized \n" );
        result = CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
        goto exit;
    }

    for ( int i = 0; i < CY_ECM_MAXIMUM_CALLBACKS_COUNT; i++ )
    {
        if( ecm_event_handler[i] == event_callback )
        {
            memset( &ecm_event_handler[i], 0, sizeof( cy_ecm_event_callback_t ) );
            result = CY_RSLT_SUCCESS;;
            goto exit;
        }
    }

exit:
    result = cy_rtos_set_mutex( &ecm_mutex );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_get_link_status( cy_ecm_t ecm_handle, bool *status )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    uint32_t total_wait_time = 0, linkstatus = 0;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || status == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n before reading link status = %d \n",*status );

    while( total_wait_time < (uint32_t )MAX_WAIT_ETHERNET_PHY_STATUS )
    {
        result = ecm_obj->eth_phy_cb.phy_get_linkstatus((uint8_t)ecm_obj->eth_idx, &linkstatus);
        if(result == CY_RSLT_SUCCESS)
        {
            if(linkstatus == 1)
            {
                *status = true;
                break;
            }
        }
        cy_rtos_delay_milliseconds( WAIT_CHECK_ETHERNET_PHY_STATUS );
        total_wait_time += WAIT_CHECK_ETHERNET_PHY_STATUS;
        continue;
    }

    if( total_wait_time >= MAX_WAIT_ETHERNET_PHY_STATUS )
    {
        result = CY_RSLT_SUCCESS;
        *status = false;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n After reading link status = %d \n",*status );

    if( cy_rtos_set_mutex( &ecm_mutex ) )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed \n" );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_get_ip_address( cy_ecm_t ecm_handle, cy_ecm_ip_address_t *ip_addr )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    cy_nw_ip_address_t ipv4_addr;
#ifdef ENABLE_ECM_LOGS
    char ip_str[15];
#endif

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || ip_addr == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    /* Check whether the network is up */
    if( ecm_obj->network_up == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Network is not up, call cy_ecm_connect API to bring network up \r\n" );
        /** Network is not up and therefore, mem setting the IP address to zero**/
        memset( ip_addr, 0, sizeof( cy_ecm_ip_address_t ) );
        result = CY_RSLT_MODULE_ECM_NOT_CONNECTED;
        goto exit;
    }

    result = cy_network_get_ip_address( ecm_obj->iface_context, &ipv4_addr );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to get the IP address\n" );
        result = CY_RSLT_ECM_IP_ADDR_ERROR;
        goto exit;
    }
    ip_addr->version = CY_ECM_IP_VER_V4;
    ip_addr->ip.v4 = ipv4_addr.ip.v4;
#ifdef ENABLE_ECM_LOGS
    cy_nw_ntoa( &ipv4_addr, ip_str );
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "IP Address %s assigned \n", ip_str );
#endif

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_get_ipv6_address( cy_ecm_t ecm_handle, cy_ecm_ipv6_type_t ipv6_addr_type, cy_ecm_ip_address_t *ip_addr )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    cy_nw_ip_address_t ipv6_addr;
    cy_network_ipv6_type_t type;
#ifdef ENABLE_ECM_LOGS
    char ip_str[39];
#endif

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

     if( ecm_handle == NULL || ip_addr == NULL )
     {
         cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
         return CY_RSLT_MODULE_ECM_BADARG;
     }

     if( !is_ecm_initialized )
     {
         cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
         return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
     }

     if( ipv6_addr_type != CY_ECM_IPV6_LINK_LOCAL )
     {
         /*TODO : Need to add support for Global IPV6 address */
         return CY_RSLT_ECM_IPV6_GLOBAL_ADDRESS_NOT_SUPPORTED;
     }
     type = CY_NETWORK_IPV6_LINK_LOCAL;

     cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    /* Check whether the network is up */
    if( ecm_obj->network_up == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Network is not up, call cy_ecm_connect API to bring network up \r\n" );
        /** Network is not up and therefore, mem setting the IP address to zero**/
        memset( ip_addr, 0, sizeof( cy_ecm_ip_address_t ) );
        result = CY_RSLT_MODULE_ECM_NOT_CONNECTED;
        goto exit;
    }

    result = cy_network_get_ipv6_address( ecm_obj->iface_context, type, &ipv6_addr );
    if( result == CY_RSLT_SUCCESS )
    {
        ip_addr->version = CY_ECM_IP_VER_V6;
        ip_addr->ip.v6[0] = ipv6_addr.ip.v6[0];
        ip_addr->ip.v6[1] = ipv6_addr.ip.v6[1];
        ip_addr->ip.v6[2] = ipv6_addr.ip.v6[2];
        ip_addr->ip.v6[3] = ipv6_addr.ip.v6[3];

#ifdef ENABLE_ECM_LOGS
        cy_nw_ntoa_ipv6( &ipv6_addr, ip_str );
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "IPV6 Address %s assigned \n", ip_str );
#endif
    }
    else
    {
        memset( ip_addr, 0, sizeof( cy_ecm_ip_address_t ) );
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "IPV6 network not ready \n" );
        result = CY_RSLT_ECM_IPV6_INTERFACE_NOT_READY;
        goto exit;
    }

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_get_gateway_address( cy_ecm_t ecm_handle, cy_ecm_ip_address_t *gateway_addr )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    cy_nw_ip_address_t ipv4_addr;
#ifdef ENABLE_ECM_LOGS
    char ip_str[15];
#endif

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || gateway_addr == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    /* Check whether the network is up */
    if( ecm_obj->network_up == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Network is not up, call connect API to bring network up \r\n" );
        /** Network is not up and therefore, mem setting the IP address to zero**/
        memset( gateway_addr, 0, sizeof( cy_ecm_ip_address_t ) );
        result = CY_RSLT_MODULE_ECM_NOT_CONNECTED;
        goto exit;
    }

    result = cy_network_get_gateway_ip_address( ecm_obj->iface_context, &ipv4_addr );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to get the gateway address\n" );
        result = CY_RSLT_ECM_GATEWAY_ADDR_ERROR;
        goto exit;
    }
    gateway_addr->version = CY_ECM_IP_VER_V4;
    gateway_addr->ip.v4 = ipv4_addr.ip.v4;
#ifdef ENABLE_ECM_LOGS
    cy_nw_ntoa( &ipv4_addr, ip_str );
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Gateway IP Address %s assigned \n", ip_str );
#endif

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to release the mutex \n" );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_get_netmask_address( cy_ecm_t ecm_handle, cy_ecm_ip_address_t *net_mask_addr )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    cy_nw_ip_address_t ipv4_addr;
#ifdef ENABLE_ECM_LOGS
    char ip_str[15];
#endif

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || net_mask_addr == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    /* Check whether the network is up */
    if( ecm_obj->network_up == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Network is not up, call connect API to bring network up \r\n" );
        /** Network is not up and therefore, mem setting the IP address to zero**/
        memset( net_mask_addr, 0, sizeof( cy_ecm_ip_address_t ) );
        result = CY_RSLT_MODULE_ECM_NOT_CONNECTED;
        goto exit;
    }

    result = cy_network_get_netmask_address( ecm_obj->iface_context, &ipv4_addr );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to get the gateway address\n" );
        result = CY_RSLT_ECM_GATEWAY_ADDR_ERROR;
        goto exit;
    }
    net_mask_addr->version = CY_ECM_IP_VER_V4;
    net_mask_addr->ip.v4 = ipv4_addr.ip.v4;
#ifdef ENABLE_ECM_LOGS
    cy_nw_ntoa( &ipv4_addr, ip_str );
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Gateway IP Address %s assigned \n", ip_str );
#endif

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to release the mutex \n" );
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_get_mac_address( cy_ecm_t ecm_handle, cy_ecm_mac_t *mac_addr )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    cy_nw_ip_mac_t nw_mac_addr;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || mac_addr == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_MUTEX_ERROR;
        goto exit;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    /* check for network up */
    if( ecm_obj->network_up == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Network is not up, call connect API to bring network up \r\n" );
        /** Network is not up and hence mem setting the gateway MAC address to zero**/
        memset( mac_addr , 0, sizeof( cy_ecm_mac_t ) );
        result = CY_RSLT_MODULE_ECM_NOT_CONNECTED;
        goto exit;
    }

    result = cy_network_get_gateway_mac_address( ecm_obj->iface_context, &nw_mac_addr );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to get the gateway mac address address\n" );
        result = CY_RSLT_ECM_GATEWAY_ADDR_ERROR;
        goto exit;
    }

    (*mac_addr)[0] = nw_mac_addr.mac[0];
    (*mac_addr)[1] = nw_mac_addr.mac[1];
    (*mac_addr)[2] = nw_mac_addr.mac[2];
    (*mac_addr)[3] = nw_mac_addr.mac[3];
    (*mac_addr)[4] = nw_mac_addr.mac[4];
    (*mac_addr)[5] = nw_mac_addr.mac[5];

exit: 
    if( cy_rtos_set_mutex( &ecm_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to release the mutex \n");
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_ping( cy_ecm_t ecm_handle, cy_ecm_ip_address_t *address, uint32_t timeout_ms, uint32_t* elapsed_time_ms )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || address == NULL || elapsed_time_ms == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    /* check for network up */
    if( ecm_obj->network_up == false )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Network is not up, call connect API to bring network up \r\n" );
        result = CY_RSLT_MODULE_ECM_NOT_CONNECTED;
        goto exit;
    }

    result = cy_network_ping( (void *)ecm_obj->iface_context, (cy_nw_ip_address_t *) address, timeout_ms, elapsed_time_ms );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Ping failure with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_ECM_PING_FAILURE;
        goto exit;
    }

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to release the mutex \n");
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_ecm_get_link_speed( cy_ecm_t ecm_handle, cy_ecm_duplex_t *duplex, cy_ecm_phy_speed_t *speed )
{
    cy_rslt_t        result = CY_RSLT_SUCCESS;
    cy_ecm_object_t *ecm_obj;
    uint32_t         total_wait_time = 0;
    uint32_t         mode = 0, phy_speed = 0, link_status = 0;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( ecm_handle == NULL || duplex == NULL || speed == NULL )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_MODULE_ECM_BADARG;
    }

    if( !is_ecm_initialized )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Ethernet connection manager library not initialized \n" );
        return CY_RSLT_MODULE_ECM_NOT_INITIALIZED;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", ecm_mutex );
    result = cy_rtos_get_mutex( &ecm_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_ECM_MUTEX_ERROR;
    }

    ecm_obj = (cy_ecm_object_t *)ecm_handle;

    /* Check whether the link is up*/
    while( total_wait_time < (uint32_t )MAX_WAIT_ETHERNET_PHY_STATUS )
    {
        result = ecm_obj->eth_phy_cb.phy_get_linkstatus((uint8_t)ecm_obj->eth_idx, &link_status);
        if(result == CY_RSLT_SUCCESS)
        {
            if(link_status == true)
            {
                result = ecm_obj->eth_phy_cb.phy_get_linkspeed((uint8_t)ecm_obj->eth_idx, &mode, &phy_speed);
                if(result == CY_RSLT_SUCCESS)
                {
                    *duplex = (cy_ecm_duplex_t)mode;
                    *speed = (cy_ecm_phy_speed_t)phy_speed;
                    goto exit;
                }
            }
        }

        cy_rtos_delay_milliseconds( WAIT_CHECK_ETHERNET_PHY_STATUS );
        total_wait_time += WAIT_CHECK_ETHERNET_PHY_STATUS;
    }

    result = CY_RSLT_ECM_ERROR;

exit:
    if( cy_rtos_set_mutex( &ecm_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_ecm_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "Failed to release the mutex \n");
        result = CY_RSLT_ECM_MUTEX_ERROR;
    }

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", ecm_mutex );

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}
