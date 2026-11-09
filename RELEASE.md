# Ethernet Connection Manager (ECM)

## What's included?

See the [README.md](./README.md) for a complete description of the [ethernet connection manager](https://github.com/Infineon/ethernet-connection-manager) library.

## Known issues
| Problem | Workaround |
| :--- | :--- |
| IAR 9.40 toolchain throws build errors on Debug mode, if application explicitly includes iar_dlmalloc.h file | Add '--advance-heap' to LDFLAGS in application Makefile. |
| Sometimes ECM API calls can hang | Disable tickless idle mode in FreeRTOSConfig.h file by defining configUSE_TICKLESS_IDLE to 0. |
| With 100 Mbits/sec speed there are intermittent connection issue observed with certain networks(enterprise) and also issues observed while receiving data larger than 4k buffer size. | Change PHY speed to 1000 Mbits/sec or configure Autonegotiation. |

## Changelog

### v2.3.0

- Documentation updates to make documentation AI-friendly, minor fixes for the assets to make them Hobsification-compatible and minor code update.
- Updated callback handling for RxError.
- Prevented switching DP83848VV and LAN8720A to fixed speed/duplex when auto-negotiation is requested, to support Auto-MDIX per the DP83848 datasheet and LAN8720A errata.
- Updated Ethernet reference-clock dividers for TRAVEO&trade; T2G CYT4DN devices on KIT_T2G_C-2D-6M_LITE with the RTL8211FI PHY to support 10/100/1000 Mbps operation from a 125-MHz reference clock.
- Documented support for TRAVEO&trade; T2G CYT3DL on KIT_T2G_C-2D-4M_LITE using external DP83848VV or LAN8720A PHY modules connected to Ethernet 0 (eth0) over RMII, with a stable 50-MHz reference clock for 10/100 Mbps operation.
- Documented support for TRAVEO&trade; T2G CYT4DN on KIT_T2G_C-2D-6M_LITE using the RTL8211FI PHY connected to Ethernet 0 (eth0) over RGMII, with a 125-MHz reference clock for 10/100/1000 Mbps operation.
- Documented support for TRAVEO&trade; T2G CYT6BJ on KIT_T2G_B-H-16M_LITE with the LAN8670 10BASE-T1S PHY connected to Ethernet 0 (eth0) over RMII, including basic Device Configurator setup and PLCA network requirements.

### v2.2.0

- Added support for PSOC&trade; Edge E84 (PSE84) platform.
- Enabled LLVM_ARM toolchain support.
- Updated to use MTB_HAL apis.

### v2.1.1

- Added support for D-cache enablement on XMC7200 devices.

### v2.1.0

- Added Network Interface settings, IP settings, Protocol settings, Memory settings and PHY pin configuration settings in the ModusToolbox&trade; Device Configurator.

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
| :---                                                       | :---  |
| ModusToolbox&trade; software environment                   | 3.8     |
| ModusToolbox&trade; Device Configurator                    | 5.50    |
| GCC Compiler                                               | 14.2.1  |
| IAR Compiler                                               | 9.50.2  |
| Arm&reg; Compiler 6                                        | 6.22    |
| LLVM compiler                                              | 19.1.5  |

---
© Infineon Technologies AG or an affiliate of Infineon Technologies AG, 2026.
