# Ethernet Connection Manager (ECM)

## What's included?

See the [README.md](./README.md) for a complete description of the [ethernet connection manager](https://github.com/Infineon/ethernet-connection-manager) library.

## Known issues
| Problem | Workaround |
| ------- | ---------- |
| IAR 9.30 toolchain throws build errors on Debug mode, if application explicitly includes iar_dlmalloc.h file | Add '--advance-heap' to LDFLAGS in application Makefile. |

## Changelog

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
| ModusToolbox&trade; software environment                   | 3.1     |
| ModusToolbox&trade; Device Configurator                  | 4.10    |
| GCC Compiler                                               | 11.3.1  |
| IAR Compiler                           | 9.30    |
| Arm&reg; Compiler 6                                             | 6.16    |
