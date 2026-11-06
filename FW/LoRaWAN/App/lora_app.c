/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lora_app.c
  * @author  MCD Application Team
  * @brief   Application of the LRWAN Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "sys_app.h"
#include "lora_app.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "app_version.h"
#include "lorawan_version.h"
#include "subghz_phy_version.h"
#include "lora_info.h"
#include "LmHandler.h"
#include "adc_if.h"
#include "CayenneLpp.h"
#include "sys_sensors.h"
#include "flash_if.h"

/* USER CODE BEGIN Includes */
#include "app_sensor.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */
/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief LoRa State Machine states
  */
typedef enum TxEventType_e
{
  /**
    * @brief Appdata Transmission issue based on timer every TxDutyCycleTime
    */
  TX_ON_TIMER,
  /**
    * @brief Appdata Transmission external event plugged on OnSendEvent( )
    */
  TX_ON_EVENT
  /* USER CODE BEGIN TxEventType_t */

  /* USER CODE END TxEventType_t */
} TxEventType_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/**
  * LEDs period value of the timer in ms
  */
#define LED_PERIOD_TIME 500

/**
  * Join switch period value of the timer in ms
  */
#define JOIN_TIME 2000

/*---------------------------------------------------------------------------*/
/*                             LoRaWAN NVM configuration                     */
/*---------------------------------------------------------------------------*/
/**
  * @brief LoRaWAN NVM Flash address
  * @note last 2 pages (4 KB) of the 256 KB STM32WLE5JC, reserved as region NVM
  *       in STM32WLE5JCIX_FLASH.ld so code can never grow into it
  */
#define LORAWAN_NVM_BASE_ADDRESS                    ((void *)0x0803F000UL)

/* USER CODE BEGIN PD */
static const char *slotStrings[] = { "1", "2", "C", "C_MC", "P", "P_MC" };

/* Retry delay when the MAC is busy */
#define TX_RETRY_TIME_MS        10000U
/* Shortest periodic uplink interval */
#define UPLINK_PERIOD_MIN_MS    60000U

/* Downlink commands on LORAWAN_USER_APP_PORT ---------------------------------
 *   01 ss              LED            ss: 00 = off, 01 = on
 *   02 mm              measure period mm: minutes, 00 = measuring off,
 *                                         01 is raised to the 2 min minimum
 *   03 mm              uplink period  mm: minutes. Uplinks are then sent only
 *                                         on this timer (latest values).
 *                                         00 = one uplink after each measurement
 *   FF FF FF FF FF FF  reset
 */
#define DL_CMD_LED              0x01U
#define DL_CMD_MEASURE_PERIOD   0x02U
#define DL_CMD_UPLINK_PERIOD    0x03U
#define DL_RESET_LEN            6U
#define DL_RESET_BYTE           0xFFU

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  LoRa End Node send request
  */
static void SendTxData(void);

/**
  * @brief  TX timer callback function
  * @param  context ptr of timer context
  */
static void OnTxTimerEvent(void *context);

/**
  * @brief  join event callback function
  * @param  joinParams status of join
  */
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);

/**
  * @brief callback when LoRaWAN application has sent a frame
  * @brief  tx event callback function
  * @param  params status of last Tx
  */
static void OnTxData(LmHandlerTxParams_t *params);

/**
  * @brief callback when LoRaWAN application has received a frame
  * @param appData data received in the last Rx
  * @param params status of last Rx
  */
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);

/**
  * @brief callback when LoRaWAN Beacon status is updated
  * @param params status of Last Beacon
  */
static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params);

/**
  * @brief callback when system time has been updated
  */
static void OnSysTimeUpdate(void);

/**
  * @brief callback when LoRaWAN application Class is changed
  * @param deviceClass new class
  */
static void OnClassChange(DeviceClass_t deviceClass);

/**
  * @brief  LoRa store context in Non Volatile Memory
  */
static void StoreContext(void);

/**
  * @brief  stop current LoRa execution to switch into non default Activation mode
  */
