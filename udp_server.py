import socket
import time
import cv2
import numpy as np

# Server configuration
UDP_IP = "0.0.0.0"
UDP_PORT = 5000
BUFFER_SIZE = 512
EXPECTED_SIZE = 96 * 96 * 2  # For YUV422 format

print("[UDP Server] Starting...")
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_socket.bind((UDP_IP, UDP_PORT))
udp_socket.settimeout(3)  # timeout for recvfrom

print(f"[UDP Server] Listening on {UDP_IP}:{UDP_PORT}")

image_data = b""
start_time = time.time()
last_packet_time = time.time()

try:
    while True:
        try:
            data, addr = udp_socket.recvfrom(BUFFER_SIZE)
            image_data += data
            last_packet_time = time.time()

            print(f"[UDP Server] Received {len(data)} bytes from {addr} (Total: {len(image_data)}/{EXPECTED_SIZE})")

            if len(image_data) >= EXPECTED_SIZE:
                print("[UDP Server] Received full image")
                break

        except socket.timeout:
            # Check if we already received something and just stopped receiving more
            if len(image_data) > 0 and (time.time() - last_packet_time > 1):
                print("[UDP Server] Timeout reached, assuming image complete")
                break
            else:
                continue

except KeyboardInterrupt:
    print("Interrupted.")

elapsed_time = time.time() - start_time
print(f"Time taken: {elapsed_time:.2f} seconds")
print(f"Total bytes received: {len(image_data)}")

# Convert and save image
if len(image_data) == EXPECTED_SIZE:
    yuv = np.frombuffer(image_data, dtype=np.uint8).reshape((96, 96, 2))
    bgr = cv2.cvtColor(yuv, cv2.COLOR_YUV2BGR_YUYV)
    success, jpg = cv2.imencode('.jpg', bgr)
    if success:
        with open("received_udp_image.jpg", "wb") as f:
            f.write(jpg)
        print("[UDP Server] Image saved as 'received_udp_image.jpg'")
    else:
        print("[UDP Server] Failed to encode JPEG")
else:
    print("[UDP Server] Incomplete image received")

# Send reply to ESP32
if addr:
    response = b"Image received successfully"
    udp_socket.sendto(response, addr)
    print(f"[UDP Server] Sent confirmation to {addr}")

udp_socket.close()
print("[UDP Server] Done.")
