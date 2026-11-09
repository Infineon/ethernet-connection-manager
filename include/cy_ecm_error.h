/*
 * (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
 */

#ifndef LIBS_ECM_INCLUDE_CY_ECM_ERROR_H_
#define LIBS_ECM_INCLUDE_CY_ECM_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cy_result.h"
#include "cy_result_mw.h"

/**
 * @defgroup cy_ecm_error ECM-specific error codes
 * @ingroup group_ecm_macros
 * Cypress middleware APIs return results of type cy_rslt_t.
 *
 * It consists of three parts:
 * - module base
 * - type
 * - error code
 *
 * \par Result format
 *
   \verbatim
              Module base                   Type    Library-specific error code
      +-----------------------------------+------+------------------------------+
      |CY_RSLT_MODULE_ECM_BASE            | 0x2  |           Error code         |
      +-----------------------------------+------+------------------------------+
                14 bits                    2 bits            16 bits

   See the macro section of this document for library-specific error codes.
   \endverbatim
 *
 * The data structure cy_rslt_t is part of cy_result.h located in <core_lib/include>.
 *
 * Module base: This base is derived from CY_RSLT_MODULE_MIDDLEWARE_BASE (defined in cy_result.h) and is an offset of the CY_RSLT_MODULE_MIDDLEWARE_BASE.
 *              The details of the offset and the middleware base are defined in cy_result_mw.h, that is part of [GitHub connectivity-utilities] (https://github.com/infineon/connectivity-utilities)
 *              For example, Ethernet Connection Manager (ECM) uses CY_RSLT_MODULE_ECM_BASE as the module base.
 *
 * Type: This type is defined in cy_result.h and can be one of CY_RSLT_TYPE_FATAL, CY_RSLT_TYPE_ERROR, CY_RSLT_TYPE_WARNING, or CY_RSLT_TYPE_INFO. ECM library error codes are of type CY_RSLT_TYPE_ERROR.
 *
 * Library-specific error code: These error codes are library-specific and defined in the macro section.
 *
 * Helper macros used for creating the library-specific result are provided as part of cy_result.h.
 * \{
 */
/** Generic ECM base error code */

#define CY_RSLT_ECM_ERR_BASE                                      CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ECM_BASE, 0)

/** Invalid argument */
#define CY_RSLT_MODULE_ECM_BADARG                                 (CY_RSLT_ECM_ERR_BASE + 1)
/** Library not initialized */
#define CY_RSLT_MODULE_ECM_NOT_INITIALIZED                        (CY_RSLT_ECM_ERR_BASE + 2)
/** Denotes a failure to start DHCP      */
#define CY_RSLT_MODULE_ECM_ERROR_STARTING_DHCP                    (CY_RSLT_ECM_ERR_BASE + 3)
/** Invalid configuration */
#define CY_RSLT_MODULE_ECM_INVALID_OPTION                         (CY_RSLT_ECM_ERR_BASE + 4)
/** Ethernet not connected */
#define CY_RSLT_MODULE_ECM_NOT_CONNECTED                          (CY_RSLT_ECM_ERR_BASE + 5)
/** Ethernet disconnected */
#define CY_RSLT_MODULE_ECM_DISCONNECTED                           (CY_RSLT_ECM_ERR_BASE + 6)
/** Ethernet already connected */
#define CY_RSLT_MODULE_ECM_ALREADY_CONNECTED                      (CY_RSLT_ECM_ERR_BASE + 7)
/** Mutex error         */
#define CY_RSLT_ECM_MUTEX_ERROR                                   (CY_RSLT_ECM_ERR_BASE + 8)
/** IPv6 global IP not supported */
#define CY_RSLT_ECM_IPV6_GLOBAL_ADDRESS_NOT_SUPPORTED             (CY_RSLT_ECM_ERR_BASE + 9)
/** IPv6 interface not ready */
#define CY_RSLT_ECM_IPV6_INTERFACE_NOT_READY                      (CY_RSLT_ECM_ERR_BASE + 10)
/** Failure in executing the ping request */
#define CY_RSLT_ECM_PING_FAILURE                                  (CY_RSLT_ECM_ERR_BASE + 11)
/** Denotes the DHCP wait timeout        */
#define CY_RSLT_ECM_DHCP_TIMEOUT                                  (CY_RSLT_ECM_ERR_BASE + 12)
/** Ping request timed out */
#define CY_RSLT_ECM_PING_REQUEST_TIMEOUT                          (CY_RSLT_ECM_ERR_BASE + 13)
/** Static IP address not supported for IPv6 */
#define CY_RSLT_ECM_STATIC_IP_NOT_SUPPORTED                       (CY_RSLT_ECM_ERR_BASE + 14)
/** Bad static IP address */
#define CY_RSLT_ECM_BAD_STATIC_IP                                 (CY_RSLT_ECM_ERR_BASE + 15)
/** Interface is not initialized/already initialized */
#define CY_RSLT_ECM_INTERFACE_ERROR                               (CY_RSLT_ECM_ERR_BASE + 16)
/** Failed to get the gateway address  */
#define CY_RSLT_ECM_GATEWAY_ADDR_ERROR                            (CY_RSLT_ECM_ERR_BASE + 17)
/** Failed to get the netmask address  */
#define CY_RSLT_ECM_NETMASK_ADDR_ERROR                            (CY_RSLT_ECM_ERR_BASE + 18)
/** Failed to get the IP address */
#define CY_RSLT_ECM_IP_ADDR_ERROR                                 (CY_RSLT_ECM_ERR_BASE + 19)
/** Failed to get the gateway MAC address  */
#define CY_RSLT_ECM_GATEWAY_MAC_ADDR_ERROR                        (CY_RSLT_ECM_ERR_BASE + 20)
/** Failed to initialize the network stack */
#define CY_RSLT_ECM_NW_INIT_ERROR                                 (CY_RSLT_ECM_ERR_BASE + 21)
/** Failed to initialize the library */
#define CY_RSLT_ECM_INIT_ERROR                                    (CY_RSLT_ECM_ERR_BASE + 22)
/** No memory error         */
#define CY_RSLT_ECM_ERROR_NOMEM                                   (CY_RSLT_ECM_ERR_BASE + 23)
/** Generic error from the library */
#define CY_RSLT_ECM_ERROR                                         (CY_RSLT_ECM_ERR_BASE + 24)
/** Interface not supported */
#define CY_RSLT_ECM_INTERFACE_NOT_SUPPORTED                       (CY_RSLT_ECM_ERR_BASE + 25)

/** \} Error codes */

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* LIBS_ECM_INCLUDE_CY_ECM_ERROR_H_ */