static void StopJoin(void);

/**
  * @brief  Join switch timer callback function
  * @param  context ptr of Join switch context
  */
static void OnStopJoinTimerEvent(void *context);

/**
  * @brief  Notifies the upper layer that the NVM context has changed
  * @param  state Indicates if we are storing (true) or restoring (false) the NVM context
  */
static void OnNvmDataChange(LmHandlerNvmContextStates_t state);

/**
  * @brief  Store the NVM Data context to the Flash
  * @param  nvm ptr on nvm structure
  * @param  nvm_size number of data bytes which were stored
  */
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size);

/**
  * @brief  Restore the NVM Data context from the Flash
  * @param  nvm ptr on nvm structure
  * @param  nvm_size number of data bytes which were restored
  */
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size);

/**
  * Will be called each time a Radio IRQ is handled by the MAC layer
  *
  */
static void OnMacProcessNotify(void);

/**
  * @brief Change the periodicity of the uplink frames
  * @param periodicity uplink frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnTxPeriodicityChanged(uint32_t periodicity);

/**
  * @brief Change the confirmation control of the uplink frames
  * @param isTxConfirmed Indicates if the uplink requires an acknowledgement
  * @note Compliance test protocol callbacks
  */
static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed);

/**
  * @brief Change the periodicity of the ping slot frames
  * @param pingSlotPeriodicity ping slot frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity);

/**
  * @brief Will be called to reset the system
  * @note Compliance test protocol callbacks
  */
static void OnSystemReset(void);

/* USER CODE BEGIN PFP */
static void OnTxRetryTimerEvent(void *context);
static void SetUplinkPeriod(uint32_t periodMs);
static void RequestSend(void);
static void SetLed(bool on);
static void ProcessAppDownlink(const uint8_t *buf, uint8_t size);
static uint8_t BuildPayload(uint8_t *buf, uint8_t batteryLevel);

/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
/**
  * @brief LoRaWAN default activation type
  */
static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

/**
  * @brief LoRaWAN force rejoin even if the NVM context is restored
  */
static bool ForceRejoin = LORAWAN_FORCE_REJOIN_AT_BOOT;

/**
  * @brief LoRaWAN handler Callbacks
  */
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .GetBatteryLevel =              GetBatteryLevel,
  .GetTemperature =               GetTemperatureLevel,
  .GetUniqueId =                  GetUniqueId,
  .GetDevAddr =                   GetDevAddr,
  .OnRestoreContextRequest =      OnRestoreContextRequest,
  .OnStoreContextRequest =        OnStoreContextRequest,
  .OnMacProcess =                 OnMacProcessNotify,
  .OnNvmDataChange =              OnNvmDataChange,
  .OnJoinRequest =                OnJoinRequest,
  .OnTxData =                     OnTxData,
  .OnRxData =                     OnRxData,
  .OnBeaconStatusChange =         OnBeaconStatusChange,
  .OnSysTimeUpdate =              OnSysTimeUpdate,
  .OnClassChange =                OnClassChange,
  .OnTxPeriodicityChanged =       OnTxPeriodicityChanged,
  .OnTxFrameCtrlChanged =         OnTxFrameCtrlChanged,
  .OnPingSlotPeriodicityChanged = OnPingSlotPeriodicityChanged,
  .OnSystemReset =                OnSystemReset,
};

/**
  * @brief LoRaWAN handler parameters
  */
static LmHandlerParams_t LmHandlerParams =
{
  .ActiveRegion =             ACTIVE_REGION,
  .DefaultClass =             LORAWAN_DEFAULT_CLASS,
  .AdrEnable =                LORAWAN_ADR_STATE,
  .IsTxConfirmed =            LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
  .TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
  .TxPower =                  LORAWAN_DEFAULT_TX_POWER,
  .PingSlotPeriodicity =      LORAWAN_DEFAULT_PING_SLOT_PERIODICITY,
  .RxBCTimeout =              LORAWAN_DEFAULT_CLASS_B_C_RESP_TIMEOUT
};

