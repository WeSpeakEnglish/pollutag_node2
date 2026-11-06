/**
  ******************************************************************************
  * @file    app_sensor.h
  * @brief   SEN5x air-quality sensor: power cycling, measurement and results
  ******************************************************************************
  *
  * Measurement cycle, driven by a 1 s tick (LPTIM1 compare match):
  *
  *   t =  0 s      sensor power on (skipped if the supply level is too low)
  *   t =  1 s      reset, restore VOC algorithm state, start measurement
  *   t = 31..60 s  PM: one sample per second after 30 s start-up, 30 s average
  *                 (Sensirion "Reduced Power Operation SEN5x" recommendation);
  *                 VOC, temperature, humidity: last sample
  *   t = 61 s      publish the result, notify the application (onMeasured)
  *   t = 62 s      save VOC algorithm state, stop measurement
  *   t = 63 s      sensor power off
  *   t = period:   start again
  *
  ******************************************************************************
  */
#ifndef APP_SENSOR_H
#define APP_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/** "No valid reading" marker for the PM and VOC fields */
#define APP_SENSOR_INVALID_U16        0xFFFFU

/** Default and shortest measurement period (seconds). Must be longer than
  * the 63 s sensor on-window, otherwise the sensor would never switch off. */
#define APP_SENSOR_PERIOD_DEFAULT_S   1800U
#define APP_SENSOR_PERIOD_MIN_S       120U

/** Latest measurement. Values are kept until the next measurement. */
typedef struct
{
  uint16_t pm1p0;          /**< PM1.0  [ug/m3 x10], 30 s average, APP_SENSOR_INVALID_U16 if unknown */
  uint16_t pm2p5;          /**< PM2.5  [ug/m3 x10] */
  uint16_t pm4p0;          /**< PM4.0  [ug/m3 x10] */
  uint16_t pm10p0;         /**< PM10   [ug/m3 x10] */
  uint16_t vocIndex;       /**< VOC index x10 */
  double   temperature;    /**< [degC], valid only if climateValid */
  float    humidity;       /**< [%RH],  valid only if climateValid */
  bool     climateValid;
} AppSensor_Data_t;

/**
  * @brief Initialise the module. Registers the sequencer tasks and switches the
  *        sensor power off. Call once after the peripherals are initialised.
  * @param onMeasured called (from task context) after every measurement attempt,
  *        successful or not; may be NULL
  */
void AppSensor_Init(void (*onMeasured)(void));

/** @brief 1 s tick, call from the LPTIM interrupt. */
void AppSensor_OnSecondTick(void);

/**
  * @brief Set the measurement period.
  * @param seconds 0 = measuring off, otherwise clamped to >= APP_SENSOR_PERIOD_MIN_S
  */
void AppSensor_SetPeriod(uint16_t seconds);

/**
  * @brief Report the current supply level (same scale as GetBatteryLevel()).
  *        The sensor is not powered at the start of a cycle while the level is
  *        at or below the threshold (except for the very first cycle after boot).
  */
void AppSensor_SetSupplyLevel(uint8_t level);

/** @brief Latest measurement. */
const AppSensor_Data_t *AppSensor_GetData(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SENSOR_H */
