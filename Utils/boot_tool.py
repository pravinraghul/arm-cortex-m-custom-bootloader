import sys
import time
import serial
from crccheck.crc import Crc32Mpeg2

SBP_HEADER_SOF_OFFSET = 0
SBP_HEADER_TYPE_OFFSET = 1
SBP_HEADER_LEN_OFFSET = 2
SBP_DATA_OFFSET = 3

SBP_HEADER_SOF = 0x5A

SBP_TYPE_START = 0x01
SBP_TYPE_STOP = 0x23
SBP_TYPE_RESP = 0x45
SBP_TYPE_CONF = 0x67
SBP_TYPE_DATA = 0x89

SBP_CONF_VERSION_OFFSET = 0
SBP_CONF_SIZE_OFFSET = 4
SBP_CONF_CRC_OFFSET = 8

SBP_RESP_ACK = 0x15
SBP_RESP_NACK = 0x16

def read_binfile(filename):
    try:
        with open(filename, 'rb') as binfile:
            bindata = binfile.read()
    except FileNotFoundError:
        print("File not found: {}".format(filename))
        sys.exit(1)

    return bindata

def validate_response(ser):
    data = ser.read(12)

    if data[0] != SBP_HEADER_SOF:
        print("validate_response: response sof failed")
        return False

    if data[1] != SBP_TYPE_RESP:
        print("validate_response: response byte doesn't match")
        return False

    if data[4] == SBP_RESP_NACK:
        print("validate_response: response nack received")
        return False

    print("validate_response: response received OK..!")
    return True

def send_start_packet(ser):
    LENGTH = 4
    length = LENGTH.to_bytes(2, byteorder='little')
    header = [SBP_HEADER_SOF, SBP_TYPE_START]
    header.extend(length)

    data = [0 for i in range(8)]
    ser.write(header)
    time.sleep(0.1)  # Time is required
    ser.write(data)
    print("send_start_packet: start command sent...!")

def send_config_packet(ser, bindata, version):
    LENGTH = 12

    length = LENGTH.to_bytes(2, byteorder='little')
    header = [SBP_HEADER_SOF, SBP_TYPE_CONF]
    header.extend(length)

    data = []
    version.extend([0 for i in range(4 - len(version))])

    size = len(bindata).to_bytes(4, byteorder='little')
    crc = Crc32Mpeg2.calc(bindata).to_bytes(4, byteorder='little')
    print("size: ", len(bindata))
    print("crc: ", bytes(crc).hex())

    data.extend(bytes(version))
    data.extend(size)
    data.extend(crc)

    new_crc = Crc32Mpeg2.calc(bytes(data)).to_bytes(4, byteorder='little')
    data.extend(new_crc)

    ser.write(header)
    time.sleep(0.1)  # Time is required
    ser.write(data)
    print("send_config_packet: config packet sent...!")

def send_data_packet(ser, bindata):
    header = [SBP_HEADER_SOF, SBP_TYPE_DATA]
    start = 0
    end = 1024
    data = bindata[start: end]

    while data:
        length = len(data).to_bytes(2, byteorder='little')
        header_packet = header + list(length)
        crc = Crc32Mpeg2.calc(data).to_bytes(4, byteorder='little')
        data_packet = list(data) + list(crc)

        ser.write(header_packet)
        time.sleep(0.1)
        ser.write(data_packet)

        start = end
        end += 1024
        data = bindata[start: end]
        time.sleep(1)

        if not validate_response(ser):
            return sys.exit(1)
    print("send_data_packet: data packet sent...!")

def send_stop_packet(ser):
    LENGTH = 4
    length = LENGTH.to_bytes(2, byteorder='little')
    header = [SBP_HEADER_SOF, SBP_TYPE_STOP]
    header.extend(length)

    data = [0 for i in range(8)]
    ser.write(header)
    time.sleep(0.1)  # Time is required
    ser.write(data)
    print("send_stop_packet: stop command sent...!")

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 boot_tool.py <app-bin> <app-version(0.0.0)>")

    ser = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=30)
    filepath = sys.argv[1]
    version = [int(x) for x in sys.argv[2].split('.')]
    print(version)

    bindata = read_binfile(filepath)
    send_start_packet(ser)
    if not validate_response(ser):
        return sys.exit(1)

    time.sleep(2)

    send_config_packet(ser, bindata, version)
    if not validate_response(ser):
        return sys.exit(1)

    time.sleep(2)

    send_data_packet(ser, bindata)

    time.sleep(2)

    send_stop_packet(ser)
    if not validate_response(ser):
        return sys.exit(1)


if __name__ == "__main__":
    main()
