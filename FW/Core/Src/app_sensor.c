/**
  ******************************************************************************
  * @file    app_sensor.c
  * @brief   SEN5x air-quality sensor: power cycling, measurement and results
  ******************************************************************************
  */
#include "app_sensor.h"

#include "main.h"
#include "sen5x_i2c.h"
#include "stm32_seq.h"
#include "sys_app.h"
#include "utilities_def.h"

/* Cycle timing [s], see app_sensor.h ----------------------------------------*/
#define T_POWER_ON           0U
#define T_START              1U
#define T_SAMPLE_FIRST      31U   /* 30 s PM start-up after T_START       */
#define T_SAMPLE_LAST       60U   /* 30 one-second samples are averaged    */
#define T_PUBLISH           61U
#define T_STOP              62U
#define T_POWER_OFF         63U

/** Sensor is not powered while the supply level is at or below this value
  * (GetBatteryLevel() scale: average ADC counts / 8) */
#define SUPPLY_LEVEL_MIN   143U


/* SEN5x "unknown" values */
#define SEN5X_UNKNOWN_I16  0x7FFF
#define SEN5X_UNKNOWN_U16  0x7FFFU

/* SEN5x scale factors */
#define SEN5X_TEMPERATURE_SCALE   200.0
#define SEN5X_HUMIDITY_SCALE      100.0f

/* Private variables ---------------------------------------------------------*/
static AppSensor_Data_t data =
{
  .pm1p0 = APP_SENSOR_INVALID_U16, .pm2p5 = APP_SENSOR_INVALID_U16,
  .pm4p0 = APP_SENSOR_INVALID_U16, .pm10p0 = APP_SENSOR_INVALID_U16,
  .vocIndex = APP_SENSOR_INVALID_U16, .climateValid = false
};

static volatile uint16_t periodSec = APP_SENSOR_PERIOD_DEFAULT_S;
static volatile uint16_t tickSec;
static volatile uint8_t  supplyLevel;
static bool firstCycle = true;

static bool sensorOk;            /* sensor answered at start-up */

/* PM averaging over the sample window */
static struct
{
  uint32_t pm1p0, pm2p5, pm4p0, pm10p0;
  uint16_t count;
} pmSum;

/* VOC / temperature / humidity of the last sample in the window */
static uint16_t lastVoc = SEN5X_UNKNOWN_U16;
static int16_t  lastRawTemperature = SEN5X_UNKNOWN_I16;
static int16_t  lastRawHumidity = SEN5X_UNKNOWN_I16;
static uint8_t vocState[8];      /* VOC algorithm state kept across power-off */
static bool vocStateValid;

static void (*onMeasuredCb)(void);