/**
  * @brief Type of Event to generate application Tx
  */
static TxEventType_t EventType = TX_ON_TIMER;

/**
  * @brief Timer to handle the application Tx
  */
static UTIL_TIMER_Object_t TxTimer;

/**
  * @brief Tx Timer period
  */
static UTIL_TIMER_Time_t TxPeriodicity = APP_TX_DUTYCYCLE;

/**
  * @brief Join Timer period
  */
static UTIL_TIMER_Object_t StopJoinTimer;

/* USER CODE BEGIN PV */
/**
  * @brief User application buffer
  */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

/**
  * @brief User application data structure
  */
static LmHandlerAppData_t AppData = { 0, 0, AppDataBuffer };

/**
  * @brief Uplink waiting for the network join / a free MAC
  */
static volatile bool PendingTx = false;

/**
  * @brief Retry timer used when the MAC is busy
  */
static UTIL_TIMER_Object_t TxRetryTimer;

/**
  * @brief State of the user LED (reported in the uplink)
  */
static bool LedOn = false;

/**
  * @brief Periodic uplink interval [ms]; 0 = one uplink after each measurement
  */
static uint32_t UplinkPeriodMs = 0U;

/* USER CODE END PV */

/* Exported functions ---------------------------------------------------------*/
/* USER CODE BEGIN EF */

/* USER CODE END EF */

void LoRaWAN_Init(void)
{
  /* USER CODE BEGIN LoRaWAN_Init_LV */
  uint32_t feature_version = 0UL;
  /* USER CODE END LoRaWAN_Init_LV */

  /* USER CODE BEGIN LoRaWAN_Init_1 */

  /* Get LoRaWAN APP version*/
  APP_LOG(TS_OFF, VLEVEL_M, "APPLICATION_VERSION: V%X.%X.%X\r\n",
          (uint8_t)(APP_VERSION_MAIN),
          (uint8_t)(APP_VERSION_SUB1),
          (uint8_t)(APP_VERSION_SUB2));

  /* Get MW LoRaWAN info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_LORAWAN_VERSION: V%X.%X.%X\r\n",
          (uint8_t)(LORAWAN_VERSION_MAIN),
          (uint8_t)(LORAWAN_VERSION_SUB1),
          (uint8_t)(LORAWAN_VERSION_SUB2));

  /* Get MW SubGhz_Phy info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_RADIO_VERSION:   V%X.%X.%X\r\n",
          (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  /* Get LoRaWAN Link Layer info */
  LmHandlerGetVersion(LORAMAC_HANDLER_L2_VERSION, &feature_version);
  APP_LOG(TS_OFF, VLEVEL_M, "L2_SPEC_VERSION:     V%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8));

  /* Get LoRaWAN Regional Parameters info */
  LmHandlerGetVersion(LORAMAC_HANDLER_REGION_VERSION, &feature_version);
  APP_LOG(TS_OFF, VLEVEL_M, "RP_SPEC_VERSION:     V%X-%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8),
          (uint8_t)(feature_version));

  /* USER CODE END LoRaWAN_Init_1 */

  UTIL_TIMER_Create(&StopJoinTimer, JOIN_TIME, UTIL_TIMER_ONESHOT, OnStopJoinTimerEvent, NULL);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LmHandlerProcess), UTIL_SEQ_RFU, LmHandlerProcess);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), UTIL_SEQ_RFU, SendTxData);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaStoreContextEvent), UTIL_SEQ_RFU, StoreContext);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), UTIL_SEQ_RFU, StopJoin);

  /* Init Info table used by LmHandler*/
  LoraInfo_Init();

  /* Init the Lora Stack*/
  LmHandlerInit(&LmHandlerCallbacks, APP_VERSION);

  LmHandlerConfigure(&LmHandlerParams);

  /* USER CODE BEGIN LoRaWAN_Init_2 */
  {
    /* Print the identity used for the OTAA join, e.g. a DevEUI derived from
     * the MCU unique ID that must be entered in the network console */
    uint8_t eui[8];
    if (LmHandlerGetDevEUI(eui) == LORAMAC_HANDLER_SUCCESS)
    {
      APP_LOG(TS_OFF, VLEVEL_M, "DevEUI:  %02X%02X%02X%02X%02X%02X%02X%02X\r\n",
              eui[0], eui[1], eui[2], eui[3], eui[4], eui[5], eui[6], eui[7]);
    }
    if (LmHandlerGetAppEUI(eui) == LORAMAC_HANDLER_SUCCESS)
    {
      APP_LOG(TS_OFF, VLEVEL_M, "JoinEUI: %02X%02X%02X%02X%02X%02X%02X%02X\r\n",
              eui[0], eui[1], eui[2], eui[3], eui[4], eui[5], eui[6], eui[7]);
    }
  }

  /* USER CODE END LoRaWAN_Init_2 */

  LmHandlerJoin(ActivationType, ForceRejoin);

  if (EventType == TX_ON_TIMER)
  {
    /* send every time timer elapses */
    UTIL_TIMER_Create(&TxTimer, TxPeriodicity, UTIL_TIMER_ONESHOT, OnTxTimerEvent, NULL);
    UTIL_TIMER_Start(&TxTimer);
  }
  else
  {
    /* USER CODE BEGIN LoRaWAN_Init_3 */

    /* USER CODE END LoRaWAN_Init_3 */
  }

  /* USER CODE BEGIN LoRaWAN_Init_Last */
  /* Default: one uplink after each measurement (LoRaWAN_OnMeasurementDone).
   * TxTimer only runs when an uplink period is set by downlink (cmd 0x03). */
  UTIL_TIMER_Stop(&TxTimer);
  UTIL_TIMER_Create(&TxRetryTimer, TX_RETRY_TIME_MS, UTIL_TIMER_ONESHOT, OnTxRetryTimerEvent, NULL);

  /* USER CODE END LoRaWAN_Init_Last */
}

