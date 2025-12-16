#include "core/apu/noise_channel.h"
#include <array>

channels::NoiseChannel::NoiseChannel()
  : m_timer{0xFF}
  , m_frequencyAndRandomness{}
  , m_control{0xBF}
  , m_enabled{}
  , m_lfsr{}
  , m_sample{}
  , m_pushTimer{1}
  , m_disableTimer{1}
  , m_envelope{}
{
}

void channels::NoiseChannel::clearRegisters()
{
  m_envelope.setVolumeAndEnvelope(0);
  m_frequencyAndRandomness = 0;
  m_control = 0;
}

void channels::NoiseChannel::pushCycle()
{
  if(m_pushTimer == 0) return;
  else if(--m_pushTimer > 0) return;

  setPushTimer();

  const uint8 result{static_cast<uint8>((m_lfsr & 1) == ((m_lfsr >> 1) & 1))};
  m_lfsr = (m_lfsr & 0x7FFF) | (result << 15);
  if(constexpr uint8 shortModeBit{0x8}; m_frequencyAndRandomness & shortModeBit)
  {
    m_lfsr = (m_lfsr & 0xFF7F) | (result << 7);
  }
  m_lfsr >>= 1;
  m_sample = (m_lfsr & 1);
}

void channels::NoiseChannel::envelopeCycle()
{
  m_envelope.cycle();
}

void channels::NoiseChannel::disableTimerCycle()
{
  if(constexpr uint8 disableTimerBit{0b0100'0000}; !(m_control & disableTimerBit) || m_disableTimer == 0) return;

  if(--m_disableTimer == 0) m_enabled = false;
}

bool channels::NoiseChannel::isEnabled() const
{
  return m_enabled;
}

uint8 channels::NoiseChannel::getSample() const
{
  if(m_envelope.dac()) return m_sample * m_enabled * m_envelope.getVolume();
  else return 7;
}

const channels::Envelope& channels::NoiseChannel::getEnvelope() const
{
  return m_envelope;
}

uint8 channels::NoiseChannel::getFrequencyAndRandomness() const
{
  return m_frequencyAndRandomness;
}

uint8 channels::NoiseChannel::getControl() const
{
  return m_control | 0b1011'1111;
}

void channels::NoiseChannel::setTimer(const uint8 value)
{
  m_timer = value;
  constexpr uint8 timerBits{0b0011'1111};
  m_disableTimer = maxDisableTimerDuration - (m_timer & timerBits);
}

void channels::NoiseChannel::setVolumeAndEnvelope(const uint8 value)
{
  m_envelope.setVolumeAndEnvelope(value);
  if(!(m_envelope.dac())) m_enabled = false;
}

void channels::NoiseChannel::setFrequencyAndRandomness(const uint8 value)
{
  m_frequencyAndRandomness = value;
  setPushTimer();
}

void channels::NoiseChannel::setControl(const uint8 value)
{
  m_control = value;
  if(constexpr uint8 triggerBit{0x80}; m_control & triggerBit && m_envelope.dac()) trigger();
}

void channels::NoiseChannel::setPushTimer()
{
  constexpr uint8 shiftBits{0xF0};
  const uint8 shift{static_cast<uint8>((m_frequencyAndRandomness & shiftBits) >> 4)};
  if(shift > 13)
  {
    m_pushTimer = 0;
    return;
  }

  constexpr std::array<uint8, 8> dividerValues{8, 16, 32, 48, 64, 80, 96, 112};
  constexpr uint8 dividerBits{0x7};
  const uint8 divider{dividerValues[m_frequencyAndRandomness & dividerBits]};

  m_pushTimer = divider << shift;
}

void channels::NoiseChannel::trigger()
{
  m_enabled = true;
  m_lfsr = 0;
  if(m_disableTimer == 0) m_disableTimer = maxDisableTimerDuration;
  constexpr uint8 volumeBits{0xF0};
  m_envelope.trigger();
}
