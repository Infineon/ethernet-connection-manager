/*
 * Copyright 2022, Cypress Semiconductor Corporation (an Infineon company) or
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
* @file eth_internal.h
* @brief This file provides platform independent configuration macros for Ethernet.
*/

#ifndef ETHERNET_INTERNAL_H
#define ETHERNET_INTERNAL_H

#include "cy_eth_user_config.h"

extern int eth_index_internal;

#if CY_IP_MXETH_INSTANCES > 1
#define ETH_INTERFACE_TYPE (eth_index_internal == CY_ECM_INTERFACE_ETH1) ? ETH1 : ETH0
#else
#define ETH_INTERFACE_TYPE ETH0
#endif

 /* After hardware initialization, max wait time to get the physical link up */
#define MAX_WAIT_ETHERNET_PHY_STATUS          (10000)

#define REGISTER_ADDRESS_PHY_REG_BMCR       PHYREG_00_BMCR          /* BMCR register (0x0000) to read the speed and duplex mode */
#define REGISTER_PHY_REG_DUPLEX_MASK        PHYBMCR_FULL_DUPLEX_Msk /* Bit 8 of BMCR register to read the duplex mode */
#define REGISTER_PHY_REG_SPEED_MASK         (0x2040)                /* Bit 6, 13: BMCR register to read the speed */
#define REGISTER_PHY_REG_SPEED_MASK_10M     (0x0000)                /* Bit 6, 13: Both are set to 0 for 10M speed */
#define REGISTER_PHY_REG_SPEED_MASK_100M    (0x2000)                /* Bit 6, 13: Set to 0 and 1 respectively for 100M speed */
#define REGISTER_PHY_REG_SPEED_MASK_1000M   (0x0040)                /* Bit 6, 13: Set to 1 and 0 respectively for 1000M speed */

cy_rslt_t  cy_eth_driver_initialization(ETH_Type *eth_type, cy_ecm_phy_config_t *ecm_phy_config, cy_stc_ephy_t *phy_obj);
void deregister_cb(ETH_Type *reg_base);

#endif /* ETHERNET_INTERNAL_H */ 
