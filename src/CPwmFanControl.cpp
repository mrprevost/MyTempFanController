#include <CPwmFanControl.h>
#include <limits>

const double CPwmFanControl::PWM_FREQUENCY = 25000.0;
const uint8_t CPwmFanControl::PWM_RESOUTION = 8;

CPwmFanControl::CPwmFanControl(const uint8_t nPwmChannel,
                               const uint8_t nPinFanPwm,
                               const uint8_t nPinFanTach,
                               const dutycycle_t nMinFanDutyCycle /* = 0.0*/,
                               const dutycycle_t nFanOffDutyCycle /* = 0.0*/,
                               const uint8_t bAllowOff /*= 1*/)
{
  m_muxFanIrqCounter = portMUX_INITIALIZER_UNLOCKED;

  m_nPwmChannel = nPwmChannel;
  m_nPinFanPwm = nPinFanPwm;
  m_nPinFanTach = nPinFanTach;
  m_nMinFanDutyCycle = nMinFanDutyCycle;
  m_nFanOffDutyCycle = nFanOffDutyCycle;
  m_bAllowOff = bAllowOff;
}

void CPwmFanControl::setMinFanDutyCycle(const dutycycle_t nMinFanDutyCycle)
{
  m_nMinFanDutyCycle = nMinFanDutyCycle;
}

void CPwmFanControl::setFanOffDutyCycle(const dutycycle_t nFanOffDutyCycle)
{
  m_nFanOffDutyCycle = nFanOffDutyCycle;
}

void CPwmFanControl::setAllowOff(const uint8_t bAllowOff)
{
  m_bAllowOff = bAllowOff;
}

void CPwmFanControl::begin(void (*pHandleFanTackIrq)())
{
  m_muxFanIrqCounter = portMUX_INITIALIZER_UNLOCKED;

  ledcSetup(m_nPwmChannel, PWM_FREQUENCY, PWM_RESOUTION);
  ledcWrite(m_nPwmChannel, 0);
  ledcAttachPin(m_nPinFanPwm, m_nPwmChannel);

  if (pHandleFanTackIrq != NULL)
  {
    pinMode(m_nPinFanTach, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(m_nPinFanTach),
                    (void (*)())pHandleFanTackIrq,
                    FALLING);
  }
}

void IRAM_ATTR CPwmFanControl::isrFanTach()
{
  portENTER_CRITICAL_ISR(&m_muxFanIrqCounter);
  {
    m_nFanTackIrqCounter++;
  }
  portEXIT_CRITICAL_ISR(&m_muxFanIrqCounter);
}

void CPwmFanControl::setFanDutyCycle(const dutycycle_t nDutyCycle)
{
  m_nLastSpecDutyCycle = nDutyCycle;

  uint8_t nTmpDutyCycle = nDutyCycle;
  if (nTmpDutyCycle <= m_nFanOffDutyCycle)
  {
    nTmpDutyCycle = 0;
  }
  else
  {
    nTmpDutyCycle = max(nTmpDutyCycle, m_nMinFanDutyCycle);
  }

  if (!m_bAllowOff && nTmpDutyCycle <= 0)
  {
    nTmpDutyCycle = m_nMinFanDutyCycle;
  }

  m_nLastDutyCycle = nTmpDutyCycle;
  ledcWrite(m_nPwmChannel, nTmpDutyCycle);
}

void CPwmFanControl::setFanDutyCyclePercent(const double fDutyPercent)
{
  setFanDutyCycle(percentToDutyCycle(fDutyPercent));
}

void CPwmFanControl::setFullSpeed()
{
  setFanDutyCycle((dutycycle_t)pow(2.0, (double)PWM_RESOUTION));
}

dutycycle_t CPwmFanControl::getLastDutyCycle()
{
  return m_nLastDutyCycle;
}

double CPwmFanControl::getLastDutyCyclePercent()
{
  return CPwmFanControl::dutyCycleToPercent(m_nLastDutyCycle);
}

dutycycle_t CPwmFanControl::getLastSpecDutyCycle()
{
  return m_nLastSpecDutyCycle;
}

uint32_t CPwmFanControl::getFanRpms()
{
  uint32_t nReturn = 0;

  uint32_t nPrevFanTackIrqCounter = 0;
  uint32_t nPrevFanTackCounterLastReadMicros = 0;

  portENTER_CRITICAL(&m_muxFanIrqCounter);
  {
    nPrevFanTackIrqCounter = m_nFanTackIrqCounter;
    nPrevFanTackCounterLastReadMicros = m_nFanTackCounterLastReadMicros;
    m_nFanTackIrqCounter = 0;
    m_nFanTackCounterLastReadMicros = micros();
  }
  portEXIT_CRITICAL(&m_muxFanIrqCounter);

  if ((nPrevFanTackIrqCounter > 0) && (nPrevFanTackCounterLastReadMicros > 0))
  {
    double fNumFanRevs = ((double)nPrevFanTackIrqCounter) / 2.0;
    double nNumMicros = 0;

    // Handle rollover
    if (nPrevFanTackCounterLastReadMicros > m_nFanTackCounterLastReadMicros)
    {
      nNumMicros = std::numeric_limits<double>::max() - nPrevFanTackCounterLastReadMicros + m_nFanTackCounterLastReadMicros;
    }
    else
    {
      nNumMicros = (double)m_nFanTackCounterLastReadMicros - (double)nPrevFanTackCounterLastReadMicros;
    }

    double fFanTimeSec = nNumMicros / 1000000.0;

    double fFanFreqHz = fNumFanRevs / fFanTimeSec;
    double fFanRpms = fFanFreqHz * 60.0;
    nReturn = round(fFanRpms);
  }

  return nReturn;
}

dutycycle_t CPwmFanControl::percentToDutyCycle(double fDutyPercent)
{
  dutycycle_t nReturn = 0;

  if (fDutyPercent < 0.0)
  {
    fDutyPercent = 0.0;
  }
  if (fDutyPercent > 100.0)
  {
    fDutyPercent = 100.0;
  }

  double fMin = 0.0;
  double fMax = pow(2.0, PWM_RESOUTION);
  dutycycle_t nDutyCycle = min(round((fMax - fMin) * (fDutyPercent / 100.0)), fMax - 1.0);

  nReturn = nDutyCycle;

  return nReturn;
}

double CPwmFanControl::dutyCycleToPercent(const dutycycle_t nDutyCycle)
{
  double fMax = pow(2.0, PWM_RESOUTION) - 1.0;
  return ((double)nDutyCycle / fMax) * 100.0;
}
