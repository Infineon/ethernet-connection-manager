# Ethernet Connection Manager (ECM)

## Overview

ECM is a library which helps application developers to manage ethernet connectivity. This library provides a set of APIs that can be used to establish and monitor ethernet connections on Infineon platforms that support ethernet connectivity.

The library APIs are thread-safe. The library uses the ethernet configurations from Device configurator. The library monitors the ethernet connection and notifies connection state changes through an event notification mechanism.

## Features and functionality

- Supports up to four source and destination address filtering

- Supports for enabling and disabling of receiving the broadcast messages

- Supports for enable/disable of promiscuous mode

- Exposes APIs to connect, and disconnect from the network

- Connection monitoring: Monitors active connections and link events. Notifies the connection state change through event notification registration mechanism.

## Supported platforms

This library and its features are supported on the following Infineon platforms:

- [XMC7200 Evaluation Kit (KIT_XMC72_EVK)](https://www.infineon.com/KIT_XMC72_EVK)

## Quick Start

Follow below steps to enable and configure ethernet,

1. Application should pull [ethernet-core-freertos-lwip-mbedtls](https://github.com/Infineon/ethernet-core-freertos-lwip-mbedtls) library which will internally pull secure-sockets, ethernet-connection-manager, FreeRTOS, lwIP, Mbed TLS and other dependent modules.
To pull ethernet-core-freertos-lwip-mbedtls create the following *.mtb* file in deps folder.
   - *ethernet-core-freertos-lwip-mbedtls.mtb:*
      `https://github.com/Infineon/ethernet-core-freertos-lwip-mbedtls#latest-v2.X#$$ASSET_REPO$$/ethernet-core-freertos-lwip-mbedtls/latest-v2.X`

2. Application should pull [ethernet-phy-driver](https://github.com/Infineon/ethernet-phy-driver) library.
To pull ethernet-phy-driver create the following *.mtb* file in deps folder.
   - *ethernet-phy-driver.mtb:*
      `https://github.com/Infineon/ethernet-phy-driver#latest-v1.X#$$ASSET_REPO$$/ethernet-phy-driver/latest-v1.X`

3. Provide PHY operation callback functions to Ethernet Connection Manager by invoking *cy_ecm_ethif_init* API function. Refer [Ethernet PHY Driver API documentation](https://Infineon.github.io/ethernet-phy-driver/api_reference_manual/html/index.html) for API documentation of *cy_ecm_ethif_init* API function.

4. Ethernet is not a deep sleep enabled IP, hence system should always be in active mode. Open design.modus file and do the following configuration settings in the ModusToolbox&trade; Device Configurator.
    - Switch to the "System" tab.
    - Select Resource->Power.
    - In "RTOS" section, "System Idle Power Mode" should be set to "Active" from the drop-down.
    - Save the configuration to generate the necessary code.

5. To configure the PHY on [XMC7200 Evaluation Kit (KIT_XMC72_EVK)](https://www.infineon.com/KIT_XMC72_EVK), open design.modus file and do the following configuration settings in the ModusToolbox&trade; Device Configurator.
    - Switch to the "Peripherals" tab.
    - Select Communication->"Ethernet 1" for KIT_XMC72_EVK device.
    - In "Ethernet - Parameters" pane, configure interface type "PHY Interconnect" to "RGMII" from the drop-down.
    - Configure PHY driver "PHY device" to "DP83867IR" from the drop-down.
    - Enable/Disable "Autonegotiation" (default is enable).
    - Configure "PHY speed" (10/100/1000 Mbits/sec) based on the device. <br>
    **Note:** "PHY speed" and "PHY Duplex Mode" options are visible only when the "Autonegotiation" is disabled. <br>
    **Note:** With 100 Mbits/sec speed there are intermittent connection issue observed with certain networks(enterprise) and also issues observed while receiving data larger than 4k buffer size. Hence the recommended speed with current release would be 1000 Mbits/sec or Autonegotiation.
    - Configure "Mac Address".
    - Enable "Promiscuous mode" (default is disable).
    - Enable "Accept Broadcast Frames" (default is enable).
    - Configure "Mac Reference Clock" (default is External clk).
    - Configure "Nvic Mux Interrupt number". This will be enabled, if devices have separate Nvic interrupt lines. (default is 3).
    - Configure static IP or use DHCP (default is use DHCP).
	- To configure pin connections, refer "Quick Start" section of [Ethernet PHY Driver readme documentation](https://Infineon.github.io/ethernet-phy-driver/README.md)
    - Save the configuration to generate the necessary code.

6. To configure the PHY on [XMC7100 Evaluation Kit (KIT_XMC71_EVK_LITE_V1)](https://www.infineon.com/KIT_XMC71_EVK_LITE_V1), open design.modus file and do the following configuration settings in the ModusToolbox&trade; Device Configurator.
    - Switch to the "Peripherals" tab.
    - Select Communication->"Ethernet 0" for KIT_XMC71_EVK_LITE_V1 device.
    - In "Ethernet - Parameters" pane, configure interface type "PHY Interconnect" to "RMII" from the drop-down.
    - Configure PHY driver "PHY device" to "DP83825I" from the drop-down.
    - Enable/Disable "Autonegotiation" (default is enable).
    - Configure "PHY speed" (10/100 Mbits/sec) based on the device. <br>
    **Note:** "PHY speed" and "PHY Duplex Mode" options are visible only when the "Autonegotiation" is disabled.
    - Configure "Mac Address".
    - Enable "Promiscuous mode" (default is disable).
    - Enable "Accept Broadcast Frames" (default is enable).
    - Configure "Mac Reference Clock" (default is External clk).
    - Configure "Nvic Mux Interrupt number". This will be enabled, if devices have separate Nvic interrupt lines. (default is 3).
    - Configure static IP or use DHCP (default is use DHCP).
	- To configure pin connections, refer "Quick Start" section of [Ethernet PHY Driver readme documentation](https://Infineon.github.io/ethernet-phy-driver/README.md)
    - Save the configuration to generate the necessary code.

7. By default, the Ethernet Connection Manager library disables all debug log messages. Do the following to enable log messages:
    - Add the `ENABLE_ECM_LOGS` macro to the `DEFINES` in the code example's Makefile. The Makefile entry should look like as follows:
       ```
       DEFINES+=ENABLE_ECM_LOGS
       ```
    - Call the `cy_log_init()` function provided by the *cy-log* module. cy-log is part of the *connectivity-utilities* library. See [connectivity-utilities library API documentation](https://infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for cy-log details.


## Additional information

- [Ethernet Connection Manager RELEASE.md](./RELEASE.md)

- [Ethernet Connection Manager API documentation](https://Infineon.github.io/ethernet-connection-manager/api_reference_manual/html/index.html)

- [Ethernet PHY Driver Readme](https://Infineon.github.io/ethernet-phy-driver/README.md)

- [Ethernet PHY Driver API documentation](https://Infineon.github.io/ethernet-phy-driver/api_reference_manual/html/index.html)

- [Connectivity Utilities API documentation - for cy-log details](https://Infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html)

- [ModusToolbox&trade; software environment, quick start guide, documentation, and videos](https://www.cypress.com/products/modustoolbox-software-environment)

- [Ethernet Connection Manager version](./version.xml)

- [ModusToolbox&trade; code examples]( https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software )
