#ifndef __CTEMPSENSORS_H__
#define __CTEMPSENSORS_H__

#include <Arduino.h>
#include <DallasTemperature.h>

enum Resolution
{
  LOW_RES = 9,
  MEDIUM_RES = 10,
  MEDIUM_HIGH_RES = 11,
  HIGH_RES = 12
};

class CTempSensors
{
public:
  CTempSensors(OneWire *pOneWire, Resolution resolution = MEDIUM_RES, uint8_t nMaxSensors = 4);
  ~CTempSensors();

  void begin();
  float readSensor(uint8_t nIndex);
  void update();

  uint8_t getNumSensors();
  float getTempRaw(uint8_t nIndex);
  float getTempF(uint8_t nIndex);
  float getTempC(uint8_t nIndex);
  float getMaxTempF();
  float getMaxTempC();

  DeviceAddress *getSensorAddresses();
  static char *addressToString(DeviceAddress deviceAddress, char *pszBuf24, size_t nBufLen = 24);

private:
  void discoverSensorAddresses();

  portMUX_TYPE m_muxTempData = portMUX_INITIALIZER_UNLOCKED;
  uint8_t m_nMaxSensors = 4;
  uint8_t m_nNumSensors = 0;
  Resolution m_resolution;
  float *m_arrRawTemps = NULL;
  float m_fMaxRawTemp = -999.0;
  DeviceAddress *m_arrSensorAddresses = NULL;
  OneWire *m_pOneWire = NULL;
  DallasTemperature m_sensors;
};

#endif // #ifndef __CTEMPSENSORS_H__
