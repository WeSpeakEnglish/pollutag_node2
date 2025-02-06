![the Node2](https://github.com/WeSpeakEnglish/pollutag_node2/blob/main/node2.png)
# Pollutag Node2
The LoraWAN node for air quatity monitoring based on STM32WLE and SEN54 Sensirion sensor
868 Mhz LoRaWAN node for continuous air quality monitoring. The node operates autonomously and support remote commands via downlinks.
SEN54 connected via I2C2 interface.
1. LaraWAN node based on STM32WLE MCU (LORA-E5), easy to integrate monitoring and control node (air quality monitoring, garden, intelligent home. etc.)
2. You may connect load remotely by downlinks - via AO3400A transistor (on board)
3. New Sensirion SEN54 sensor optimized
4. MPPT and Li-Ion battery charger for power from 5V++ cheap solar panels.
5. Sleep current as low as 0.9 mA
6. 10+ years of autonomity with cheap 2W solar panel and standard 1850 Li-Ion battery
7. More than 2 weeks work autonomity from fully charged 3500mA  Li-Ion battery
8. Ideal for sensor.community integration over LoRaWAN IoT, like TTN and Helium, no internet connection from your side needed!
9. Full fledged with routed test points - easy to solder - easy to add functionality (inputs, outputs, interfaces etc.)

## Downlink commands supported
1. Reset the device via sending downlink on Port2: FF FF FF FF FF FF
2. Set transmit period (in milliseconds) via downlink on Port2: 01 00 XX XX XX XX (XX XX XX XX - milliseconds coded in hex (big endian), ex. for 5 minutes = 300,000 ms = 00 04 93 E0)
3. Set measuring period (in minutes) via downlink as for now provided with LED_ON command in one 2-bytes downlink message on Port2: 01 XX - LED ON and  measuring period = XX minutes, 00 XX - LED is OFF and  measuring period = XX minutes
   This LED enchanced with FET transictor, so you may connect some load to be switched ON/OFF (see the schematic in PCB folder)  
         
The node uplink data packet containing 13 payload bytes with following data - PM1 concenthation, PM2.5 concentration, PM10 concentration, temperature,
humidity, volatile organic copounds index (VOC), battery level related to volts, status of LED output. 
Decoder for payload on server side (uplink):
```js
 function Decoder(bytes, port, uplink_info) {
    var decoded = {};
    decoded.PM1 = ((bytes[0]*256 +  bytes[1])/10.0).toFixed(2);
    decoded.PM2_5 = ((bytes[2]*256 +  bytes[3])/10.0).toFixed(2);
    decoded.PM10 = ((bytes[4]*256 +  bytes[5])/10.0).toFixed(2);
    decoded.T = (-100.0 + bytes[6] +  bytes[7]/100.0).toFixed(2);
    decoded.RH = (bytes[8] +  bytes[9]/100).toFixed(2);
    decoded.VOC = ((bytes[10]*256 + bytes[11])/10.0).toFixed(2);
    decoded.Battery = (bytes[12]*0.0238).toFixed(2);//battery , volts
    decoded.LEDON = bytes[13];
    decoded.RSSI = normalizedPayload.gateways[0].rssi.toFixed(0);
    decoded.SNR = normalizedPayload.gateways[0].snr.toFixed(0);
    decoded.FREQUENCY = normalizedPayload.frequency;
    return decoded;
 }
```
Downlink payload encoder example:
```js
function Encoder(measurements, port) {
    if(measurements.LED_ON.value) return [1];
    else return [0];
}
```
Before compile please edit comissioning in CubeMX (.ioc). Network ID may be left 0, if you are connecting to any first public network avaliable (may be useful if you planning to transfer to a different LoRaWAN network lately, then you may set the nodes commisionings on other's network console, reset the node and delete commisionings on old networks console, following the second reset of the node)
![to compile please edit comissioning first](https://github.com/WeSpeakEnglish/pollutag_node2/blob/main/commissioning.png)

Hackaday project: [LINK](https://hackaday.io/project/198348-pollutagnode2-air-quality-monitor-lorawan)

OSHWLab STD project: [LINK](https://oshwlab.com/pinelab/pollutagnode2-lorawan-node-air-quality-monitor)

OSHWLab PRO poject: [LINK](https://oshwlab.com/pinelab/pollutag2)

Pollutants.eu: [LINK](https://pollutants.eu/index.php/component/search/?searchword=pollutag&searchphrase=all&Itemid=121)

