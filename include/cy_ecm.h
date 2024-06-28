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
#pragma once

#include "cy_ecm_error.h"
#include "cy_ephy.h"

/**
* @file cy_ecm.h
* @brief Ethernet Connection Manager (ECM) is a library that helps application developers manage their Ethernet connectivity.
* The library provides a set of APIs that can be used to establish and monitor ethernet ports on Infineon platforms that support Ethernet connectivity.
* The library APIs are thread-safe. The library monitors the Ethernet connection and notifies the connection state change through an event notification mechanism.
* See individual APIs for more details.
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup group_ecm_macros
 * \{
 */

/******************************************************
 *                    Constants
 ******************************************************/
#define CY_ECM_MAX_FILTER_ADDRESS                  (4U)         /**< Maximum number of addresses to be filtered by MAC */
#define CY_ECM_MAC_ADDR_LEN                        (6U)         /**< MAC address length                              */

/** \} group_ecm_macros */

/**
 * \addtogroup group_ecm_enums
 * \{
 */

/******************************************************
 *            Enumerations
 ******************************************************/

/**
 * Enumeration of ECM interfaces types
 */
typedef enum
{
    CY_ECM_INTERFACE_ETH0 = 0,    /**< Interface for Ethernet port 0 */
    CY_ECM_INTERFACE_ETH1,        /**< Interface for Ethernet port 1 */
    CY_ECM_INTERFACE_INVALID      /**< Invalid interface */
} cy_ecm_interface_t;

/**
 * IP address version
 */
typedef enum
{
    CY_ECM_IP_VER_V4 = 4,      /**< IPv4 version */
    CY_ECM_IP_VER_V6 = 6       /**< IPv6 version */
} cy_ecm_ip_version_t;

/**
 * IPv6 address types
 */
typedef enum
{
    CY_ECM_IPV6_LINK_LOCAL = 0,  /**< IPv6 link-local address  */
    CY_ECM_IPV6_GLOBAL           /**< IPv6 global address  */
} cy_ecm_ipv6_type_t;

/** PHY duplex mode */
typedef enum
{
    CY_ECM_DUPLEX_HALF,    /**< Half duplex */
    CY_ECM_DUPLEX_FULL,    /**< Full duplex */
    CY_ECM_DUPLEX_AUTO     /**< Both half/full duplex */
} cy_ecm_duplex_t;

/** PHY speed */
typedef enum
{
    CY_ECM_PHY_SPEED_10M,   /**< 10 Mbps */
    CY_ECM_PHY_SPEED_100M,  /**< 100 Mbps */
    CY_ECM_PHY_SPEED_1000M, /**< 1000 Mbps */
    CY_ECM_PHY_SPEED_AUTO   /**< All 10/100/1000 Mbps */
} cy_ecm_phy_speed_t;

/** Standard interface type */
typedef enum
{
    CY_ECM_SPEED_TYPE_MII,   /**< Media-Independent Interface (MII) */
    CY_ECM_SPEED_TYPE_RMII,  /**< Reduced Media-Independent Interface (RMII) */
    CY_ECM_SPEED_TYPE_GMII,  /**< Gigabit Media-Independent Interface (GMII) */
    CY_ECM_SPEED_TYPE_RGMII  /**< Reduced Gigabit Media-Independent Interface (RGMII) */
} cy_ecm_speed_type_t;

/** Filter type */
typedef enum
{
    CY_ECM_FILTER_TYPE_DESTINATION = 0, /**< Filter on the destination address */
    CY_ECM_FILTER_TYPE_SOURCE      = 1, /**< filter on the source address */
} cy_ecm_filter_type_t;

/**
 * Enumeration of ECM events
 */
typedef enum
{
    CY_ECM_EVENT_CONNECTED = 0,      /**< Ethernet connection established event; notified on Ethernet link up       */
    CY_ECM_EVENT_DISCONNECTED,       /**< Ethernet disconnection event; notified on Ethernet link down  */
    CY_ECM_EVENT_IP_CHANGED          /**< IP address change event; notified after connection, re-connection, and IP address change due to DHCP renewal */
} cy_ecm_event_t;

/** \} group_ecm_enums */

/**
 * \addtogroup group_ecm_typedefs
 * \{
 */
/******************************************************
 *                 Type definitions
 ******************************************************/