/* USER CODE BEGIN PB_Callbacks */

/* USER CODE END PB_Callbacks */

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */
static void RequestSend(void)
{
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);
}

void LoRaWAN_OnMeasurementDone(void)
{
  /* with an uplink period set, uplinks come only from TxTimer */
  if (UplinkPeriodMs == 0U)
  {
    RequestSend();
  }
}

static void OnTxRetryTimerEvent(void *context)
{
  if (PendingTx)
  {
    RequestSend();
  }
}

/**
  * @brief Set the uplink mode.
  * @param periodMs 0 = one uplink after each measurement; otherwise uplinks are
  *        sent only every periodMs (at least UPLINK_PERIOD_MIN_MS), each with
  *        the latest measurement
  */
static void SetUplinkPeriod(uint32_t periodMs)
{
  if (periodMs == 0U)
  {
    UplinkPeriodMs = 0U;
    UTIL_TIMER_Stop(&TxTimer);
    APP_LOG(TS_OFF, VLEVEL_M, "Uplink after each measurement\r\n");
  }
  else
  {
    UplinkPeriodMs = MAX(periodMs, UPLINK_PERIOD_MIN_MS);
    OnTxPeriodicityChanged(UplinkPeriodMs);   /* restarts TxTimer */
    APP_LOG(TS_OFF, VLEVEL_M, "Uplink every %u s\r\n", (unsigned)(UplinkPeriodMs / 1000U));
  }
}

static void SetLed(bool on)
{
  LedOn = on;
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
  APP_LOG(TS_OFF, VLEVEL_M, "LED %s\r\n", on ? "ON" : "OFF");
}

/**
  * @brief Execute a downlink received on LORAWAN_USER_APP_PORT (see DL_CMD_xxx)
  */
