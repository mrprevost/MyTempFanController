#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <PID_v1.h>

#include <CPwmFanControl.h>
#include <CTempSensors.h>
#include <CControllerServer.h>
#include <MyOTA.h>
#include "private.h"

#define FAN1_PWM_CHANNEL 0
#define FAN1_PWM_PIN 16
#define FAN1_TACH_PIN 17

#define FAN2_PWM_CHANNEL 1
#define FAN2_PWM_PIN 18
#define FAN2_TACH_PIN 19

#define TEMP_SENSOR_PIN 15

#define NON_WIFI_CORE 1

typedef struct FanSettings
{
  double fPidSetpoint = 85.0;
  double fPidKp = 2.0;
  double fPidKi = 5.0;
  double fPidKd = 1.0;
  double fFullSpeedTemp = 95.0;
  double fMinFanDutyCyclePercent = 30.0;
  double fFanOffDutyCyclePercent = 0.00;
} FanSettings;

typedef struct PersistentSettings
{
  FanSettings fan1;
  FanSettings fan2;
} PersistentSettings;

typedef struct FanControlSettings
{
  FanSettings *pFanSettings = NULL;
  CPwmFanControl *pFanCtrl = NULL;
  CTempSensors *pTempSensors = NULL;
  uint8_t nTempSensorIndex = 0;
  uint8_t bUseMaxTemp = 1;
} FanControlSettings;

PersistentSettings persistentSettings;
FanControlSettings settingsFan1 = {};
FanControlSettings settingsFan2 = {};

CPwmFanControl fan1Ctrl(FAN1_PWM_CHANNEL, FAN1_PWM_PIN, FAN1_TACH_PIN, 0, 0);
CPwmFanControl fan2Ctrl(FAN2_PWM_CHANNEL, FAN2_PWM_PIN, FAN2_TACH_PIN, 0, 0);
OneWire oneWire(TEMP_SENSOR_PIN);
CTempSensors tempSensors(&oneWire, HIGH_RES);

CControllerServer server(80);

void IRAM_ATTR handleFan1TachIrq()
{
  fan1Ctrl.isrFanTach();
}

void IRAM_ATTR handleFan2TachIrq()
{
  fan2Ctrl.isrFanTach();
}

