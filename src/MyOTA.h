// Adapted from https://github.com/SensorsIot/ESP32-OTA

#ifndef __MYOTA_H__
#define __MYOTA_H__

#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#define ESP32_RTOS
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>

#define USE_TELNETSTREAM

#ifndef USE_TELNETSTREAM
#define MySerial Serial
#else
#define MySerial TelnetStream
#endif

#if defined(ESP32_RTOS) && defined(ESP32)
void taskOTAHandle(void *parameter)
{
  for (;;)
  {
    ArduinoOTA.handle();
    delay(3500);
  }
  vTaskDelete(NULL);
}
#endif

const char *computeHostname(const char *pszPrefix, char *pszOutput, size_t nBufLen = 40)
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  const char *szPrefixDefault = "OTA-Target";
  if (pszPrefix == NULL)
  {
    pszPrefix = szPrefixDefault;
  }
  snprintf(pszOutput, nBufLen, "%s-%02x%02x%02x", pszPrefix, mac[3], mac[4], mac[5]);
  return pszOutput;
}

void setupOTA(const char *nameprefix)
{
  //const int maxlen =
  if (nameprefix == NULL)
  {
    const char *pszHostName = WiFi.getHostname();
    if (pszHostName != NULL)
    {
      ArduinoOTA.setHostname(pszHostName);
    }
  }
  else
  {
    char fullhostname[40];
    // uint8_t mac[6];
    // WiFi.macAddress(mac);
    // snprintf(fullhostname, maxlen, "%s-%02x%02x%02x", nameprefix, mac[3], mac[4], mac[5]);
    ArduinoOTA.setHostname(computeHostname(nameprefix, fullhostname, sizeof(fullhostname)));
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();
  TelnetStream.begin();

  Serial.println("OTA Initialized");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

#if defined(ESP32_RTOS) && defined(ESP32)
  xTaskCreate(
      taskOTAHandle,   /* Task function. */
      "taskOTAHandle", /* String with name of task. */
      10000,           /* Stack size in bytes. */
      NULL,            /* Parameter passed as input of the task */
      1,               /* Priority of the task. */
      NULL);           /* Task handle. */
#endif
}

#endif // __MYOTA_H__