static void ProcessAppDownlink(const uint8_t *buf, uint8_t size)
{
  if (size == DL_RESET_LEN)
  {
    for (uint8_t i = 0; i < DL_RESET_LEN; i++)
    {
      if (buf[i] != DL_RESET_BYTE)
      {
        APP_LOG(TS_OFF, VLEVEL_M, "Unknown downlink\r\n");
        return;
      }
    }
    NVIC_SystemReset();
  }

  if (size != 2U)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "Unknown downlink\r\n");
    return;
  }

  switch (buf[0])
  {
    case DL_CMD_LED:
      if (buf[1] <= 1U)
      {
        SetLed(buf[1] == 1U);
      }
      break;
    case DL_CMD_MEASURE_PERIOD:
      AppSensor_SetPeriod((uint16_t)buf[1] * 60U);
      APP_LOG(TS_OFF, VLEVEL_M, "Measure period %u min\r\n", buf[1]);
      break;
    case DL_CMD_UPLINK_PERIOD:
      SetUplinkPeriod((uint32_t)buf[1] * 60000U);
      break;
    default:
      APP_LOG(TS_OFF, VLEVEL_M, "Unknown downlink cmd 0x%02X\r\n", buf[0]);
      break;
  }
}

/**
  * @brief Build the 14-byte uplink payload (big endian):
  *   0-1  PM1.0  [ug/m3 x10]      0xFFFF = invalid
  *   2-3  PM2.5  [ug/m3 x10]      0xFFFF = invalid
  *   4-5  PM10   [ug/m3 x10]      0xFFFF = invalid
  *   6-7  temperature: v = T*100 + 10000, bytes v/100, v%100   FF FF = invalid
  *   8-9  humidity:    v = RH*100,        bytes v/100, v%100   FF FF = invalid
  *   10-11 VOC index x10          0xFFFF = invalid
  *   12   battery level (average ADC counts / 8)
  *   13   LED state
  * @return payload size
  */
static uint8_t BuildPayload(uint8_t *buf, uint8_t batteryLevel)
{
  const AppSensor_Data_t *d = AppSensor_GetData();
  uint8_t i = 0;
  int16_t v;

  buf[i++] = (uint8_t)(d->pm1p0 >> 8);
  buf[i++] = (uint8_t)(d->pm1p0);
  buf[i++] = (uint8_t)(d->pm2p5 >> 8);
  buf[i++] = (uint8_t)(d->pm2p5);
  buf[i++] = (uint8_t)(d->pm10p0 >> 8);
  buf[i++] = (uint8_t)(d->pm10p0);

  if (d->climateValid)
  {
    v = (int16_t)(d->temperature * 100.0) + 10000;
    buf[i++] = (uint8_t)(v / 100);
    buf[i++] = (uint8_t)(v % 100);
    v = (int16_t)(d->humidity * 100.0);
    buf[i++] = (uint8_t)(v / 100);
    buf[i++] = (uint8_t)(v % 100);
  }
  else
  {
    /* FF FF cannot occur in the normal encoding (2nd byte is 0..99) */
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
  }

  buf[i++] = (uint8_t)(d->vocIndex >> 8);
  buf[i++] = (uint8_t)(d->vocIndex);
  buf[i++] = batteryLevel;
  buf[i++] = LedOn ? 1U : 0U;

  return i;
}

/* USER CODE END PrFD */

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
  /* USER CODE BEGIN OnRxData_1 */
  if ((params == NULL) || (appData == NULL))
  {
    return;
  }

  if (params->IsMcpsIndication && (appData->Buffer != NULL))
  {
    switch (appData->Port)
    {
      case LORAWAN_SWITCH_CLASS_PORT:
        /* this port switches the class */
        if (appData->BufferSize == 1)
        {
          switch (appData->Buffer[0])
          {
            case 0:
              LmHandlerRequestClass(CLASS_A);
              break;
            case 1:
              LmHandlerRequestClass(CLASS_B);
              break;
            case 2:
              LmHandlerRequestClass(CLASS_C);
              break;
            default:
              break;
          }
        }
        break;

      case LORAWAN_USER_APP_PORT:
        ProcessAppDownlink(appData->Buffer, appData->BufferSize);
        break;

      default:
        break;
    }
  }

  if (params->RxSlot < RX_SLOT_NONE)
  {
    APP_LOG(TS_OFF, VLEVEL_H, "###### D/L FRAME:%04d | PORT:%d | DR:%d | SLOT:%s | RSSI:%d | SNR:%d\r\n",
            params->DownlinkCounter, appData->Port, params->Datarate, slotStrings[params->RxSlot],
            params->Rssi, params->Snr);
  }
  /* USER CODE END OnRxData_1 */
}

