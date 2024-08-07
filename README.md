# Ethernet Connection Manager (ECM)

## Overview

ECM is a library which helps application developers to manage ethernet connectivity. This library provides a set of APIs that can be used to establish and monitor ethernet connections on Infineon platforms that support ethernet connectivity.

The library APIs are thread-safe. The library uses the ethernet configurations from Device configurator. The library monitors the ethernet connection and notifies connection state changes through an event notification mechanism.

## Features and functionality

- Supports reduced gigabit media-independent interface (RGMII) mode

- Supports up to four source and destination address filtering

- Supports for enabling and disabling of receiving the broadcast messages

- Supports for enable/disable of promiscuous mode

- Exposes APIs to connect, and disconnect from the network

- Connection monitoring: Monitors active connections and link events. Notifies the connection state change through event notification registration mechanism.

## Supported platforms

This library and its features are supported on the following Infineon platforms:

- [XMC7200D-E272K8384 kit (KIT-XMC72-EVK)](https://www.infineon.com/KIT_XMC72_EVK)

## Quick Start

1. To use ethernet-connection-manager library on FreeRTOS, lwIP, and Mbed TLS combination, the application should pull [ethernet-core-freertos-lwip-mbedtls](https://github.com/Infineon/ethernet-core-freertos-lwip-mbedtls) library which will internally pull secure-sockets, ethernet-connection-manager, FreeRTOS, lwIP, Mbed TLS and other dependent modules.
To pull ethernet-core-freertos-lwip-mbedtls create the following *.mtb* file in deps folder.
   - *ethernet-core-freertos-lwip-mbedtls.mtb:*
      `https://github.com/Infineon/ethernet-core-freertos-lwip-mbedtls#latest-v1.X#$$ASSET_REPO$$/ethernet-core-freertos-lwip-mbedtls/latest-v2.X`

2. On [XMC7200D-E272K8384 kit (KIT-XMC72-EVK)](https://www.infineon.com/KIT_XMC72_EVK) with PHY chip DP83867IR, application should pull [ethernet-phy-driver](https://github.com/Infineon/ethernet-phy-driver)
To pull ethernet-phy-driver create the following *.mtb* file in deps folder.
   - *ethernet-phy-driver.mtb:*
      `https://github.com/Infineon/ethernet-phy-driver#latest-v1.X#$$ASSET_REPO$$/ethernet-phy-driver/latest-v1.X`

3. Provide PHY operation callback functions to Ethernet Connection Manager by invoking *cy_ecm_ethif_init* API function. Refer [Ethernet PHY Driver API documentation](https://Infineon.github.io/ethernet-phy-driver/api_reference_manual/html/index.html) for API documentation of *cy_ecm_ethif_init* API function.

4. To enable ethernet configurations, open design.modus file and do the following configuration settings in the ModusToolbox&trade; Device Configurator.
    - Switch to the "Peripherals" tab.
    - Select Communication->Ethernet.
    - In "Ethernet - Parameters" pane, configure interface type "PHY Interconnect" from the drop-down. 
    - Enable/Disable "Autonegotiation" (default is disable). 
    - Configure "PHY speed" (default 100 Mbits/sec). <br>
    **Note:** With 100 Mbits/sec speed there are intermittent connection issue observed with certain networks(enterprise) and also issues observed while receiving data larger than 4k buffer size. Hence the recommended speed with current release would be 1000 Mbits/sec or Autonegotiation.
    - Configure "PHY Duplex mode" (default Full Duplex).
    - Configure "Mac Address".
    - Enable "Promiscuous mode" (default is disable).
    - Enable "Accept Broadcast Frames" (default is enable).
    - Configure "Mac Reference Clock" (default is External clk).
    - Configure "Nvic Mux Interrupt number". This will be enabled, if devices have separate Nvic interrupt lines. (default is 3).
    - Save the configuration to generate the necessary code.

5. By default, the Ethernet Connection Manager library disables all debug log messages. Do the following to enable log messages:
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

- [ModusToolbox&trade; cloud connectivity code examples](https://github.com/Infineon?q=mtb-example-anycloud%20NOT%20Deprecated)
