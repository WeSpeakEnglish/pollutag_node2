#include <Arduino.h>
#include <RadioLib.h>
#include <STM32RTC.h>
#include <EEPROM.h>

// --- RADIO / LoRaWAN setup ---
STM32WLx radio = new STM32WLx_Module();   // internal STM32WL radio
LoRaWANNode node(&radio, &EU868);

// RF switch pins for LoRa-E5 mini
const uint32_t rfPins[] = { PA4, PA5, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC };

// RF switch table mapping modes to pin states
const Module::RfSwitchMode_t rfTable[] = {
  { STM32WLx::MODE_IDLE,  { LOW,  LOW } },
  { STM32WLx::MODE_RX,    { HIGH, LOW } },
  { STM32WLx::MODE_TX_HP, { LOW,  HIGH } },
  END_OF_MODE_TABLE
};

// RTC instance
STM32RTC& rtc = STM32RTC::getInstance();

// Transmission interval (ms)
static const unsigned long TX_INTERVAL = 60000;
unsigned long last_tx = 0;

// --- LoRaWAN OTAA credentials ---
uint64_t joinEUI = 0x6081F9FDFF1A2E6BULL;
uint64_t devEUI  = 0x6081F9AE72BB6E19ULL;
uint8_t nwkKey[16] = { 0xC1, 0xC8, 0x5F, 0x97, 0x04, 0x6C, 0xF0, 0x78, 0x23, 0x2C, 0x0E, 0xDA, 0x2A, 0x5E, 0x9C, 0xC9 };
uint8_t appKey[16] = { 0xC1, 0xC8, 0x5F, 0x97, 0x04, 0x6C, 0xF0, 0x78, 0x23, 0x2C, 0x0E, 0xDA, 0x2A, 0x5E, 0x9C, 0xC9 };

bool isJoined = false;

// --- EEPROM seed storage ---
#define DEVNONCE_SEED_ADDR 0

uint32_t loadNonceSeed() {
  uint32_t val;
  EEPROM.get(DEVNONCE_SEED_ADDR, val);
  if (val == 0xFFFFFFFF || val == 0) val = 1;
  return val;
}

void saveNonceSeed(uint32_t val) {
  EEPROM.put(DEVNONCE_SEED_ADDR, val);
}

// --- Join function ---
bool joinNetwork() {
  Serial.println("Setting up OTAA credentials...");
  int16_t err = node.beginOTAA(joinEUI, devEUI, NULL, appKey);
  if (err != RADIOLIB_ERR_NONE) {
    Serial.print("OTAA setup failed, code: ");
    Serial.println(err);
    return false;
  }

  // Use a stored random seed to vary DevNonce internally
  uint32_t seed = loadNonceSeed();
  randomSeed(seed + millis());
  saveNonceSeed(seed + 1);

  Serial.print("Nonce seed: ");
  Serial.println(seed);

  Serial.println("Attempting to join LoRaWAN network...");
  err = node.activateOTAA();

  if (err == RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.println("✅ Successfully joined LoRaWAN network!");
    return true;
  } else {
    Serial.print("❌ OTAA join failed, code: ");
    Serial.println(err);
    return false;
  }
}

// --- Send function ---
void sendPacket() {
  if (!isJoined) {
    Serial.println("Not joined — retrying OTAA...");
    isJoined = joinNetwork();
    if (!isJoined) {
      Serial.println("Join failed again, waiting...");
      return;
    }
  }

  // Prepare payload with RTC timestamp
  char payload[32];
  snprintf(payload, sizeof(payload),
           "%02d/%02d/%04d %02d:%02d:%02d",
           rtc.getDay(), rtc.getMonth(), 2000 + rtc.getYear(),
           rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());

  Serial.println("----------------------------------------");
  Serial.print("Sending uplink: ");
  Serial.println(payload);

  uint8_t downBuf[64];
  size_t downLen = sizeof(downBuf);
  int16_t ret = node.sendReceive((uint8_t*)payload, strlen(payload),
                                 10, downBuf, &downLen, false);

  if (ret == RADIOLIB_ERR_NONE) {
    Serial.println("✓ sendReceive succeeded");
    if (downLen > 0) {
      Serial.print("Downlink (");
      Serial.print(downLen);
      Serial.println(" bytes): ");
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
    if (ret == RADIOLIB_ERR_NETWORK_NOT_JOINED) {
      Serial.println("Lost join state, retrying next loop...");
      isJoined = false;
    }
  }

  Serial.println("----------------------------------------");
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  while (!Serial) {;}

  Serial.println("LoRa-E5 mini + RadioLib OTAA example");

  // Configure RF switch
  radio.setRfSwitchTable(rfPins, rfTable);

  int16_t err = radio.begin();
  if (err != RADIOLIB_ERR_NONE) {
    Serial.print("radio.begin() failed, code: ");
    Serial.println(err);
    while (true) delay(1000);
  }
  Serial.println("Radio initialized successfully");

  err = radio.setTCXO(1.7);
  if (err != RADIOLIB_ERR_NONE) {
    Serial.print("setTCXO() failed, code: ");
    Serial.println(err);
  }

  rtc.begin();
  rtc.setTime(12, 0, 0);
  rtc.setDate(4, 11, 24);

  // Join network
  isJoined = joinNetwork();
  if (!isJoined) {
    Serial.println("Initial join failed, will retry in loop...");
  } else {
    Serial.println("Starting periodic transmissions...");
  }
}

// --- Loop ---
void loop() {
  if (millis() - last_tx >= TX_INTERVAL) {
    sendPacket();
    last_tx = millis();
  }
  delay(10);
}



