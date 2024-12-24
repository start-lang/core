import serial
import time
import sys

def read_serial_data(port, baudrate, code=None):
    ser = serial.Serial(port, baudrate, timeout=0.1)
    recording = False
    buffer = ""
    start_time = None
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    while True:
        c = ser.read(1)
        if c:
            buffer += chr(c[0])
        if not recording and "Rebooting..." in buffer:
            print(buffer)
            exit(1)
        if not recording and "ready\n" in buffer:
            if code:
                ser.write(f"_code_start_{code}_code_end_\n".encode())
                print(f"< sent code {code}")
            else:
                ser.write(b"run\n")
                print("< sent run command")
            recording = True
        elif recording and buffer.endswith("start\n"):
            buffer = ""
            print("> start of data")
            start_time = time.time()
        if recording and buffer.endswith("end\n"):
            end_time = time.time()
            print("> end of data")
            break

    ser.close()
    duration = (end_time - start_time) * 1000

    start_index = buffer.find("start\n") + len("start\n")
    done_index = buffer.find("end\n")
    captured_data = buffer[start_index:done_index].strip()

    return captured_data, duration


if __name__ == "__main__":
    port = sys.argv[1]
    baudrate = int(sys.argv[2])
    code = None
    if len(sys.argv) > 3:
        code = sys.argv[3]
    data, duration = read_serial_data(port, baudrate, code)
    print(data)
    print(f"{duration} ms")
