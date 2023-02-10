# CurrentMonitor
 London Water Co-op Current Monitor

The Current Monitor (Monitor hereafter) along with the Current Recorder started out as a leak detection mechanism for London Water Co-op (LWC). As the name suggests, it monitors current flow for the pressure pump that supplies drinking water to the LWC connections. If the pump runs for too long or too frequently, a leak can be assumed to exist. The Monitor communicates with the Current Recorder via LoRa. 

Before its implementation, the only way to detect a leak was to turn on a faucet. If there's no water, you've got a leak. And, while that technique still works, it is far better to catch it early enough to do something before all of the water is gone.

Today, in addition to its original function, the Monitor uses the WiFi network at the WTP to query tank level and turbidity from the PLC as well as to communicate with the Pump Controller for chlorine readings and pump settings which are realyed via LoRa to the Current Recorder. Also, the Current Recorder can send pump settings to the Monitor via LoRa, which are realyed to the Pump Controller over WiFi.

The Monitor is built around a Heltec ESP32 LoRa V2 microcontroller which is mounted on stripboard and enclosed in a modified version of the plastic box that Heltec ships the microcontroller in. The current is monitored via a split core current transformer attached to one of the legs supplying electricity to the pressure pump. Output of the transformer is rectified on the circuit board and routed to the gate of an FET whos source and drain are hooked to ground and a GPIO pin on the ESP32 respectively.

## Configuration

The Monitor is configured via a webpage:

![Current Monitor Config](/assets/CurrentMonitorConfig.png)

The webpage can be accessed through the local WiFi network if the Monitor has joined the network. If the monitor has joined the local WiFi network, the DNS name for the Monitor will be its SSID with ".local" appended. (For example: CurrentMonitor.local.) If the Monitor is not connected to the local WiFi network, one can join the Monitor's SSID, in which case the IP address is 192.168.4.1.

The configuration fields are as follows:

- SSID to Join: is the SSID for the local WiFi network at the WTP.
- Password for SSID to join: the password for the local WiFi network at the WTP.
- SSID for Captive Net: SSID for the WiFi access point (AP) that the Monitor creates. The purpose of this AP is to perform initial configuration or if the local WiFi network is unavailable or has changed. By appending .local, this name also becomes the local DNS name for accessing the Monitor.
- Password for Captive Net SSID: Leaving blank will make the captive AP open.
- Spreading Factor (7-12): This is the LoRa spreading factor. It should match the value configured for the Current Recorder and LWC Monitor instances. For more information on LoRa spreading factor, ask Google.
- Modbus Server IP: This is the IP address of the PLC server in the WTP.
- Frequency to check tank level (in seconds): Sets the interval between queries for both tank level and turbidity. Initially, turbidity queries will occur half of this period after the first tank level query. For example: if set to 60, the first tank query will happen after 60 seconds and the first turbidity query will happen at 90 seconds. The second tank query at 120 and turbidity at 150, etc. However, when the pressure pump goes from on to off, the next tank query will happen five seconds later and then every 60 seconds until the next on to off transition of the pump. The turbidity cadence will not change.

Pressing the submit button will save the changes but not all of the changes will have an effect until the Monitor is rebooted. A change to the Frequency to check tank level parameter will take effect after the next tank and turbidity qureies occur. Also, a change to the Modbus Server IP parameter will take effect when a new connection is made to the PLC. (The Monitor will use the same TCP/IP connection to the PLC for as long as it is working. If it fails or if the connection had never been established in the first place, the new value for Modbus server IP will be used.)

## Construction

### Hardware

While it is unlikely that LWC will ever have to build another Monitor from scratch, these instructions are given here in case someone wants a better understanding of how it works or if another small water system (or any other entity that whants to remotely track something that draws current) wants to use the design. The Monitor was designed with a very specific task in mind; however, it is a general purpose current detector with significant connectivity capabilities including LoRa, WiFi, Bluetooth and Serial.

The TinyCAD schematic for the circuit card is in the /hardware directory as is the VeeCAD source for the strip board and a bill of materials. The LWC version is enclosed in the plastic box that the Heltec ESP32 LoRa V2 microcontroller was delivered in. The box has been modified by making holes to accommodate the antenna, current source and USB connector. The stock antenna has been replaced with a high gain antenna for better performance, but this is strictly optional (the stock antennas that Heltec ships work quite well). The dimentions of the strip board are dictated by the size of the enclosure, so if another enclosure is to be used, there is latitude in that regard.

The header pins for the ESP32 come unsoldered and the only pins needed in the design are on the corners of the device, so only three pins are used in each corner and correspondingly the sockets on the stripboard are only at the corners. If the plastic Heltec box is going to be used as the enclosure, it will not close unless those pins are modified by removing the plastic and trimming the ends of the pins so that the ESP32 sits all of the way down on the sockets. (Sorry, I have no picture of that; however, the same technique is used in the AsyncLWCMonitor repository which does have pictures.)

If the meaning of any of the foregoing is not perfectly obvious, please seek assistance from an IoT geek.

### Firmware

There are instructions for setting up the Arduino IDE in the london-water-co-op repository on GitHub. Once the IDE is set up, the firmware can be built by setting the board to the WiFi LoRa 32(V2) from under Heltec ESP32 Arduino.

![WiFi LoRa 32(V2)](/assets/WiFiLora32V2.png)

Again, if any of the foregoing is not perfectly obvious, seek assistance from an IoT geek.