/**
 * Ethernet Connection Manager handle
 */
typedef void* cy_ecm_t;

typedef uint8_t cy_ecm_mac_t[CY_ECM_MAC_ADDR_LEN];                   /**< Unique 6-byte MAC address represented in network byte order */

/**
 * ECM PHY Init callback function pointer type. Callback functions are invoked when the ECM needs to call PHY Ethernet driver API's during driver initialization.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_init)(uint8_t eth_idx, ETH_Type *reg_base);

/**
 * ECM PHY Configure callback function pointer type.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_configure)(uint8_t eth_idx, uint32_t duplex, uint32_t speed);

/**
 * ECM PHY Reset callback function pointer type.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_reset)(uint8_t eth_idx, ETH_Type *reg_base);

/**
 * ECM PHY Discover callback function pointer type.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_discover)(uint8_t eth_idx);

/**
 * ECM PHY enable extended Register callback function pointer type.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_enable_ext_reg)(ETH_Type *reg_base, uint32_t speed);

/**
 * ECM PHY Get Link speed callback function pointer type.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_get_linkspeed)(uint8_t eth_idx, uint32_t *duplex, uint32_t *speed);

/**
 * ECM PHY Get Link speed status function pointer type.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_get_linkstatus)(uint8_t eth_idx, uint32_t *link_status);

/**
 * ECM PHY Get Auto Negotiation callback function pointer type.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_get_auto_neg_status)(uint8_t eth_idx, uint32_t *neg_status);

/**
 * ECM PHY Get link partner capabilities callback function pointer type.
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef cy_rslt_t (*cy_ecm_phy_get_link_partner_cap)(uint8_t eth_idx, uint32_t *duplex, uint32_t *speed);

/** \} group_ecm_typedefs */

/**
 * \addtogroup group_ecm_structures
 * \{
 */

/******************************************************
 *             Structures
 ******************************************************/

/****************************************************************************************************************/
/**
 * Structure used to set or receive the IP address information to or from the underlying network stack
 */
typedef struct
{
    cy_ecm_ip_version_t version;  /**< IP version. */
    union
    {
        uint32_t v4;     /**< IPv4 address in network byte order */
        uint32_t v6[4];  /**< IPv6 address in network byte order */
    } ip;                /**< IP address bytes */
} cy_ecm_ip_address_t;

/**
 * Structure used to pass the static IP address information to \ref cy_ecm_connect
 */
typedef struct
{
    cy_ecm_ip_address_t  ip_address;  /**< IP address      */
    cy_ecm_ip_address_t  gateway;     /**< Gateway address */
    cy_ecm_ip_address_t  netmask;     /**< Netmask         */
} cy_ecm_ip_setting_t;

/**
 * Structure containing the configuration parameters to configure Ethernet PHY and MAC for data transfer handling
 */
typedef struct
{
    cy_ecm_speed_type_t interface_speed_type; /**< Standard interface to be used for data transfer  */
    cy_ecm_phy_speed_t phy_speed;             /**< Physical transfer speed */
    cy_ecm_duplex_t mode;                     /**< Transfer mode */
} cy_ecm_phy_config_t;

/**
 * Structure containing the configuration parameters to configure Ethernet PHY and MAC to filter the address during data transfer
 */
typedef struct
{
    cy_ecm_filter_type_t filter_type;                      /**< Type of address to be filtered */
    uint8_t              filter_addr[CY_ECM_MAC_ADDR_LEN]; /**< Address to be filtered */
    uint8_t              ignoreBytes;                      /**< For example,  ignoreBytes = 0x01 implies that the first byte received should not be compared. ignoreBytes = 0x03 implies that the first and second bytes received should not be compared. */
} cy_ecm_filter_address_t;

/**
 * Structure used to pass the callback function implementation for PHY related operations
 */