void taskFanControl(FanControlSettings *pSettings)
{
  if (pSettings != NULL &&
      pSettings->pFanSettings != NULL &&
      pSettings->pFanCtrl != NULL &&
      pSettings->pTempSensors != NULL)
  {
    double fPidSetpoint = pSettings->pFanSettings->fPidSetpoint;
    double fPidInput = 0.0;
    double fPidOutputDutyCycle = 0.0;

    pSettings->pFanCtrl->setMinFanDutyCycle(CPwmFanControl::percentToDutyCycle(pSettings->pFanSettings->fMinFanDutyCyclePercent));
    pSettings->pFanCtrl->setFanOffDutyCycle(CPwmFanControl::percentToDutyCycle(pSettings->pFanSettings->fFanOffDutyCyclePercent));

    PID pid(&fPidInput,
            &fPidOutputDutyCycle,
            &fPidSetpoint,
            pSettings->pFanSettings->fPidKp,
            pSettings->pFanSettings->fPidKi,
            pSettings->pFanSettings->fPidKd,
            REVERSE);

    pid.SetMode(AUTOMATIC);

    for (;;)
    {
      double fTemp = 0.0;
      if (pSettings->bUseMaxTemp)
      {
        fTemp = pSettings->pTempSensors->getMaxTempF();
      }
      else
      {
        fTemp = pSettings->pTempSensors->getTempF(pSettings->nTempSensorIndex);
      }

      fPidInput = fTemp;

      pid.Compute();

      if (fTemp >= pSettings->pFanSettings->fFullSpeedTemp)
      {
        pSettings->pFanCtrl->setFullSpeed();
      }
      else
      {
        pSettings->pFanCtrl->setFanDutyCycle(round(fPidOutputDutyCycle));
      }

      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
  }
}

void taskTempUpdate(void *pvParam)
{
  for (;;)
  {
    //unsigned long nStart = micros();
    tempSensors.update();
    //Serial.printf("Temp Sensor Update took %02.3f sec\n", (double)(micros() - nStart) / 1000000.0);
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  //EEPROM.begin(sizeof(PersistentSettings));
  //EEPROM.get(0, persistentSettings);
  // EEPROM.put(0, persistentSettings);
  // EEPROM.commit();

  // initialize temp sensors and fan controllers
  tempSensors.begin();
  fan1Ctrl.begin(handleFan1TachIrq);
  fan1Ctrl.setFanDutyCyclePercent(100.0);
  fan2Ctrl.begin(handleFan2TachIrq);
  fan2Ctrl.setFanDutyCyclePercent(100.0);

  // START TEMPERATURE UPDATE TASK
  xTaskCreatePinnedToCore(
      (void (*)(void *))taskTempUpdate, // Function that should be called
      "taskTempUpdate",                 // Name of the task (for debugging)
      2500,                             // Stack size (bytes)
      NULL,                             // Parameter to pass
      0,                                //configMAX_PRIORITIES / 2, // Task priority
      NULL,                             // Task handle
      1                                 // Core you want to run the task on (0 or 1)
  );

  // START FAN #1 CONTROL TASK
  settingsFan1.pFanSettings = &persistentSettings.fan1;
  settingsFan1.pFanCtrl = &fan1Ctrl;
  settingsFan1.pTempSensors = &tempSensors;
  settingsFan1.nTempSensorIndex = 0;
  settingsFan1.bUseMaxTemp = 1;

  xTaskCreatePinnedToCore(
      (void (*)(void *))taskFanControl, // Function that should be called
      "taskFanControl1",                // Name of the task (for debugging)
      2500,                             // Stack size (bytes)
      &settingsFan1,                    // Parameter to pass
      0,                                //configMAX_PRIORITIES / 2, // Task priority
      NULL,                             // Task handle
      NON_WIFI_CORE                     // Core you want to run the task on (0 or 1)
  );

  // START FAN #2 CONTROL TASK
  // persistentSettings.fan2.fPidSetpoint = 75.0;
  // persistentSettings.fan2.fFullSpeedTemp = 85.0;

  settingsFan2.pFanSettings = &persistentSettings.fan2;
  settingsFan2.pFanCtrl = &fan2Ctrl;
  settingsFan2.pTempSensors = &tempSensors;
  settingsFan2.nTempSensorIndex = 1;
  settingsFan2.bUseMaxTemp = 0;

  xTaskCreatePinnedToCore(
      (void (*)(void *))taskFanControl, // Function that should be called
      "taskFanControl2",                // Name of the task (for debugging)
      2500,                             // Stack size (bytes)
      &settingsFan2,                    // Parameter to pass
      0,                                //configMAX_PRIORITIES / 2, // Task priority
      NULL,                             // Task handle
      NON_WIFI_CORE                     // Core you want to run the task on (0 or 1)
  );

  // CONNECT TO WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
  }
  else
  {
    CPwmFanControl *arrFanCtrl[] = {&fan1Ctrl, &fan2Ctrl};

    server.begin(arrFanCtrl, 2, &tempSensors);
  }

  setupOTA("MyFanController1");

  delay(1000);
}

void loop()
{
  //vTaskDelete(NULL);
  MySerial.printf("\n######### LOOP: running on core %d\n", xPortGetCoreID());

  MySerial.printf("Temp   1 = %02.3f\n", tempSensors.getTempF(0));
  MySerial.printf("Temp   2 = %02.3f\n", tempSensors.getTempF(1));
  MySerial.printf("Temp Max = %02.3f\n", tempSensors.getMaxTempF());

  MySerial.printf("Fan1 RPMs = %d\n", fan1Ctrl.getFanRpms());
  MySerial.printf("Fan2 RPMs = %d\n", fan2Ctrl.getFanRpms());

  // Serial.printf("persistentSettings.fan1.fPidSetpoint = %02.2f\n", persistentSettings.fan1.fPidSetpoint);
  // Serial.printf("persistentSettings.fan1.fFullSpeedTemp = %02.2f\n", persistentSettings.fan1.fFullSpeedTemp);
  // Serial.printf("persistentSettings.fan1.fMinFanDutyCyclePercent = %02.2f\n", persistentSettings.fan1.fMinFanDutyCyclePercent);
  MySerial.printf("fan1Ctrl.getLastDutyCycle() = %d (%d)\n", fan1Ctrl.getLastDutyCycle(), fan1Ctrl.getLastSpecDutyCycle());
  MySerial.printf("fan2Ctrl.getLastDutyCycle() = %d (%d)\n", fan2Ctrl.getLastDutyCycle(), fan2Ctrl.getLastSpecDutyCycle());

  MySerial.printf("\n");
  delay(1000);
}
