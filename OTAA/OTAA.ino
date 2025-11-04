#include <Arduino.h>
#include <RadioLib.h>
#include <STM32RTC.h>

// --- RADIO / LoRaWAN setup ---

// Use the internal STM32WL radio (inside LoRa‑E5 mini)
STM32WLx radio = new STM32WLx_Module();

// LoRaWAN node using EU868 band
LoRaWANNode node(&radio, &EU868);

// RF switch pins for LoRa-E5 mini
const uint32_t rfPins[] = { PA4, PA5, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC };

// RF switch table mapping modes to pin states
// Corrected for LoRa-E5 mini: RX on PA4=HIGH,PA5=LOW  TX on PA4=LOW,PA5=HIGH
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
uint8_t nwkKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Track join status
bool isJoined = false;

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

  // Set TCXO voltage (required for STM32WL)
  // Typically 1.7V for most boards, check your board's documentation
  err = radio.setTCXO(1.7);
  if (err != RADIOLIB_ERR_NONE) {
    Serial.print("setTCXO() failed, code: ");
    Serial.println(err);
  }

  // Initialize RTC for timestamp
  rtc.begin();
  rtc.setTime(15, 30, 58);       // Hours, Minutes, Seconds
  rtc.setDate(4, 11, 24);        // Day, Month, Year (Nov 4, 2024)

  // Setup OTAA credentials
  Serial.println("Setting up OTAA credentials...");
  err = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (err != RADIOLIB_ERR_NONE) {
    Serial.print("OTAA setup failed, code: ");
    Serial.println(err);
    while (true) {
      delay(1000);
    }
  }
  Serial.println("OTAA credentials configured");

  // Actually join the network - THIS IS THE MISSING STEP!
  Serial.println("Attempting to join LoRaWAN network...");
  Serial.println("This may take 10-30 seconds...");
  err = node.activateOTAA();
  if (err != RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.print("OTAA join failed, code: ");
    Serial.println(err);
    Serial.println("Check your credentials and gateway coverage");
    while (true) {
      delay(1000);
    }
  }
  
  isJoined = true;
  Serial.println("Successfully joined LoRaWAN network!");
  Serial.println("Starting periodic transmissions...");
}

void sendPacket() {
  // Verify we're still joined
  if (!isJoined) {
    Serial.println("Not joined to network!");
    return;
  }

  // Prepare payload with RTC timestamp
  char payload[32];
  snprintf(payload, sizeof(payload),
           "%02d/%02d/%04d - %02d:%02d:%02d",
           rtc.getMonth(), rtc.getDay(), 2000 + rtc.getYear(),
           rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());

  Serial.println("----------------------------------------");
  Serial.print("Sending uplink: ");
  Serial.println(payload);

  // Use sendReceive: send uplink and wait for downlink
  uint8_t downBuf[64];
  size_t downLen = sizeof(downBuf);
  int16_t ret = node.sendReceive((uint8_t*)payload, strlen(payload),
                                 10,         // FPort = 10
                                 downBuf, &downLen,
                                 false);     // unconfirmed uplink

  if (ret == RADIOLIB_ERR_NONE) {
    Serial.println("✓ sendReceive succeeded");
    if (downLen > 0) {
      Serial.print("Downlink received (");
      Serial.print(downLen);
      Serial.println(" bytes):");
      Serial.print("  Data: ");
      for (size_t i = 0; i < downLen; i++) {
        if (downBuf[i] < 0x10) Serial.print("0");
        Serial.print(downBuf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    } else {
      Serial.println("No downlink received");
    }
  } else {
    Serial.print("✗ sendReceive failed, error: ");
    Serial.println(ret);
    
    // Handle specific errors
    if (ret == RADIOLIB_ERR_NETWORK_NOT_JOINED) {
      Serial.println("Device not joined to network!");
      isJoined = false;
    }
  }
  Serial.println("----------------------------------------");
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