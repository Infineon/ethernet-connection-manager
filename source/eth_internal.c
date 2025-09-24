/*
 * Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
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
* @file eth_internal.c
* @brief This file provides set of APIs that communicate with ethernet PDL driver for initialization,
* configuration, connection.
*/

#include "eth_internal.h"
#include "cy_sysint.h"
#include <string.h>
#include <stdlib.h>

#include "cyabs_rtos.h"
#include "cy_log.h"
#include "cy_ecm.h"
#include "cy_ecm_error.h"
#include "cycfg.h"

#ifdef ENABLE_ECM_LOGS
#define cy_ecm_log_msg cy_log_msg
#else
#define cy_ecm_log_msg(a,b,c,...)
#endif

#define SLEEP_ETHERNET_PHY_STATUS                 (1) /* Sleep time in milliseconds. */

/********************************************************/
extern uint8_t *pRx_Q_buff_pool[CY_ETH_DEFINE_TOTAL_BD_PER_RXQUEUE];

/********************************************************/
extern void cy_process_ethernet_data_cb(ETH_Type *eth_type, uint8_t * rx_buffer, uint32_t length);
extern void cy_notify_ethernet_rx_data_cb(ETH_Type *base, uint8_t **u8RxBuffer, uint32_t *u32Length);
extern void cy_tx_complete_cb ( ETH_Type *pstcEth, uint8_t u8QueueIndex );
extern void cy_tx_failure_cb ( ETH_Type *pstcEth, uint8_t u8QueueIndex );
static void cy_eth_phy_initialization ( cy_ecm_interface_t eth_idx,
                                        ETH_Type *reg_base,
                                        cy_ecm_phy_config_t *ecm_phy_config,
                                        cy_ecm_phy_callbacks_t *phy_callbacks );

/********************************************************/

static bool is_driver_configured = false;

static cy_stc_ethif_wrapper_config_t stcWrapperConfig;

int eth_index_internal;

/** General Ethernet configuration  */
static cy_stc_ethif_mac_config_t stcENETConfig = {
                .bintrEnable         = 1,                           /** Interrupt enable  */
                .dmaDataBurstLen     = CY_ETHIF_DMA_DBUR_LEN_4,
                .u8dmaCfgFlags       = CY_ETHIF_CFG_DMA_FRCE_TX_BRST,
#if defined(CY_DEVICE_SERIES_PSOCE84)
                .mdcPclkDiv          = CY_ETHIF_MDC_DIV_BY_96,      /** source clock is 200 MHz and MDC must be less than 2.5MHz   */
#else
                .mdcPclkDiv          = CY_ETHIF_MDC_DIV_BY_48,      /** source clock is 100 MHz and MDC must be less than 2.5MHz   */
#endif
                .u8rxLenErrDisc      = 0,                           /** Length error frame not discarded  */
                .u8disCopyPause      = 0,
                .u8chkSumOffEn       = 0,                           /** Checksum for both Tx and Rx disabled    */
                .u8rx1536ByteEn      = 1,                           /** Enable receive frame up to 1536    */
                .u8rxJumboFrEn       = 0,
                .u8enRxBadPreamble   = 1,
                .u8ignoreIpgRxEr     = 0,
                .u8storeUdpTcpOffset = 0,
                .u8aw2wMaxPipeline   = 2,                           /** Value must be > 0   */
                .u8ar2rMaxPipeline   = 2,                           /** Value must be > 0   */
                .u8pfcMultiQuantum   = 0,
                .pstcWrapperConfig   = &stcWrapperConfig,
                .pstcTSUConfig       = NULL, //&stcTSUConfig,               /** TSU settings    */
                .btxq0enable         = 1,                           /** Tx Q0 Enabled   */
                .btxq1enable         = 0,                           /** Tx Q1 Disabled  */
                .btxq2enable         = 0,                           /** Tx Q2 Disabled  */
                .brxq0enable         = 1,                           /** Rx Q0 Enabled   */
                .brxq1enable         = 0,                           /** Rx Q1 Disabled  */
                .brxq2enable         = 0,                           /** Rx Q2 Disabled  */
};

