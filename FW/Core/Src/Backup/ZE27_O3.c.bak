/*
 * ZE27_O3.c

 */

#include "ZE27_O3.h"


uint8_t ZE27_Checksum(uint8_t *msg, unsigned char ln){
    uint8_t sum = 0;

  //Checksum = (NOT(Byte1+Byte2+Byte3+Byte4+Byte5+Byte6+Byte7)) +1
    for (uint8_t i = 1; i <= ln - 2; i++) {
        sum += msg[i];
    }

    uint8_t checksum = (~sum)+1;
    return checksum;
}

uint16_t ZE27_parsePPB(uint8_t *msg){
	if(msg[ZE27_RX_BUFFER_SIZE -1] == ZE27_Checksum(msg, ZE27_RX_BUFFER_SIZE)){
		return msg[4]*256 + msg[5];
	}
 return 0;
}


