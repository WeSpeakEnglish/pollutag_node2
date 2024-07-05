# pollutag_node
The LoraWAN node for air quatity monitoring based on STM32WLE and SEN54 Sensirion sensor
868 Mhz LoRaWAN node for continuous air quality monitoring. The node operates autonomously and support remote commands via downlinks.
SEN54 connected via I2C2 interface.
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