/** Interrupt configurations    */
static cy_stc_ethif_intr_config_t stcInterruptConfig = {
                .btsu_time_match        = 0,          /** Timestamp unit time match event */
                .bwol_rx                = 0,          /** Wake-on-LAN event received */
                .blpi_ch_rx             = 0,          /** LPI indication status bit change received */
                .btsu_sec_inc           = 0,          /** TSU seconds register increment */
                .bptp_tx_pdly_rsp       = 0,          /** PTP pdelay_resp frame transmitted */
                .bptp_tx_pdly_req       = 0,          /** PTP pdelay_req frame transmitted */
                .bptp_rx_pdly_rsp       = 0,          /** PTP pdelay_resp frame received */
                .bptp_rx_pdly_req       = 0,          /** PTP pdelay_req frame received */
                .bptp_tx_sync           = 0,          /** PTP sync frame transmitted */
                .bptp_tx_dly_req        = 0,          /** PTP delay_req frame transmitted */
                .bptp_rx_sync           = 0,          /** PTP sync frame received */
                .bptp_rx_dly_req        = 0,          /** PTP delay_req frame received */
                .bext_intr              = 0,          /** External input interrupt detected */
                .bpause_frame_tx        = 0,          /** Pause frame transmitted */
                .bpause_time_zero       = 0,          /** Pause time reaches zero or zero pause frame received */
                .bpause_nz_qu_rx        = 0,          /** Pause frame with non-zero quantum received */
                .bhresp_not_ok          = 0,          /** DMA HRESP not OK */
                .brx_overrun            = 1,          /** Rx overrun error */
                .bpcs_link_change_det   = 0,          /** Link status change detected by PCS */
                .btx_complete           = 1,          /** Frame has been transmitted successfully */
                .btx_fr_corrupt         = 1,          /** Tx frame corrupted */
                .btx_retry_ex_late_coll = 1,          /** Retry limit exceeded or late collision */
                .btx_underrun           = 1,          /** Tx underrun */
                .btx_used_read          = 1,          /** Used bit set has been read in Tx descriptor list */
                .brx_used_read          = 1,          /** Used bit set has been read in Rx descriptor list */
                .brx_complete           = 1,          /** Frame received successfully and stored */
                .bman_frame             = 0,          /** Management frame sent */
};

static cy_stc_ethif_cb_t stcInterruptCB = {
    /** Callback functions  */
                .rxframecb  = cy_process_ethernet_data_cb, //Ethx_RxFrameCB,
                .txerrorcb  = cy_tx_failure_cb,
                .txcompletecb = cy_tx_complete_cb, /** Set it to NULL if callback is not required */
                .tsuSecondInccb = NULL,
                .rxgetbuff = cy_notify_ethernet_rx_data_cb
};

/** Enable Ethernet interrupts  */
#if (defined (eth_0_ENABLED) && (eth_0_ENABLED == 1u))
static const cy_stc_sysint_t irq_cfg_eth0_q0 = {.intrSrc  = (eth_0_INTRSRC_Q0), .intrPriority=eth_0_INTRPRIORITY};
static const cy_stc_sysint_t irq_cfg_eth0_q1 = {.intrSrc  = (eth_0_INTRSRC_Q1), .intrPriority=eth_0_INTRPRIORITY};
static const cy_stc_sysint_t irq_cfg_eth0_q2 = {.intrSrc  = (eth_0_INTRSRC_Q2), .intrPriority=eth_0_INTRPRIORITY};
#endif

