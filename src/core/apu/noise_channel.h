#pragma once
#include "core/apu/envelope.h"
#include "type_alias.h"

namespace channels
{
class NoiseChannel
{
public:
  NoiseChannel();

  void clearRegisters();
  void pushCycle();
  void envelopeCycle();
  void disableTimerCycle();

  bool isEnabled() const;
  uint8 getSample() const;
  const channels::Envelope& getEnvelope() const;

  uint8 getFrequencyAndRandomness() const;
  uint8 getControl() const;

  void setTimer(const uint8 value);
  void setVolumeAndEnvelope(const uint8 value);
  void setFrequencyAndRandomness(const uint8 value);
  void setControl(const uint8 value);

private:
  void setPushTimer();
  void trigger();

  static constexpr int maxDisableTimerDuration{64};

  uint8 m_timer;
  uint8 m_frequencyAndRandomness;
  uint8 m_control;

  bool m_enabled;
  uint16 m_lfsr;
  uint8 m_sample;
  uint32 m_pushTimer;
  uint8 m_disableTimer;
  channels::Envelope m_envelope;
};
} //namespace channels
