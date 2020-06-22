#ifndef __CCONTROLLERSERVER_H__
#define __CCONTROLLERSERVER_H__

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <CPwmFanControl.h>
#include <CTempSensors.h>

class CControllerServer
{
public:
  CControllerServer(uint8_t nPort = 80);
  ~CControllerServer();

  void begin(CPwmFanControl **arrFanCtrl, size_t nNumFans, CTempSensors *pTempSensors);

protected:
  void onReqStatus(AsyncWebServerRequest *pRequest);

  void setReponseHeaders(AsyncWebServerResponse *pResponse);

  CPwmFanControl *getFanCtrl(uint8_t nIndex = 0);

private:
  CPwmFanControl **m_arrFanCtrl = NULL;
  size_t m_nNumFans = 0;
  CTempSensors *m_pTempSensors = NULL;
  AsyncWebServer m_server;
};

#endif // #ifndef __CCONTROLLERSERVER_H__