#if (defined (eth_1_ENABLED) && (eth_1_ENABLED == 1u))
static const cy_stc_sysint_t irq_cfg_eth1_q0 = {.intrSrc  = (eth_1_INTRSRC_Q0), .intrPriority=eth_1_INTRPRIORITY};
static const cy_stc_sysint_t irq_cfg_eth1_q1 = {.intrSrc  = (eth_1_INTRSRC_Q1), .intrPriority=eth_1_INTRPRIORITY};
static const cy_stc_sysint_t irq_cfg_eth1_q2 = {.intrSrc  = (eth_1_INTRSRC_Q2), .intrPriority=eth_1_INTRPRIORITY};
#endif

/********************************************************/
/** Interrupt handlers for Ethernet 0     */
#if (defined (eth_0_ENABLED) && (eth_0_ENABLED == 1u))
static void Cy_Eth0_InterruptHandler (void)
{
    Cy_ETHIF_DecodeEvent(ETH0);
}
#endif

/** Interrupt handlers for Ethernet 1     */
#if (defined (eth_1_ENABLED) && (eth_1_ENABLED == 1u))
static void Cy_Eth1_InterruptHandler (void)
{
    Cy_ETHIF_DecodeEvent(ETH1);
}
#endif

static cy_en_ethif_speed_sel_t ecm_config_to_speed_sel( cy_ecm_phy_config_t *config)
{
    cy_en_ethif_speed_sel_t speed_sel;

    if( config->interface_speed_type == CY_ECM_SPEED_TYPE_MII)
    {
        speed_sel = (cy_en_ethif_speed_sel_t)config->phy_speed;
    }
    else if( config->interface_speed_type == CY_ECM_SPEED_TYPE_GMII)
    {
        speed_sel = CY_ETHIF_CTL_GMII_1000;
    }
    else if( config->interface_speed_type == CY_ECM_SPEED_TYPE_RGMII)
    {
        if(config->phy_speed == CY_ECM_PHY_SPEED_10M)
        {
            speed_sel = CY_ETHIF_CTL_RGMII_10;
        }
        else if(config->phy_speed == CY_ECM_PHY_SPEED_100M)
        {
            speed_sel = CY_ETHIF_CTL_RGMII_100;
        }
        else
        {
            speed_sel = CY_ETHIF_CTL_RGMII_1000;
        }
    }
    else
    {
        speed_sel = (config->phy_speed == CY_ECM_PHY_SPEED_10M)?CY_ETHIF_CTL_RMII_10 : CY_ETHIF_CTL_RMII_100;
    }

    return speed_sel;
}

