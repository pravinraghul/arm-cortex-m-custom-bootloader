# Host based bootloader software design

## Design description

The software, which runs on a PC, directs the bootloader through various states
required to flash a new image onto the system. However, it is unable to manage
the bootloader process independently.

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

The host tool to flash the bin into the microcontroller using Simple Bootloader
Protocol(SBP).

> python3 Utils/boot_tool.py <app_binary_path> <app_version>