typedef struct cy_ecm_phy_callbacks
{
    cy_ecm_phy_init phy_init;                                   /**< Function pointer for Ethernet PHY Init.  */
    cy_ecm_phy_configure phy_configure;                         /**< Function pointer for Ethernet PHY Configure.  */
    cy_ecm_phy_reset phy_reset;                                 /**< Function pointer for Ethernet PHY Reset.  */
    cy_ecm_phy_discover phy_discover;                           /**< Function pointer for Ethernet PHY discover.  */
    cy_ecm_phy_enable_ext_reg phy_enable_ext_reg;               /**< Function pointer for Ethernet PHY enable extended registers.  */
    cy_ecm_phy_get_linkspeed phy_get_linkspeed;                 /**< Function pointer for Ethernet PHY get link speed.  */
    cy_ecm_phy_get_linkstatus phy_get_linkstatus;               /**< Function pointer for Ethernet PHY get link status.  */
    cy_ecm_phy_get_auto_neg_status phy_get_auto_neg_status;     /**< Function pointer for Ethernet PHY get auto negotiation status.  */
    cy_ecm_phy_get_link_partner_cap phy_get_link_partner_cap;   /**< Function pointer for Ethernet PHY get link partner capabilities.  */
} cy_ecm_phy_callbacks_t;

/** \} group_ecm_structures */

/**
 * \addtogroup group_ecm_union
 * \{
 */
/**
 * Union used to receive the IP address of the connected interface through the callback registered using \ref cy_ecm_register_event_callback.
 */
typedef union
{
    cy_ecm_ip_address_t ip_addr;  /**< Contains the IP address for the CY_ECM_EVENT_IP_CHANGED event */
} cy_ecm_event_data_t;

/** \} group_ecm_union */

/**
 * \addtogroup group_ecm_typedefs
 * \{
 */
/**
 * ECM event callback function pointer type; events are invoked when the Ethernet driver posts events to ECM.
 * @param[in] event            : ECM events
 * @param[in] event_data       : A pointer to the event data. The event data will be freed once the callback returns from the application.
 *
 * Note: The callback function will be executed in the context of the ECM.
 */
typedef void (*cy_ecm_event_callback_t)(cy_ecm_event_t event, cy_ecm_event_data_t *event_data);

/** \} group_ecm_typedefs */

/**
 * \addtogroup group_ecm_functions
 * \{
 * * The ECM library internally creates a thread; the created threads are executed with the "CY_RTOS_PRIORITY_ABOVENORMAL" priority. The definition of the CY_RTOS_PRIORITY_ABOVENORMAL macro is located at "libs/abstraction-rtos/include/COMPONENT_FREERTOS/cyabs_rtos_impl.h".
 * * ECM APIs are thread-safe.
 * * All ECM APIs except \ref cy_ecm_init and \ref cy_ecm_deinit are blocking APIs.
 * * All application callbacks invoked by the ECM will be running in the context of the ECM; the pointers passed as the argument in the callback function will be freed once the function returns.
 * * For the APIs that expect \ref cy_ecm_interface_t as an argument, unless a specific interface type has been called out in the description of the API, any valid ECM interface type can be passed as an argument to the API.
 */

/**
 * Does general allocation and initialization of resources needed for the library.
 * This API function must be called before using any other Ethernet Connection Manager API.
 *
 * \note \ref cy_ecm_init and \ref cy_ecm_deinit API functions are not thread-safe. The caller
 *       must ensure that these two API functions are not invoked simultaneously from different threads.
 *
 * @return CY_RSLT_SUCCESS if ECM initialization was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_INITIALIZED \n
 *             \ref CY_RSLT_ECM_NW_INIT_ERROR \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR
 */
cy_rslt_t cy_ecm_init( void );

/**
 * Releases the resources allocated in the \ref cy_ecm_init function.
 *
 * \note \ref cy_ecm_init and \ref cy_ecm_deinit API functions are not thread-safe. The caller
 *       must ensure that these two API functions are not invoked simultaneously from different threads.
 *
 * @return CY_RSLT_SUCCESS if ECM de-initialization was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_ECM_INIT_ERROR
 */
cy_rslt_t cy_ecm_deinit( void );