static void eth_clock_config(cy_ecm_interface_t eth_idx, cy_en_ethif_speed_sel_t speed_sel, cy_ecm_phy_speed_t phy_speed)
{
    if((speed_sel == CY_ETHIF_CTL_MII_10) && (phy_speed == CY_ECM_PHY_SPEED_10M))
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_MII_10;       /** 10 Mbps MII */
    else if((speed_sel == CY_ETHIF_CTL_MII_100) && (phy_speed == CY_ECM_PHY_SPEED_100M))
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_MII_100;      /** 100 Mbps MII */
    else if((speed_sel == CY_ETHIF_CTL_GMII_1000) && (phy_speed == CY_ECM_PHY_SPEED_1000M))
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_GMII_1000;    /** 1000 Mbps GMII */
    else if((speed_sel == CY_ETHIF_CTL_RGMII_10) && (phy_speed == CY_ECM_PHY_SPEED_10M))
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_RGMII_10;     /** 10 Mbps RGMII */
    else if((speed_sel == CY_ETHIF_CTL_RGMII_100) && (phy_speed == CY_ECM_PHY_SPEED_100M))
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_RGMII_100;    /** 100 Mbps RGMII */
    else if((speed_sel == CY_ETHIF_CTL_RGMII_1000) && (phy_speed == CY_ECM_PHY_SPEED_1000M))
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_RGMII_1000;    /** 1000 Mbps RGMII */
    else if((speed_sel == CY_ETHIF_CTL_RMII_10) && (phy_speed == CY_ECM_PHY_SPEED_10M))
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_RMII_10;      /** 10 Mbps RMII */
    else if((speed_sel == CY_ETHIF_CTL_RMII_100) && (phy_speed == CY_ECM_PHY_SPEED_100M))
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_RMII_100;     /** 100 Mbps RMII */
    else
        stcWrapperConfig.stcInterfaceSel = CY_ETHIF_CTL_RGMII_1000;  /** Error in configuration */
    if(eth_idx == CY_ECM_INTERFACE_ETH0)
    {
#if (defined (eth_0_ENABLED) && (eth_0_ENABLED == 1u))
        stcWrapperConfig.bRefClockSource = (cy_en_ethif_clock_ref_t)eth_0_MAC_CLOCK;     /** Assigning Ref_Clk to HSIO clock; use an external clock from HSIO  */
#endif
    }
    else if(eth_idx == CY_ECM_INTERFACE_ETH1)
    {
#if (defined (eth_1_ENABLED) && (eth_1_ENABLED == 1u))
        stcWrapperConfig.bRefClockSource = (cy_en_ethif_clock_ref_t)eth_1_MAC_CLOCK;     /** Assigning Ref_Clk to HSIO clock; use an external clock from HSIO  */
#endif
    }
    else
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() Invalid Ethernet Interface... \n", __FUNCTION__ );
    }

    if(phy_speed == CY_ECM_PHY_SPEED_10M)
    {
#if (defined (CYCFG_XMC7100_DEFINED) && (CYCFG_XMC7100_DEFINED == 1u))
        stcWrapperConfig.u8RefClkDiv = 1;                          /** RefClk: 2.5 MHz; divide Refclock by 1 to have a 25-MHz Tx clock  */
#else
        stcWrapperConfig.u8RefClkDiv = 10;                         /** RefClk: 25 MHz; divide Refclock by 10 to have a 2.5-MHz Tx clock  */
#endif
    }
    else if(phy_speed == CY_ECM_PHY_SPEED_100M)
    {
        stcWrapperConfig.u8RefClkDiv = 1;                          /** RefClk: 25 MHz; divide Refclock by 1 to have a 25-MHz Tx clock  */
    }
    else if(phy_speed == CY_ECM_PHY_SPEED_1000M)
    {
        stcWrapperConfig.u8RefClkDiv = 1;                          /** RefClk: 25 MHz; divide Refclock by 1 to have a 25-MHz Tx clock  */
    }
    else /*(phy_speed == CY_ECM_PHY_SPEED_1000M)*/
    {
        stcWrapperConfig.u8RefClkDiv = 1;                          /** RefClk: 125 MHz; divide Refclock by 1 to have a 125-MHz Tx clock || Although only relevant in RGMII/GMII modes */
    }

    return;
}

