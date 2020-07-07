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

const char *computeHostname(const char *pszPrefix, char *pszOutput, size_t nBufLen = 40);

void setupOTA(const char *nameprefix);

#endif // __MYOTA_H__
