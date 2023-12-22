import serial
import struct
import time

# Constants
SERIAL_PORT = "COM5"  # Update this with your serial port
BAUD_RATE = 115200    # Update this if needed
GET_PARAMETERS_MESSAGE = "GET"
GET_PARAMETERS_ACK = "GET_ACK"

# The struct format string (adjust according to your struct layout)
struct_format = "4B50s4B4H16B"  # Corresponds to the Parameters_t structure

# Open serial port
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Function to mock sending Parameters_t struct
def send_mock_parameters():
    # Mock data for the Parameters_t struct
    mock_data = (
        1, 2, 3, 4,                      # uint8_t x 4
        b"Example Company".ljust(50),    # uint8_t array[50]
        192, 168, 0, 1,                  # uint8_t array[4]
        10, 20, 30, 40,                  # uint16_t x 4
        *range(5, 21)                    # uint8_t x 16
    )
    packed_data = struct.pack(struct_format, *mock_data)
    ser.write(packed_data)

# Main loop
try:
    print("Listening for GET command...")
    while True:
        if ser.in_waiting > 0:
            received = ser.read(ser.in_waiting).decode()
            if GET_PARAMETERS_MESSAGE in received:
                ser.write(GET_PARAMETERS_ACK.encode())
                time.sleep(0.1)  # Give some delay for reliable transmission
                send_mock_parameters()
                print("Sent GET_ACK and mock Parameters_t data")
except KeyboardInterrupt:
    print("Exiting...")

# Close serial port
ser.close()
