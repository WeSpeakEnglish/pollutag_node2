# Pollutag Node2
The LoraWAN node for air quatity monitoring based on STM32WLE and SEN54 Sensirion sensor
868 Mhz LoRaWAN node for continuous air quality monitoring. The node operates autonomously and support remote commands via downlinks.
SEN54 connected via I2C2 interface.
1. LaraWAN node based on STM32WLE MCU (LORA-E5), easy to integrate monitoring and control node (air quality monitoring, garden, intelligent home. etc.)
2. You may connect load remotely by downlinks - via AO3400A transistor (on board)
3. New Sensirion SEN54 sensor optimized
4. MPPT and Li-Ion battery charger for power from 5V++ cheap solar panels.
5. Sleep current below 1mA expected

6. 10+ years of autonomity with cheap 2W solar panel and standard 1850 Li-Ion battery

7. More than 2 weeks work autonomity from fully charged 3500mA  Li-Ion battery

8. Ideal for sensor.community integration over LoRaWAN IoT, like TTN and Helium, no internet connection from your side needed!

9. Full fledged with routed test points - easy to solder - easy to add functionality (inputs, outputs, interfaces etc.)

    
The node uplink data packet containing 13 payload bytes with following data - PM1 concenthation, PM2.5 concentration, PM10 concentration, temperature,
humidity, volatile organic copounds index (VOC), battery level related to volts, status of LED output. 
Decoder for payload on server side (uplink):
```js
 function Decoder(bytes, port, uplink_info) {
    var decoded = {};
    decoded.PM1 = (bytes[0]*256 +  bytes[1])/10.0;
    decoded.PM2_5 = (bytes[2]*256 +  bytes[3])/10.0;
    decoded.PM10 = (bytes[4]*256 +  bytes[5])/10.0;
    decoded.T = -100.0 + bytes[6] +  bytes[7]/100.0 ;
    decoded.TH = bytes[8] +  bytes[9]/100;
    decoded.VOC = (bytes[10]*256 + bytes[11])/10.0;
    decoded.Battery = bytes[12]*(3.0/76);// battery, volts
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