/**
 * Initializes the Ethernet physical driver,
 * enables Ethernet port, and brings up the network stack. This function should be called after calling \ref cy_ecm_init.
 * The handle to the ECM instance is returned via the handle pointer supplied by the user on success.
 *
 * \note
 * 1. Ethernet Connection Manager library returns error as required ethernet phy driver callbacks functions were not passed with this API.
 * 2. As a part of \ref cy_ecm_ethif_init, Ethernet driver initialization is called, which does GPIO and clock divider settings for the given physical configurations. But, Ethernet driver deinitialization is not available to clear these clock and GPIO settings.Hence, \ref cy_ecm_ethif_init cannot be called more than once in a single session, with different physical configurations.
 * 3. If either speed or duplex mode is set to AUTO, the auto-negotiation will be enabled. Application can call \ref cy_ecm_get_link_speed, to check the speed and duplex mode configured.
 *
 * @param[in]  eth_idx        : Ethernet port to be initialized
 * @param[in]  phy_callbacks  : Structure containing the Ethernet physical driver callback implementation for required ethernet PHY chip.
 * @param[out] ecm_handle     : Pointer to store the ECM handle allocated by this function on a successful return.
 *                              Caller should not free the handle directly. You should invoke \ref cy_ecm_ethif_deinit to free the handle.
 *
 * @return CY_RSLT_SUCCESS if ECM initialization was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_INITIALIZED \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR \n
 *             \ref CY_RSLT_ECM_ERROR_NOMEM \n
 *             \ref CY_RSLT_ECM_INIT_ERROR
 */
cy_rslt_t cy_ecm_ethif_init(cy_ecm_interface_t eth_idx,
                            cy_ecm_phy_callbacks_t *phy_callbacks,
                            cy_ecm_t *ecm_handle);

/**
 * Enable or Disable the promiscuous mode.
 *
 * This function configures the ethernet MAC to enable or disable the promiscuous mode. By default promiscuous mode will be disabled.
 * This function should be called after calling \ref cy_ecm_ethif_init ECM API.
 * 
 * @param[in]  ecm_handle           : ECM handle created using \ref cy_ecm_ethif_init.
 * @param[in]  is_promiscuous_mode  : true enables promiscuous mode, if false then this is disabled.
 *
 * @return CY_RSLT_SUCCESS if ECM configuration was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_INITIALIZED \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR
 */
cy_rslt_t cy_ecm_set_promiscuous_mode(cy_ecm_t ecm_handle, bool is_promiscuous_mode);

/**
 * Enable or Disable the incoming broadcast packets.
 *
 * This function configures the ethernet MAC to enable or disable the incoming broadcast packets.
 * By default broadcast packets are accepted. This function should be called after calling \ref cy_ecm_ethif_init ECM API.
 *
 * @param[in]  ecm_handle           : ECM handle created using \ref cy_ecm_ethif_init.
 * @param[in]  is_broadcast_disable : false broadcasts are accepted, if true they are rejected.
 *
 * @return CY_RSLT_SUCCESS if ECM configuration was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_INITIALIZED \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR
 */
cy_rslt_t cy_ecm_broadcast_disable(cy_ecm_t ecm_handle, bool is_broadcast_disable);

/**
 * Filter the receiving packets with the configured source or destination filter address.
 *
 * This function configures the filter address to the ethernet MAC to filter the receiving packets.
 * This function should be called after calling \ref cy_ecm_ethif_init ECM API.
 *
 * @param[in]  ecm_handle           : ECM handle created using \ref cy_ecm_ethif_init.
 * @param[in]  filter_address       : Array containing list of address to be filtered.
 * @param[in]  filter_address_count : Total count of addresses to be filtered. Maximum supported is CY_ECM_MAX_FILTER_ADDRESS.
 *
 * @return CY_RSLT_SUCCESS if ECM configuration was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_INITIALIZED \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR \n
 *             \ref CY_RSLT_ECM_ERROR
 */
cy_rslt_t cy_ecm_set_filter_address(cy_ecm_t ecm_handle, cy_ecm_filter_address_t *filter_address, uint8_t filter_address_count);

/**
 * Brings up the interface and configure to corresponding Ethernet port.
 *
 * This function brings up the network interface. This function should be called after calling the \ref cy_ecm_ethif_init ECM API.
 *
 * @param[in]  ecm_handle     : ECM handle created using \ref cy_ecm_ethif_init
 * @param[in]  static_ip_addr : Configuration of the static IP address. If NULL, the IP address is created using DHCP.
 * @param[out] ip_addr        : Pointer to return the IPv4 address (optional)
 *
 * @return CY_RSLT_SUCCESS if ECM configuration was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_INITIALIZED \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR \n
 *             \ref CY_RSLT_ECM_BAD_STATIC_IP \n
 *             \ref CY_RSLT_ECM_INTERFACE_ERROR \n
 *             \ref CY_RSLT_MODULE_ECM_ALREADY_CONNECTED \n
 *             \ref CY_RSLT_ECM_ERROR
 */