static void SendTxData(void)
{
  /* USER CODE BEGIN SendTxData_1 */
  LmHandlerErrorStatus_t status;
  uint8_t batteryLevel = GetBatteryLevel();

  /* the sensor uses the supply level to decide whether to power up */
  AppSensor_SetSupplyLevel(batteryLevel);
  APP_LOG(TS_ON, VLEVEL_M, "Battery level: %u\r\n", batteryLevel);

  AppData.Port = LORAWAN_USER_APP_PORT;
  AppData.BufferSize = BuildPayload(AppData.Buffer, batteryLevel);

  /* allowDelayedTx = true: if the duty cycle is not free yet the MAC queues
   * the frame and sends it as soon as allowed */
  status = LmHandlerSend(&AppData, LmHandlerParams.IsTxConfirmed, true);
  PendingTx = false;

  switch (status)
  {
    case LORAMAC_HANDLER_SUCCESS:
      APP_LOG(TS_ON, VLEVEL_L, "SEND REQUEST\r\n");
      break;
    case LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED:
      APP_LOG(TS_ON, VLEVEL_L, "Next Tx in  : ~%d second(s)\r\n", (LmHandlerGetDutyCycleWaitTime() / 1000));
      break;
    case LORAMAC_HANDLER_NO_NETWORK_JOINED:
      /* LmHandlerSend has (re)started the join; send once joined */
      PendingTx = true;
      APP_LOG(TS_ON, VLEVEL_M, "Not joined, uplink postponed\r\n");
      break;
    case LORAMAC_HANDLER_BUSY_ERROR:
    case LORAMAC_HANDLER_ERROR:
      PendingTx = true;
      UTIL_TIMER_Start(&TxRetryTimer);
      APP_LOG(TS_ON, VLEVEL_M, "MAC busy, retry in %d s\r\n", TX_RETRY_TIME_MS / 1000);
      break;
    default:
      APP_LOG(TS_ON, VLEVEL_M, "Send failed (%d)\r\n", status);
      break;
  }

  /* USER CODE END SendTxData_1 */
}

static void OnTxTimerEvent(void *context)
{
  /* USER CODE BEGIN OnTxTimerEvent_1 */

  /* USER CODE END OnTxTimerEvent_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);

  /*Wait for next tx slot*/
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxTimerEvent_2 */

  /* USER CODE END OnTxTimerEvent_2 */
}

/* USER CODE BEGIN PrFD_LedEvents */

/* USER CODE END PrFD_LedEvents */

static void OnTxData(LmHandlerTxParams_t *params)
{
  /* USER CODE BEGIN OnTxData_1 */
  /* Process Tx event only if its a mcps response to prevent some internal events (mlme) */
  if ((params != NULL) && (params->IsMcpsConfirm != 0))
  {
    APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### ========== MCPS-Confirm =============\r\n");
    APP_LOG(TS_OFF, VLEVEL_H, "###### U/L FRAME:%04d | PORT:%d | DR:%d | PWR:%d", params->UplinkCounter,
            params->AppData.Port, params->Datarate, params->TxPower);

    APP_LOG(TS_OFF, VLEVEL_H, " | MSG TYPE:");
    if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG)
    {
      APP_LOG(TS_OFF, VLEVEL_H, "CONFIRMED [%s]\r\n", (params->AckReceived != 0) ? "ACK" : "NACK");
    }
    else
    {
      APP_LOG(TS_OFF, VLEVEL_H, "UNCONFIRMED\r\n");
    }
  }
  /* USER CODE END OnTxData_1 */
}

