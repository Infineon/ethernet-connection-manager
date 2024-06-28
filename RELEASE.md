# Ethernet Connection Manager (ECM)

## What's included?

See the [README.md](./README.md) for a complete description of the [ethernet connection manager](https://github.com/Infineon/ethernet-connection-manager) library.

## Known issues
| Problem | Workaround |
| ------- | ---------- |
| IAR 9.40 toolchain throws build errors on Debug mode, if application explicitly includes iar_dlmalloc.h file | Add '--advance-heap' to LDFLAGS in application Makefile. |
| Sometimes ECM API calls can hang | Disable tickless idle mode in FreeRTOSConfig.h file by defining configUSE_TICKLESS_IDLE to 0. |
| With 100 Mbits/sec speed there are intermittent connection issue observed with certain networks(enterprise) and also issues observed while receiving data larger than 4k buffer size. | Change PHY speed to 1000 Mbits/sec or configure Autonegotiation. |

## Changelog

### v2.0.0

- Removed hardcoded Ethernet PHY operations from ECM and restructured to support multiple Ethernet PHY chips.
- General bug fixes

### v1.0.1

- Updated .cyignore

### v1.0.0

- Initial release for Ethernet Connection Manager library
- Exposes ethernet APIs to configure, connect, and disconnect from the network
- Includes support for source and destination address filtering
- Includes support for enable/disable of broadcast message and promiscuous mode
- Connection monitoring: Monitor active connections and link events. Provides a mechanism to register for event notification.

### Supported software and tools

This version of the library was validated for compatibility with the following software and tools:

| Software and tools                                         | Version |
| :---                                                       | :----:  |
| ModusToolbox&trade; software environment                   | 3.2     |
| ModusToolbox&trade; Device Configurator                    | 4.20    |
| GCC Compiler                                               | 11.3.1  |
| IAR Compiler                                               | 9.40    |
| Arm&reg; Compiler 6                                        | 6.16    |
