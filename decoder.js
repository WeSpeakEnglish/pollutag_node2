 function Decoder(bytes, port, uplink_info) {
    var decoded = {};
    decoded.PM1 = (bytes[0]*256 +  bytes[1])/10.0;
    decoded.PM2_5 = (bytes[2]*256 +  bytes[3])/10.0;
    decoded.PM10 = (bytes[4]*256 +  bytes[5])/10.0;
    decoded.T = -100.0 + bytes[6] +  bytes[7]/100.0 ;
    decoded.TH = bytes[8] +  bytes[9]/100;
    decoded.VOC = (bytes[10]*256 + bytes[11])/10.0;
    decoded.Battery = bytes[12]*(3.0/76);//52.08;//31.06;
    decoded.LEDON = bytes[13];
    decoded.RSSI = normalizedPayload.gateways[0].rssi.toFixed(0);
    decoded.SNR = normalizedPayload.gateways[0].snr.toFixed(0);
    decoded.FREQUENCY = normalizedPayload.frequency;
    return decoded;
 }