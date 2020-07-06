#ifndef __CPWMFANCONTROL_H__
#define __CPWMFANCONTROL_H__

#include <Arduino.h>

typedef uint8_t dutycycle_t;

class CPwmFanControl
{
public:
  CPwmFanControl(const uint8_t nPwmChannel,
                 const uint8_t nPinFanPwm,
                 const uint8_t nPinFanTach,
                 const dutycycle_t nMinFanDutyCycle = 0.0,
                 const dutycycle_t nFanOffDutyCycle = 0.0,
                 const uint8_t bAllowOff = 1);

  void begin(void (*pHandleFanTackIrq)());

  void IRAM_ATTR isrFanTach();

  void setMinFanDutyCycle(const dutycycle_t nMinFanDutyCycle);
  void setFanOffDutyCycle(const dutycycle_t nFanOffDutyCycle);
  void setAllowOff(const uint8_t bAllowOff);

  void setFanDutyCycle(const dutycycle_t nDutyCycle);
  void setFanDutyCyclePercent(const double fDutyPercent);

  dutycycle_t getLastDutyCycle();
  dutycycle_t getLastSpecDutyCycle();

  double getLastDutyCyclePercent();

  void
  setFullSpeed();

  uint32_t getFanRpms();

  static uint8_t percentToDutyCycle(const double fDutyPercent);
  static double dutyCycleToPercent(const dutycycle_t nDutyCycle);

  static const double PWM_FREQUENCY;
  static const uint8_t PWM_RESOUTION;

private:
  portMUX_TYPE m_muxFanIrqCounter = portMUX_INITIALIZER_UNLOCKED;
  volatile uint32_t m_nFanTackIrqCounter = 0;
  volatile u_long m_nFanTackCounterLastReadMicros = 0;
  uint8_t m_nPwmChannel = 0; // this variable is used to select the channel number
  uint8_t m_nPinFanPwm = 0;  // GPIO to which we want to attach this channel signal
  uint8_t m_nPinFanTach = 0;
  dutycycle_t m_nMinFanDutyCycle = 0;
  dutycycle_t m_nFanOffDutyCycle = 0;
  uint8_t m_bAllowOff = 1;
  dutycycle_t m_nLastDutyCycle = 0;
  dutycycle_t m_nLastSpecDutyCycle = 0;
};

#endif // #ifndef __CPWMFANCONTROL_H__