/* Private functions ---------------------------------------------------------*/
static void SensorPower(bool on)
{
  HAL_GPIO_WritePin(CONNECT_PM_SENSOR_GPIO_Port, CONNECT_PM_SENSOR_Pin,
                    on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void InvalidateData(void)
{
  data.pm1p0 = data.pm2p5 = data.pm4p0 = data.pm10p0 = APP_SENSOR_INVALID_U16;
  data.vocIndex = APP_SENSOR_INVALID_U16;
  data.climateValid = false;
}

static void NotifyMeasured(void)
{
  if (onMeasuredCb != NULL)
  {
    onMeasuredCb();
  }
}

/** Task: reset the sensor, restore the VOC state and start measuring. */
static void TaskStart(void)
{
  unsigned char text[32];
  int16_t err = 0;

  err |= sen5x_device_reset();
  err |= sen5x_get_serial_number(text, sizeof(text));
  err |= sen5x_get_product_name(text, sizeof(text));

  /* The VOC state can only be written in idle mode, i.e. before starting. */
  if ((err == 0) && vocStateValid)
  {
    (void)sen5x_set_voc_algorithm_state(vocState, sizeof(vocState));
  }

  err |= sen5x_start_measurement();
  sensorOk = (err == 0);

  pmSum.pm1p0 = pmSum.pm2p5 = pmSum.pm4p0 = pmSum.pm10p0 = 0U;
  pmSum.count = 0U;
  lastVoc = SEN5X_UNKNOWN_U16;
  lastRawTemperature = lastRawHumidity = SEN5X_UNKNOWN_I16;
  if (!sensorOk)
  {
    APP_LOG(TS_ON, VLEVEL_M, "SEN5x: no response at start-up\r\n");
  }
}

/** Task: take one sample (once per second inside the sample window). */
static void TaskSample(void)
{
  uint16_t pm1p0, pm2p5, pm4p0, pm10p0, voc, nox;
  int16_t rawHumidity, rawTemperature;
  bool ready = false;

  if (!sensorOk || (sen5x_read_data_ready(&ready) != 0) || !ready)
  {
    return;   /* no new sample this second */
  }
  if (sen5x_read_measured_values(&pm1p0, &pm2p5, &pm4p0, &pm10p0,
                                 &rawHumidity, &rawTemperature, &voc, &nox) != 0)
  {
    return;
  }

  /* PM: 0xFFFF means "unknown" in the SEN5x protocol, skip such samples */
  if ((pm1p0 != APP_SENSOR_INVALID_U16) && (pm2p5 != APP_SENSOR_INVALID_U16) &&
      (pm4p0 != APP_SENSOR_INVALID_U16) && (pm10p0 != APP_SENSOR_INVALID_U16))
  {
    pmSum.pm1p0  += pm1p0;
    pmSum.pm2p5  += pm2p5;
    pmSum.pm4p0  += pm4p0;
    pmSum.pm10p0 += pm10p0;
    pmSum.count++;
  }

  /* VOC / temperature / humidity: keep the latest sample */
  lastVoc = voc;
  lastRawTemperature = rawTemperature;
  lastRawHumidity = rawHumidity;
}

static uint16_t Average(uint32_t sum, uint16_t count)
{
  return (uint16_t)((sum + count / 2U) / count);
}

/** Task: publish the averaged result and notify the application. */
static void TaskPublish(void)
{
  if (!sensorOk || (pmSum.count == 0U))
  {
    APP_LOG(TS_ON, VLEVEL_M, "SEN5x: no data\r\n");
    InvalidateData();
    NotifyMeasured();   /* still report supply level / sensor failure */
    return;
  }

  data.pm1p0  = Average(pmSum.pm1p0,  pmSum.count);
  data.pm2p5  = Average(pmSum.pm2p5,  pmSum.count);
  data.pm4p0  = Average(pmSum.pm4p0,  pmSum.count);
  data.pm10p0 = Average(pmSum.pm10p0, pmSum.count);
  data.vocIndex = (lastVoc == SEN5X_UNKNOWN_U16) ? APP_SENSOR_INVALID_U16 : lastVoc;

  if ((lastRawTemperature == SEN5X_UNKNOWN_I16) || (lastRawHumidity == SEN5X_UNKNOWN_I16))
  {
    data.climateValid = false;
  }
  else
  {
    data.temperature  = (double)lastRawTemperature / SEN5X_TEMPERATURE_SCALE;
    data.humidity     = lastRawHumidity / SEN5X_HUMIDITY_SCALE;
    data.climateValid = true;
  }

  APP_LOG(TS_ON, VLEVEL_M, "SEN5x: PM2.5 %u (x0.1 ug/m3), %u samples\r\n",
          data.pm2p5, pmSum.count);
  NotifyMeasured();
}

/** Task: save the VOC state and stop measuring (before power-off). */
static void TaskStop(void)
{
  if (sensorOk && (sen5x_get_voc_algorithm_state(vocState, sizeof(vocState)) == 0))
  {
    vocStateValid = true;
  }
  (void)sen5x_stop_measurement();
}

static void RunTask(CFG_SEQ_Task_Id_t task)
{
  UTIL_SEQ_SetTask(1U << task, CFG_SEQ_Prio_0);
}

/* Exported functions --------------------------------------------------------*/
void AppSensor_Init(void (*onMeasured)(void))
{
  onMeasuredCb = onMeasured;
  UTIL_SEQ_RegTask(1U << CFG_SEQ_Task_SensorStart, UTIL_SEQ_RFU, TaskStart);
  UTIL_SEQ_RegTask(1U << CFG_SEQ_Task_SensorSample,  UTIL_SEQ_RFU, TaskSample);
  UTIL_SEQ_RegTask(1U << CFG_SEQ_Task_SensorPublish, UTIL_SEQ_RFU, TaskPublish);
  UTIL_SEQ_RegTask(1U << CFG_SEQ_Task_SensorStop,  UTIL_SEQ_RFU, TaskStop);
  SensorPower(false);
}

void AppSensor_OnSecondTick(void)
{
  uint16_t period = periodSec;

  if (period == 0U)
  {
    SensorPower(false);   /* measuring switched off */
    return;
  }

  switch (tickSec)
  {
    case T_POWER_ON:
      if (firstCycle || (supplyLevel > SUPPLY_LEVEL_MIN))
      {
        SensorPower(true);
      }
      firstCycle = false;
      break;
    case T_START:
      RunTask(CFG_SEQ_Task_SensorStart);
      break;
    case T_PUBLISH:
      RunTask(CFG_SEQ_Task_SensorPublish);
      break;
    case T_STOP:
      RunTask(CFG_SEQ_Task_SensorStop);
      break;
    case T_POWER_OFF:
      SensorPower(false);
      break;
    default:
      if ((tickSec >= T_SAMPLE_FIRST) && (tickSec <= T_SAMPLE_LAST))
      {
        RunTask(CFG_SEQ_Task_SensorSample);
      }
      break;
  }

  /* period >= APP_SENSOR_PERIOD_MIN_S > T_POWER_OFF, so the on-window always
   * completes; if the period was shortened below the current position a new
   * cycle starts right away. */
  tickSec++;
  if (tickSec >= period)
  {
    tickSec = 0U;
  }
}

void AppSensor_SetPeriod(uint16_t seconds)
{
  if ((seconds != 0U) && (seconds < APP_SENSOR_PERIOD_MIN_S))
  {
    seconds = APP_SENSOR_PERIOD_MIN_S;
  }
  periodSec = seconds;
}

void AppSensor_SetSupplyLevel(uint8_t level)
{
  supplyLevel = level;
}

const AppSensor_Data_t *AppSensor_GetData(void)
{
  return &data;
}
