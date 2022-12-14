# Ethernet Connection Manager (ECM)

## Overview

ECM is a library which helps application developers to manage ethernet connectivity. This library provides a set of APIs that can be used to establish and monitor ethernet connections on Infineon platforms that support ethernet connectivity.

The library APIs are thread-safe. The library monitors the ethernet connection and notifies connection state changes through an event notification mechanism.

## Features and functionality

- Supports reduced gigabit media-independent interface (RGMII) mode

- Supports up to four source and destination address filtering

- Supports for enabling and disabling of receiving the broadcast messages

- Supports for enable/disable of promiscuous mode

- Exposes APIs to configure, connect, and disconnect from the network

- Connection monitoring: Monitors active connections and link events. Notifies the connection state change through event notification registration mechanism.

## Supported platforms

This library and its features are supported on the following Infineon platforms:

- [XMC7200D-E272K8384 kit (KIT-XMC72-EVK)](https://www.infineon.com/KIT_XMC72_EVK)

## Log messages

By default, the Ethernet Connection Manager library disables all debug log messages. Do the following to enable log messages:

1. Add the `ENABLE_ECM_LOGS` macro to the `DEFINES` in the code example's Makefile. The Makefile entry should look like as follows:
   ```
   DEFINES+=ENABLE_ECM_LOGS
   ```
2. Call the `cy_log_init()` function provided by the *cy-log* module. cy-log is part of the *connectivity-utilities* library. See [connectivity-utilities library API documentation](https://infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for cy-log details.

## Dependencies

The Ethernet Connection Manager library depends on the following:

- [lwIP Network Interface Integration](https://github.com/Infineon/lwip-network-interface-integration)

- [Freertos](https://github.com/Infineon/Freertos)

- [lwIP FreeRTOS Integration](https://github.com/Infineon/lwip-freertos-integration)

## Additional information

- [Ethernet Connection Manager RELEASE.md](./RELEASE.md)

- [Ethernet Connection Manager API documentation](https://Infineon.github.io/ethernet-connection-manager/api_reference_manual/html/index.html)

- [Connectivity Utilities API documentation - for cy-log details](https://Infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html)

- [ModusToolbox&trade; software environment, quick start guide, documentation, and videos](https://www.cypress.com/products/modustoolbox-software-environment)

- [Ethernet Connection Manager version](./version.xml)

- [ModusToolbox&trade; cloud connectivity code examples](https://github.com/Infineon?q=mtb-example-anycloud%20NOT%20Deprecated)
