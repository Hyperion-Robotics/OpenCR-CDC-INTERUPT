import serial, struct, time

# ==== CONFIG ====
port = "COM34"  # Adjust for your system (Windows: "COM3")
baud = 115200
ser = serial.Serial(port, baud, timeout=0.1)

# ==== STRUCT ====
def send_struct(a, b):
    pkt = struct.pack("<ii", a, b)
    data = bytearray()
    data += b'\xAF\xAF\xAF\xAF'
    data.append(len(pkt))
    data.append(9)
    data += pkt

    checksum = 0xAF ^ 0xAF ^ 0xAF ^ 0xAF
    checksum ^= data[4]  # len
    checksum ^= data[5]  # id
    for b in pkt:
        checksum ^= b
    data.append(checksum)

    ser.write(data)

def read_data():
    buf = ser.read(512)
    if buf:
        print("RAW RX:", buf.hex(" ").upper())

    for i in range(len(buf)-6):
        if buf[i:i+4] == b'\xAF\xAF\xAF\xAF':
            msg_len = buf[i+4]
            pkt_id = buf[i+5]
            if i + 6 + msg_len < len(buf):
                payload = buf[i+6:i+6+msg_len]
                checksum = buf[i+6+msg_len]

                # Verify checksum
                chs = 0xAF ^ 0xAF ^ 0xAF ^ 0xAF ^ msg_len ^ pkt_id
                for b in payload:
                    chs ^= b

                if checksum == chs and pkt_id == 9 and len(payload) == 8:
                    a, b = struct.unpack("<ii", payload)
                    print(f"Received struct: A={a}, B={b}")
                    return a, b

    return None


# ==== MAIN ====
time.sleep(2)  # Wait for OpenCR reset

# 🔄 Send initial struct
send_struct(100, 200)

a, b = 1, 2

while True:
    send_struct(a+5, b+5)
    time.sleep(1)
    print(f"Sent updated struct: A={a+5}, B={b+5}")
    pkt = read_data()
    print(f"Sent updated struct: A={a+5}, B={b+5}")
    if pkt:
        a, b = pkt
        print(f"Received updated struct: A={a}, B={b}")
        time.sleep(1)