static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
  /* USER CODE BEGIN OnJoinRequest_1 */
  if (joinParams == NULL)
  {
    return;
  }

  if (joinParams->Status == LORAMAC_HANDLER_SUCCESS)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOINED = %s ======================\r\n",
            (joinParams->Mode == ACTIVATION_TYPE_ABP) ? "ABP" : "OTAA");
    if (PendingTx)
    {
      RequestSend();
    }
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOIN FAILED\r\n");
    if (joinParams->Mode == ACTIVATION_TYPE_OTAA)
    {
      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = RE-TRYING OTAA JOIN\r\n");
      LmHandlerJoin(ActivationType, LORAWAN_FORCE_REJOIN_AT_BOOT);
    }
  }
  /* USER CODE END OnJoinRequest_1 */
}

static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params)
{
  /* USER CODE BEGIN OnBeaconStatusChange_1 */
  if (params != NULL)
  {
    switch (params->State)
    {
      default:
      case LORAMAC_HANDLER_BEACON_LOST:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON LOST\r\n");
        break;
      }
      case LORAMAC_HANDLER_BEACON_RX:
      {
        APP_LOG(TS_OFF, VLEVEL_M,
                "\r\n###### BEACON RECEIVED | DR:%d | RSSI:%d | SNR:%d | FQ:%d | TIME:%d | DESC:%d | "
                "INFO:02X%02X%02X %02X%02X%02X\r\n",
                params->Info.Datarate, params->Info.Rssi, params->Info.Snr, params->Info.Frequency,
                params->Info.Time.Seconds, params->Info.GwSpecific.InfoDesc,
                params->Info.GwSpecific.Info[0], params->Info.GwSpecific.Info[1],
                params->Info.GwSpecific.Info[2], params->Info.GwSpecific.Info[3],
                params->Info.GwSpecific.Info[4], params->Info.GwSpecific.Info[5]);
        break;
      }
      case LORAMAC_HANDLER_BEACON_NRX:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON NOT RECEIVED\r\n");
        break;
      }
    }
  }
  /* USER CODE END OnBeaconStatusChange_1 */
}

static void OnSysTimeUpdate(void)
{
  /* USER CODE BEGIN OnSysTimeUpdate_1 */

  /* USER CODE END OnSysTimeUpdate_1 */
}

static void OnClassChange(DeviceClass_t deviceClass)
{
  /* USER CODE BEGIN OnClassChange_1 */
  APP_LOG(TS_OFF, VLEVEL_M, "Switch to Class %c done\r\n", "ABC"[deviceClass]);
  /* USER CODE END OnClassChange_1 */
}

static void OnMacProcessNotify(void)
{
  /* USER CODE BEGIN OnMacProcessNotify_1 */

  /* USER CODE END OnMacProcessNotify_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LmHandlerProcess), CFG_SEQ_Prio_0);

  /* USER CODE BEGIN OnMacProcessNotify_2 */

  /* USER CODE END OnMacProcessNotify_2 */
}

static void OnTxPeriodicityChanged(uint32_t periodicity)
{
  /* USER CODE BEGIN OnTxPeriodicityChanged_1 */

  /* USER CODE END OnTxPeriodicityChanged_1 */
  TxPeriodicity = periodicity;

  if (TxPeriodicity == 0)
  {
    /* Revert to application default periodicity */
    TxPeriodicity = APP_TX_DUTYCYCLE;
  }

  /* Update timer periodicity */
  UTIL_TIMER_Stop(&TxTimer);
  UTIL_TIMER_SetPeriod(&TxTimer, TxPeriodicity);
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxPeriodicityChanged_2 */

  /* USER CODE END OnTxPeriodicityChanged_2 */
}

