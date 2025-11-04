#include <Arduino.h>
#include <RadioLib.h>
#include <STM32RTC.h>

// --- RADIO / LoRaWAN setup ---

// Use the internal STM32WL radio (inside LoRa‑E5 mini)
STM32WLx radio = new STM32WLx_Module();

// LoRaWAN node using EU868 band
LoRaWANNode node(&radio, &EU868);

// RF switch pins (example: PA4, PA5); fill rest with NC
const uint32_t rfPins[] = { PA4, PA5, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC };

// RF switch table mapping modes to pin states
const Module::RfSwitchMode_t rfTable[] = {
  { STM32WLx::MODE_IDLE,  { LOW,  LOW } },
  { STM32WLx::MODE_RX,    { HIGH, LOW } },
  { STM32WLx::MODE_TX_HP, { LOW,  HIGH } },
  END_OF_MODE_TABLE
};

// Transmission interval
static const unsigned long TX_INTERVAL = 60000;  // 60 seconds
unsigned long last_tx = 0;

// RTC instance
STM32RTC& rtc = STM32RTC::getInstance();

// OTAA credentials — **replace these with your actual keys**
uint64_t joinEUI = 0x0000000000000000ULL;
uint64_t devEUI  = 0x0000000000000000ULL;
uint8_t nwkKey[16] = { 0 };
uint8_t appKey[16] = { 0 };

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  Serial.println("LoRa‑E5 mini + RadioLib v7.x example");

  // Set RF switch table before initializing radio
  radio.setRfSwitchTable(rfPins, rfTable);

  // Initialize the radio
  int16_t err = radio.begin();
  if (err != RADIOLIB_ERR_NONE) {
    Serial.print("radio.begin() failed, code: ");
    Serial.println(err);
    while (true) {
      delay(1000);
    }
  }
  Serial.println("Radio initialized successfully");

  // Initialize RTC for timestamp
  rtc.begin();
  rtc.setTime(15, 30, 58);
  rtc.setDate(4, 7, 23);

  // Join via OTAA
  Serial.println("Starting OTAA join...");
  err = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (err != RADIOLIB_ERR_NONE) {
    Serial.print("OTAA join setup failed, code: ");
    Serial.println(err);
    while (true) {
      delay(1000);
    }
  }
  Serial.println("OTAA credentials set");
}

void sendPacket() {
  // Prepare payload with RTC timestamp
  char payload[80];
  snprintf(payload, sizeof(payload),
           "%02d/%02d/%04d - %02d:%02d:%02d",
           rtc.getMonth(), rtc.getDay(), 2000 + rtc.getYear(),
           rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());

  Serial.print("Sending uplink: ");
  Serial.println(payload);

  // Use sendReceive: send uplink and wait for downlink
  // Signature: sendReceive(dataUp, lenUp, fPort, dataDown, lenDownPtr, isConfirmed)
  uint8_t downBuf[64];
  size_t downLen = sizeof(downBuf);
  int16_t ret = node.sendReceive((uint8_t*)payload, strlen(payload),
                                 10,         // FPort = 10
                                 downBuf, &downLen,
                                 false);     // unconfirmed uplink

  if (ret == RADIOLIB_ERR_NONE) {
    Serial.println("sendReceive succeeded");
    if (downLen > 0) {
      Serial.print("Downlink received (");
      Serial.print(downLen);
      Serial.println(" bytes):");
      for (size_t i = 0; i < downLen; i++) {
        Serial.print(downBuf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    } else {
      Serial.println("No downlink received");
    }
  } else {
    Serial.print("sendReceive failed, error: ");
    Serial.println(ret);
  }
}

void loop() {
  // Send at the defined interval
  if (!last_tx || millis() - last_tx >= TX_INTERVAL) {
    sendPacket();
    last_tx = millis();
  }

  // You may insert low-power sleep or other code here
  delay(10);
}

