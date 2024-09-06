# Serial Custom Bootloader

## Design description

The goal of this serial custom bootloader project is to enhance understanding of bootloader software for microcontrollers, particularly those based on ARM Cortex-M. It involves developing simple custom commands and communication protocols to establish a link between the microcontroller and a host PC.

## Host communication

### Simple bootloader protocol
                                                         
#### Packet Format

#### Header packet

|      | **SOF** | **TYPE** | **LEN** |
|------|-----|------|-----|
| **SIZE (bytes)** |  1  |   1  |  2  |
        
#### Data packet

|      |   **DATA**   | **CRC** |
|------|----------|-----|
| **SIZE (bytes)** | 4 - 1024 |  4  |


#### Packet Type
- **START**: to indicate the start of firmware download.
- **STOP**: to indicate the end of firmware download.
- **RESP**: to respond the host on packet reception based on the validation.
- **CONF**: to send the firmware configuration includes (version, size, crc) etc.
- **DATA**: to send the actual firmware binary packets.
                                                       
### SBP host tool

The host tool to flash the bin into the microcontroller using Simple Custom Bootloader
Protocol(SBP).

> python3 Utils/boot_tool.py <app_binary_path> <app_version>
