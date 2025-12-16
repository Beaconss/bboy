#pragma once
#include "type_alias.h"
#include <array>

namespace channels
{
class WaveChannel
{
public:
  WaveChannel();
  void clearRegisters();

  bool isEnabled();
  uint8 getSample();

  void pushCycle();
  void disableTimerCycle();

  uint8 getDacEnable() const;
  uint8 getOutLevel() const;
  uint8 getPeriodHighAndControl() const;
  uint8 getWaveRam(const uint8 index) const;

  void setDacEnable(const uint8 value);
  void setTimer(const uint8 value);
  void setOutLevel(const uint8 value);
  void setPeriodLow(const uint8 value);
  void setPeriodHighAndControl(const uint8 value);
  void setWaveRam(const uint8 value, uint8 index);

private:
  void trigger();
  void setPushTimer();

  static constexpr uint16 maxPeriod{0x7FF};
  static constexpr int maxDisableTimerDuration{256};
  static constexpr uint8 dacBit{0x80};
  static constexpr uint8 volumeBits{0b0110'0000};

  uint8 m_dacEnable;
  uint8 m_timer;
  uint8 m_outLevel;
  uint8 m_periodLow;
  uint8 m_periodHighAndControl;
  std::array<uint8, 16> m_waveRam;

  bool m_enabled;
  uint16 m_pushTimer;
  uint8 m_sample;
  uint8 m_waveIndex;
  uint16 m_disableTimer;
};
} // namespace channels
