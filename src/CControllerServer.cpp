#include <CControllerServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

CControllerServer::CControllerServer(uint8_t nPort /*= 80*/)
    : m_server(nPort)
{
}

CControllerServer::~CControllerServer()
{
  if (m_arrFanCtrl != NULL)
  {
    delete m_arrFanCtrl;
    m_arrFanCtrl = NULL;
  }
}

void CControllerServer::begin(CPwmFanControl **arrFanCtrl, size_t nNumFans, CTempSensors *pTempSensors)
{
  m_nNumFans = nNumFans;

  if ((arrFanCtrl != NULL) && (m_nNumFans > 0))
  {
    m_arrFanCtrl = (CPwmFanControl **)new CPwmFanControl *[nNumFans] {};
    for (uint8_t nIndex = 0; nIndex < nNumFans; nIndex++)
    {
      m_arrFanCtrl[nIndex] = arrFanCtrl[nIndex];
    }
  }

  m_pTempSensors = pTempSensors;

  m_server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *pRequest) {
    onReqStatus(pRequest);
  });

  m_server.begin();
}

CPwmFanControl *CControllerServer::getFanCtrl(uint8_t nIndex /* = 0*/)
{
  CPwmFanControl *pReturn = NULL;

  if ((m_pTempSensors != NULL) && (nIndex >= 0) && (nIndex < m_nNumFans))
  {
    pReturn = m_arrFanCtrl[nIndex];
  }

  return pReturn;
}

void CControllerServer::setReponseHeaders(AsyncWebServerResponse *pResponse)
{
  if (pResponse != NULL)
  {
    // Add no cache headers
    pResponse->addHeader("Cache-Control", "private, no-cache, no-store, must-revalidate, no-transform");
    pResponse->addHeader("Expires", "Thu, 01 Jan 1970 00:00:00 GMT");
    pResponse->addHeader("Pragma", "no-cache");
    // Add CORS headers
    pResponse->addHeader("Access-Control-Allow-Credentials", "true");
    pResponse->addHeader("Access-Control-Allow-Headers", "*");
    pResponse->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    pResponse->addHeader("Access-Control-Allow-Origin", "*");
    pResponse->addHeader("Access-Control-Max-Age", "86400");
  }
}

void CControllerServer::onReqStatus(AsyncWebServerRequest *pRequest)
{
  if (pRequest != NULL)
  {
    AsyncJsonResponse *pResponse = new AsyncJsonResponse();

    setReponseHeaders(pResponse);

    // Create response JSON
    const JsonObject &root = pResponse->getRoot();
    const JsonArray &arrFans = root.createNestedArray("fans");
    for (uint8_t nIndex = 0; nIndex < m_nNumFans; nIndex++)
    {
      CPwmFanControl *pFanCtrl = getFanCtrl(nIndex);
      if (pFanCtrl != NULL)
      {
        const JsonObject &objFan1 = arrFans.createNestedObject();
        objFan1["rpm"] = pFanCtrl->getFanRpms();
        objFan1["duty"] = pFanCtrl->getLastDutyCyclePercent();
      }
    }
    const JsonObject &objTempSensors = root.createNestedObject("tempSensors");
    objTempSensors["maxTempF"] = -999.0;
    objTempSensors["maxTempC"] = -999.0;
    const JsonArray &arrTempSensors = objTempSensors.createNestedArray("sensors");
    if (m_pTempSensors != NULL)
    {
      objTempSensors["maxTempF"] = m_pTempSensors->getMaxTempF();
      objTempSensors["maxTempC"] = m_pTempSensors->getMaxTempC();
      uint8_t nNumTempSensors = m_pTempSensors->getNumSensors();
      for (int nIndex = 0; nIndex < nNumTempSensors; nIndex++)
      {
        const JsonObject &objTemp = arrTempSensors.createNestedObject();
        objTemp["tempF"] = m_pTempSensors->getTempF(nIndex);
        objTemp["tempC"] = m_pTempSensors->getTempC(nIndex);
      }
    }

    pResponse->setLength();
    pRequest->send(pResponse);
  }
}