cy_rslt_t cy_eth_driver_initialization(cy_ecm_interface_t eth_idx,
                                       ETH_Type *reg_base,
                                       cy_ecm_phy_config_t *ecm_phy_config,
                                       cy_ecm_phy_callbacks_t *phy_callbacks)
{
    cy_rslt_t  result = CY_RSLT_SUCCESS;
    uint32_t   retry_count = 0, link_status = 0;

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if(eth_idx == CY_ECM_INTERFACE_ETH0)
    {
#if (defined (eth_0_ENABLED) && (eth_0_ENABLED == 1u))
        Cy_SysInt_Init(&irq_cfg_eth0_q0, Cy_Eth0_InterruptHandler);
        Cy_SysInt_Init(&irq_cfg_eth0_q1, Cy_Eth0_InterruptHandler);
        Cy_SysInt_Init(&irq_cfg_eth0_q2, Cy_Eth0_InterruptHandler);

#if (!CY_IP_M7CPUSS)
    NVIC_EnableIRQ((IRQn_Type) irq_cfg_eth0_q0.intrSrc);
    NVIC_EnableIRQ((IRQn_Type) irq_cfg_eth0_q0.intrSrc);
    NVIC_EnableIRQ((IRQn_Type) irq_cfg_eth0_q0.intrSrc);
#else
    NVIC_ClearPendingIRQ((IRQn_Type)eth_0_INTRMUXNUMBER);
    NVIC_EnableIRQ((IRQn_Type)eth_0_INTRMUXNUMBER);
#endif

#endif
    }
    else
    {
#if (defined (eth_1_ENABLED) && (eth_1_ENABLED == 1u))
        Cy_SysInt_Init(&irq_cfg_eth1_q0, Cy_Eth1_InterruptHandler);
        Cy_SysInt_Init(&irq_cfg_eth1_q1, Cy_Eth1_InterruptHandler);
        Cy_SysInt_Init(&irq_cfg_eth1_q2, Cy_Eth1_InterruptHandler);
        NVIC_ClearPendingIRQ((IRQn_Type)eth_1_INTRMUXNUMBER);
        NVIC_EnableIRQ((IRQn_Type)eth_1_INTRMUXNUMBER);
#endif
    }

    /* rx Q0 buffer pool */
    stcENETConfig.pRxQbuffPool[0] = (cy_ethif_buffpool_t *)&pRx_Q_buff_pool;
    stcENETConfig.pRxQbuffPool[1] = NULL;

    /** Initialize PHY  */
    cy_eth_phy_initialization(eth_idx, reg_base, ecm_phy_config, phy_callbacks);

    while( retry_count < MAX_WAIT_ETHERNET_PHY_STATUS)
    {
        result = phy_callbacks->phy_get_linkstatus((uint8_t)eth_idx, &link_status);
        if(result == CY_RSLT_SUCCESS)
        {
            if(link_status == 1)
            {
                result = CY_RSLT_SUCCESS;
                break;
            }
        }
        cy_rtos_delay_milliseconds(SLEEP_ETHERNET_PHY_STATUS);
        retry_count += SLEEP_ETHERNET_PHY_STATUS;
    }

    if(retry_count > MAX_WAIT_ETHERNET_PHY_STATUS)
    {
        cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Link up failed\n" );
        result = CY_RSLT_ECM_ERROR;
    }

    Cy_ETHIF_RegisterCallbacks(reg_base, &stcInterruptCB);

    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s():retry_count:[%d] END \n", __FUNCTION__, retry_count );

    return result;
}


void deregister_cb(ETH_Type *reg_base)
{
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Deregister driver callbacks \n" );
    stcInterruptCB.rxframecb  = NULL;
    Cy_ETHIF_RegisterCallbacks(reg_base, &stcInterruptCB);
}

