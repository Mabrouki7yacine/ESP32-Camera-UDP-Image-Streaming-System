This project implements a simple yet efficient UDP-based image streaming system using an ESP32-CAM module and a Python server. The ESP32 captures frames from its onboard camera and streams raw YUV422 image data in chunks over UDP. A Python script on the host receives, reconstructs, decodes, and saves the image, providing a lightweight wireless vision system.

Real-time image streaming over UDP
Raw YUV422 camera data transmission
Automatic frame reassembly and conversion to JPEG
ESP32-based camera capture using esp_camera
Simple confirmation reply from server to client
Works with local Wi-Fi networks

 Tech Stack
ESP-IDF (C for ESP32)
OpenCV + NumPy (Python)
UDP Sockets (no handshake, fast transmission)

Use Cases
Wireless camera monitoring
IoT-based visual data transmission
Lightweight remote sensing with minimal bandwidth
Robotics vision integration
