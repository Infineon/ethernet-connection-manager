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

/**
* @file eth_internal.h
* @brief This file provides platform independent configuration macros for Ethernet.
*/

#ifndef ETHERNET_INTERNAL_H
#define ETHERNET_INTERNAL_H

#include "cy_result.h"
#include "cy_ethif.h"
#include "cy_ecm.h"

extern int eth_index_internal;

#if CY_IP_MXETH_INSTANCES > 1
#define ETH_INTERFACE_TYPE (eth_index_internal == CY_ECM_INTERFACE_ETH1) ? ETH1 : ETH0
#else
#define ETH_INTERFACE_TYPE ETH0
#endif

 /* After hardware initialization, max wait time to get the physical link up */
#define MAX_WAIT_ETHERNET_PHY_STATUS          (10000)

/**
 * Structure containing the configuration parameters to configure Ethernet PHY and MAC for data transfer handling
 */
typedef struct
{
    cy_ecm_speed_type_t interface_speed_type; /**< Standard interface to be used for data transfer  */
    cy_ecm_phy_speed_t phy_speed;             /**< Physical transfer speed */
    cy_ecm_duplex_t mode;                     /**< Transfer mode */
} cy_ecm_phy_config_t;

cy_rslt_t  cy_eth_driver_initialization(cy_ecm_interface_t eth_idx, ETH_Type *eth_type, cy_ecm_phy_config_t *ecm_phy_config, cy_ecm_phy_callbacks_t *phy_callbacks);
void deregister_cb(ETH_Type *reg_base);

#endif /* ETHERNET_INTERNAL_H */ 