/*******************************************************************************
* Function name: cy_eth_phy_initialization
****************************************************************************//**
*
* \brief Dedicated to initialize ETH PHY
* Configures the PHY with 10-Mbps link speed, full-duplex communication, and auto negotiation off
*
* \Note: Implementation of PHY callbacks called from this function will differ based on the Ethernet PHY hardware used.
*
*******************************************************************************/
static void cy_eth_phy_initialization (cy_ecm_interface_t eth_idx, ETH_Type *reg_base,
                                       cy_ecm_phy_config_t *ecm_phy_config,
                                       cy_ecm_phy_callbacks_t *phy_callbacks)
{
    cy_en_ethif_speed_sel_t speed_sel;
    uint32_t                duplex = 0, phy_speed = 0, neg_status = 0;
    cy_en_ethif_status_t    eth_status;
    cy_rslt_t               result = CY_RSLT_SUCCESS;

    /* Driver configuration is already done */
    if(is_driver_configured == true)
    {
        /* Initialize the PHY */
        (void)phy_callbacks->phy_init((uint8_t)eth_idx, reg_base);

        /* If driver already configured and the auto negotiation is enabled, replace the speed and mode by the auto negotiated values decided during driver initialization */
        if(ecm_phy_config->mode == CY_ECM_DUPLEX_AUTO || ecm_phy_config->phy_speed == CY_ECM_PHY_SPEED_AUTO)
        {
            result = phy_callbacks->phy_get_linkspeed((uint8_t)eth_idx, &duplex, &phy_speed);
            if(result == CY_RSLT_SUCCESS)
            {
                ecm_phy_config->phy_speed = (cy_ecm_phy_speed_t)phy_speed;
                ecm_phy_config->mode      = (cy_ecm_duplex_t)duplex;
            }
        }
    }

    if(!is_driver_configured)
    {
        /* Auto Negotiation enable */
        if(ecm_phy_config->phy_speed == CY_ECM_PHY_SPEED_AUTO || ecm_phy_config->mode == CY_ECM_DUPLEX_AUTO)
        {
            eth_status = Cy_ETHIF_MdioInit(reg_base, &stcENETConfig);
            if (CY_ETHIF_SUCCESS != eth_status)
            {
                cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Ethernet MAC Pre-Init failed with ethStatus=0x%X \n", eth_status );
                return;
            }
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Ethernet MAC Pre-Init success \n" );

            /* Initialize the PHY */
            (void)phy_callbacks->phy_init((uint8_t)eth_idx, reg_base);

            /* Start auto negotiation */
            duplex = (uint32_t)CY_ECM_DUPLEX_AUTO;
            phy_speed = (uint32_t)CY_ECM_PHY_SPEED_AUTO;

            /* Configure PHY */
            (void)phy_callbacks->phy_configure((uint8_t)eth_idx, duplex, phy_speed);

            /* Required some delay to get PHY back to Run state */
            cy_rtos_delay_milliseconds(100);

            do
            {
                cy_rtos_delay_milliseconds(100);
                result = phy_callbacks->phy_get_auto_neg_status((uint8_t)eth_idx, &neg_status);
                if(result != CY_RSLT_SUCCESS)
                {
                    break;
                }
            } while(neg_status == 0);

            result = phy_callbacks->phy_get_link_partner_cap((uint8_t)eth_idx, &duplex, &phy_speed);
            if(result == CY_RSLT_SUCCESS)
            {
                ecm_phy_config->phy_speed = (cy_ecm_phy_speed_t)phy_speed;
                ecm_phy_config->mode = (cy_ecm_duplex_t)duplex;
            }
        }

        speed_sel = ecm_config_to_speed_sel(ecm_phy_config);

        /* Update the configuration based on user input */
        eth_clock_config(eth_idx, speed_sel, ecm_phy_config->phy_speed);

        /** Initialize ENET MAC */
        eth_status = Cy_ETHIF_Init(reg_base, &stcENETConfig, &stcInterruptConfig);
        if (CY_ETHIF_SUCCESS != eth_status)
        {
            cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Ethernet MAC Init failed with ethStatus=0x%X \n", eth_status );
            return;
        }
        if(!(ecm_phy_config->phy_speed == CY_ECM_PHY_SPEED_AUTO || ecm_phy_config->mode == CY_ECM_DUPLEX_AUTO))
        {
            /* Initialize the PHY */
            (void)phy_callbacks->phy_init((uint8_t)eth_idx, reg_base);
        }
        is_driver_configured = true;
    }
    cy_ecm_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Register driver callbacks  \n" );
    stcInterruptCB.rxframecb  = cy_process_ethernet_data_cb;

    /* Reset the PHY */
    (void)phy_callbacks->phy_reset((uint8_t)eth_idx, reg_base);

    /* Discover */
    (void)phy_callbacks->phy_discover((uint8_t)eth_idx);

    duplex = ecm_phy_config->mode;
    phy_speed = ecm_phy_config->phy_speed;

    (void)phy_callbacks->phy_configure((uint8_t)eth_idx, duplex, phy_speed);

    /* Enable PHY extended registers */
    (void)phy_callbacks->phy_enable_ext_reg(reg_base, phy_speed);
}

// EMAC END *******

/* [] END OF FILE */
