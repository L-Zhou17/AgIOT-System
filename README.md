# AgIOT-System

### Requirements:
#### Hardware:
1. ESP32 module
2. SHT20
3. BH1750

#### Tools:
1. Arduino IDE
2. Arduino core for the ESP32, available at https://github.com/me-no-dev
3. Libraries: AsyncTCP and ESPAsyncWebServer, available at https://github.com/me-no-dev

#### Instructions
1. Install Arduino IDE.
2. Install library dependencies for ESP32 (see the Requirements).
3. Choose the correct ESP32 mudule in the IDE.
4. Upload the codes to the module.

#### Functions:
- The node works in AP-STA mode.
- A local server is established on the node. First, connect to the access point using the SSID and password defined in the code. Next, access 192.168.4.1 with browser to visit the GUI. The informantion including temperature, humidity and illumination are listed on the main page. Then, users can change the configurations of the node, such as the target accesss point (SSID and password) and the target remote server (IP address and port). Once the changes are committed, the node will force restart and work according to the configuration. 
- The node uploads the collected data to the remote server using a certain format $ID,Name,User,Type,Frequency,Battery,data1,data2,data_n,#