cy_rslt_t cy_ecm_connect(cy_ecm_t ecm_handle, cy_ecm_ip_setting_t *static_ip_addr, cy_ecm_ip_address_t *ip_addr);

/**
 * Brings down the interface.
 *
 * This function should be called after calling the \ref cy_ecm_connect ECM API.
 *
 * @param[in]  ecm_handle: ECM handle created using \ref cy_ecm_ethif_init.
 *
 * @return CY_RSLT_SUCCESS if ECM configuration was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_INITIALIZED \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR \n
 *             \ref CY_RSLT_ECM_INTERFACE_ERROR \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_CONNECTED \n
 *             \ref CY_RSLT_ECM_ERROR
 */
cy_rslt_t cy_ecm_disconnect(cy_ecm_t ecm_handle);

/**
 * Registers an event callback to monitor the connection and IP address change events.
 * This is an optional registration; use it if the application needs to monitor events across disconnection and reconnection.
 *
 * @param[in]  ecm_handle     : ECM handle created using \ref cy_ecm_ethif_init
 * @param[in]  event_callback : Callback function to be invoked for event notification.
 *                              The callback will be executed in the context of the ECM.
 *
 * @return CY_RSLT_SUCCESS if application callback registration was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_ERROR
*/
cy_rslt_t cy_ecm_register_event_callback(cy_ecm_t ecm_handle, cy_ecm_event_callback_t event_callback);

/**
 * Deregisters an event callback
 *
 * @param[in]  ecm_handle     : ECM handle obtained by \ref cy_ecm_ethif_init
 * @param[in]  event_callback : Callback function to deregister from getting notifications
 *
 * @return CY_RSLT_SUCCESS if application callback de-registration was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_ERROR
*/
cy_rslt_t cy_ecm_deregister_event_callback(cy_ecm_t ecm_handle, cy_ecm_event_callback_t event_callback);

/**
 * Provides the status of the link
 *
 * @param[in]   ecm_handle : ECM handle created using \ref cy_ecm_ethif_init
 * @param[out]  status     : Pointer to store the link status(0=down, 1=up)
 *
 * @return CY_RSLT_SUCCESS if link status reading was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_ERROR
*/
cy_rslt_t cy_ecm_get_link_status(cy_ecm_t ecm_handle, bool *status);

/**
 * Retrieves the IPv4 address of the given interface
 *
 * @param[in]   ecm_handle : ECM handle created using \ref cy_ecm_ethif_init
 * @param[out]  ip_addr    : Pointer to an IP address structure (or) an IP address structure array
 *
 * @return CY_RSLT_SUCCESS if IP address get is successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_IP_ADDR_ERROR
 */
cy_rslt_t cy_ecm_get_ip_address(cy_ecm_t ecm_handle, cy_ecm_ip_address_t *ip_addr);

/**
 * Retrieves the IPv6 address of the given interface
 *
 * Note: Currently, this API supports only the \ref CY_ECM_IPV6_LINK_LOCAL type.
 *
 * @param[in]   ecm_handle      : ECM handle created using \ref cy_ecm_ethif_init
 * @param[in]   ipv6_addr_type  : IPv6 address type
 * @param[out]  ip_addr         : Pointer to a structure filled with IP address
 *
 * @return CY_RSLT_SUCCESS if IPv6 interface is up and IPv6 address is ready; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_IPV6_GLOBAL_ADDRESS_NOT_SUPPORTED \n
 *             \ref CY_RSLT_ECM_IPV6_INTERFACE_NOT_READY \n
 *             \ref CY_RSLT_ECM_ERROR
 */
cy_rslt_t cy_ecm_get_ipv6_address(cy_ecm_t ecm_handle, cy_ecm_ipv6_type_t ipv6_addr_type, cy_ecm_ip_address_t *ip_addr);

/**
 * Retrieves the gateway IP address of the given interface
 *
 * @param[in]   ecm_handle   : ECM handle created using \ref cy_ecm_ethif_init
 * @param[out]  gateway_addr : Pointer to a structure filled with the gateway IP address
 *
 * @return CY_RSLT_SUCCESS if retrieval of the gateway IP address was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_GATEWAY_ADDR_ERROR \n
 *             \ref CY_RSLT_ECM_ERROR
 */
