#include <CTempSensors.h>

CTempSensors::CTempSensors(OneWire *pOneWire, Resolution resolution /*=MEDIUM_RES*/, uint8_t nMaxSensors /* =4 */)
    : m_sensors(pOneWire)
{
  m_pOneWire = pOneWire;
  m_resolution = resolution;

  m_arrRawTemps = new float[nMaxSensors];
  memset(m_arrRawTemps, 0, sizeof(float[m_nMaxSensors]));

  m_arrSensorAddresses = new DeviceAddress[nMaxSensors];
  memset(m_arrRawTemps, 0, sizeof(DeviceAddress[m_nMaxSensors]));
}

CTempSensors::~CTempSensors()
{
  if (m_arrRawTemps != NULL)
  {
    delete m_arrRawTemps;
  }

  if (m_arrSensorAddresses != NULL)
  {
    delete m_arrSensorAddresses;
  }
}

void CTempSensors::begin()
{
  discoverSensorAddresses();

  for (int nIndex = 0; nIndex < m_nNumSensors; nIndex++)
  {
    m_sensors.setResolution(m_arrSensorAddresses[nIndex], m_resolution, true);
  }

  m_sensors.begin();
}

char *CTempSensors::addressToString(DeviceAddress deviceAddress, char *pszBuf24, size_t nBufLen /* = 24*/)
{
  if (pszBuf24 != NULL)
  {
    snprintf(pszBuf24,
             nBufLen,
             "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
             deviceAddress[0],
             deviceAddress[1],
             deviceAddress[2],
             deviceAddress[3],
             deviceAddress[4],
             deviceAddress[5],
             deviceAddress[6],
             deviceAddress[7]);
  }

  return pszBuf24;
}

DeviceAddress *CTempSensors::getSensorAddresses()
{
  return m_arrSensorAddresses;
}
uint8_t CTempSensors::getNumSensors()
{
  return m_nNumSensors;
};

void CTempSensors::discoverSensorAddresses()
{
  m_nNumSensors = 0;

  memset(m_arrSensorAddresses, 0, sizeof(DeviceAddress[m_nMaxSensors]));

  DeviceAddress deviceAddress = {};

  memset(deviceAddress, 0, sizeof(deviceAddress));
  m_pOneWire->reset_search();
  m_pOneWire->search(deviceAddress);
  m_pOneWire->reset_search();

  while (m_nNumSensors < m_nMaxSensors && m_pOneWire->search(deviceAddress))
  {
    if (m_sensors.validAddress(deviceAddress))
    {
      if (m_sensors.validFamily(deviceAddress))
      {
        memcpy(m_arrSensorAddresses[m_nNumSensors++], deviceAddress, sizeof(deviceAddress));
      }
    }

    memset(deviceAddress, 0, sizeof(deviceAddress));
  }
}

float CTempSensors::readSensor(uint8_t nIndex)
{
  float fReturn = -999.0;

  if ((nIndex >= 0) && (nIndex < m_nNumSensors))
  {
    const uint8_t *pAddr = m_arrSensorAddresses[nIndex];
    m_sensors.requestTemperaturesByAddress(pAddr);
    fReturn = m_sensors.getTemp(pAddr);
  }

  return fReturn;
}

void CTempSensors::update()
{
  m_sensors.requestTemperatures();

  portENTER_CRITICAL(&m_muxTempData);
  {
    m_fMaxRawTemp = 0.0;
    for (int nIndex = 0; nIndex < m_nNumSensors; nIndex++)
    {
      m_arrRawTemps[nIndex] = m_sensors.getTemp(m_arrSensorAddresses[nIndex]);

      if (m_arrRawTemps[nIndex] > m_fMaxRawTemp)
      {
        m_fMaxRawTemp = m_arrRawTemps[nIndex];
      }
    }
  }
  portEXIT_CRITICAL(&m_muxTempData);
}

float CTempSensors::getTempRaw(uint8_t nIndex)
{
  float fReturn = -999.9;

  if ((nIndex >= 0) && (nIndex < m_nNumSensors))
  {
    portENTER_CRITICAL(&m_muxTempData);
    {
      fReturn = m_arrRawTemps[nIndex];
    }
    portEXIT_CRITICAL(&m_muxTempData);
  }

  return fReturn;
}

float CTempSensors::getTempF(uint8_t nIndex)
{
  return m_sensors.rawToFahrenheit(getTempRaw(nIndex));
}

float CTempSensors::getTempC(uint8_t nIndex)
{
  return m_sensors.rawToCelsius(getTempRaw(nIndex));
}

float CTempSensors::getMaxTempF()
{
  float fReturn = -999.9;

  portENTER_CRITICAL(&m_muxTempData);
  {
    fReturn = m_sensors.rawToFahrenheit(m_fMaxRawTemp);
  }
  portEXIT_CRITICAL(&m_muxTempData);

  return fReturn;
}

float CTempSensors::getMaxTempC()
{
  float fReturn = -999.9;

  portENTER_CRITICAL(&m_muxTempData);
  {
    fReturn = m_sensors.rawToCelsius(m_fMaxRawTemp);
  }
  portEXIT_CRITICAL(&m_muxTempData);

  return fReturn;
}