static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed)
{
  /* USER CODE BEGIN OnTxFrameCtrlChanged_1 */

  /* USER CODE END OnTxFrameCtrlChanged_1 */
  LmHandlerParams.IsTxConfirmed = isTxConfirmed;
  /* USER CODE BEGIN OnTxFrameCtrlChanged_2 */

  /* USER CODE END OnTxFrameCtrlChanged_2 */
}

static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity)
{
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_1 */

  /* USER CODE END OnPingSlotPeriodicityChanged_1 */
  LmHandlerParams.PingSlotPeriodicity = pingSlotPeriodicity;
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_2 */

  /* USER CODE END OnPingSlotPeriodicityChanged_2 */
}

static void OnSystemReset(void)
{
  /* USER CODE BEGIN OnSystemReset_1 */

  /* USER CODE END OnSystemReset_1 */
  if ((LORAMAC_HANDLER_SUCCESS == LmHandlerHalt()) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET))
  {
    NVIC_SystemReset();
  }
  /* USER CODE BEGIN OnSystemReset_Last */

  /* USER CODE END OnSystemReset_Last */
}

static void StopJoin(void)
{
  /* USER CODE BEGIN StopJoin_1 */

  /* USER CODE END StopJoin_1 */

  UTIL_TIMER_Stop(&TxTimer);

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerStop())
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stop on going ...\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stopped\r\n");
    if (LORAWAN_DEFAULT_ACTIVATION_TYPE == ACTIVATION_TYPE_ABP)
    {
      ActivationType = ACTIVATION_TYPE_OTAA;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to OTAA mode\r\n");
    }
    else
    {
      ActivationType = ACTIVATION_TYPE_ABP;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to ABP mode\r\n");
    }
    LmHandlerConfigure(&LmHandlerParams);
    LmHandlerJoin(ActivationType, true);
    UTIL_TIMER_Start(&TxTimer);
  }
  UTIL_TIMER_Start(&StopJoinTimer);
  /* USER CODE BEGIN StopJoin_Last */

  /* USER CODE END StopJoin_Last */
}

static void OnStopJoinTimerEvent(void *context)
{
  /* USER CODE BEGIN OnStopJoinTimerEvent_1 */

  /* USER CODE END OnStopJoinTimerEvent_1 */
  if (ActivationType == LORAWAN_DEFAULT_ACTIVATION_TYPE)
  {
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), CFG_SEQ_Prio_0);
  }
  /* USER CODE BEGIN OnStopJoinTimerEvent_Last */

  /* USER CODE END OnStopJoinTimerEvent_Last */
}

static void StoreContext(void)
{
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

  /* USER CODE BEGIN StoreContext_1 */

  /* USER CODE END StoreContext_1 */
  status = LmHandlerNvmDataStore();

  if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA UP TO DATE\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORE FAILED\r\n");
  }
  /* USER CODE BEGIN StoreContext_Last */

  /* USER CODE END StoreContext_Last */
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  /* USER CODE BEGIN OnNvmDataChange_1 */

  /* USER CODE END OnNvmDataChange_1 */
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORED\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA RESTORED\r\n");
  }
  /* USER CODE BEGIN OnNvmDataChange_Last */

  /* USER CODE END OnNvmDataChange_Last */
}

static void OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnStoreContextRequest_1 */

  /* USER CODE END OnStoreContextRequest_1 */
  /* store nvm in flash */
  if (FLASH_IF_Erase(LORAWAN_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE) == FLASH_IF_OK)
  {
    FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, (const void *)nvm, nvm_size);
  }
  /* USER CODE BEGIN OnStoreContextRequest_Last */

  /* USER CODE END OnStoreContextRequest_Last */
}

static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnRestoreContextRequest_1 */

  /* USER CODE END OnRestoreContextRequest_1 */
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  /* USER CODE BEGIN OnRestoreContextRequest_Last */

  /* USER CODE END OnRestoreContextRequest_Last */
}