cy_rslt_t cy_ecm_get_gateway_address(cy_ecm_t ecm_handle, cy_ecm_ip_address_t *gateway_addr);

/**
 * Retrieves the subnet mask address of the given interface
  *
 * @param[in]   ecm_handle    : ECM handle created using \ref cy_ecm_ethif_init
 * @param[out]  net_mask_addr : Pointer to a structure filled with the subnet mask address
 *
 * @return CY_RSLT_SUCCESS if retrieval of the subnet mask address was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_NETMASK_ADDR_ERROR \n
 *             \ref CY_RSLT_ECM_ERROR
 */
cy_rslt_t cy_ecm_get_netmask_address(cy_ecm_t ecm_handle, cy_ecm_ip_address_t *net_mask_addr);

/**
 * Retrieves the MAC address of the gateway for the Ethernet interface. Uses Address Resolution Protocol (ARP) to retrieve the gateway MAC address.
 *
 * This function is a blocking call and uses an internal timeout while running ARP.
 *
 * @param[in]   ecm_handle : ECM handle created using \ref cy_ecm_ethif_init
 * @param[out]  mac_addr   : Pointer to a MAC address structure which is filled with the gateway's MAC address on successful return.
 *
 * @return CY_RSLT_SUCCESS if retrieval of the gateway MAC address was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_GATEWAY_MAC_ADDR_ERROR \n
 *             \ref CY_RSLT_ECM_ERROR
 */
cy_rslt_t cy_ecm_get_mac_address(cy_ecm_t ecm_handle, cy_ecm_mac_t *mac_addr);

/**
 * Sends a ping request to the given IP address. This function is a blocking call; it returns after the specified timeout.
 *
 * @param[in]  ecm_handle  : ECM handle created using \ref cy_ecm_ethif_init
 * @param[in]  ip_addr     : Pointer to the destination IP address structure to which the ping request will be sent
 * @param[in]  timeout_ms  : Ping request timeout in milliseconds
 * @param[out] elapsed_ms  : Pointer to store the round-trip time (in milliseconds);
 *                           i.e., the time taken to receive the ping response from the destination.
 *
 * @return CY_RSLT_SUCCESS if ping to the IP address was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_PING_REQUEST_TIMEOUT \n
 *             \ref CY_RSLT_ECM_PING_FAILURE \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR
 */
cy_rslt_t cy_ecm_ping(cy_ecm_t ecm_handle, cy_ecm_ip_address_t *ip_addr, uint32_t timeout_ms, uint32_t* elapsed_ms);

/**
 * Retrieves the configured ethernet speed and duplex mode
 *
 * @param[in]   ecm_handle : ECM handle created using \ref cy_ecm_ethif_init
 * @param[out]  duplex     : Pointer to cy_ecm_duplex_t enum which is filled with the duplex mode on successful return
 * @param[out]  speed      : Pointer to cy_ecm_speed_t enum which is filled with the link speed on successful return
 *
 * @return CY_RSLT_SUCCESS if retrieval of the link speed and mode was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_ERROR
 */
cy_rslt_t cy_ecm_get_link_speed(cy_ecm_t ecm_handle, cy_ecm_duplex_t *duplex, cy_ecm_phy_speed_t *speed);

/**
 * Deinitializes the Ethernet physical driver.
 * Disables Ethernet port, brings down the network stack, and frees the handle. This function should be called after calling \ref cy_ecm_ethif_init.
 *
 * @param[in, out]  ecm_handle : Pointer containing the ECM handle created using \ref cy_ecm_ethif_init
 *
 * @return CY_RSLT_SUCCESS if ECM de-initialization was successful; an error code on failure.
 *             Important error code related to this API function are: \n
 *             \ref CY_RSLT_MODULE_ECM_BADARG \n
 *             \ref CY_RSLT_ECM_MUTEX_ERROR \n
 *             \ref CY_RSLT_MODULE_ECM_NOT_INITIALIZED
 */
cy_rslt_t cy_ecm_ethif_deinit(cy_ecm_t *ecm_handle);

/** \} group_ecm_functions */

#ifdef __cplusplus
} /* extern C */
#endif
