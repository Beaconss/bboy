#include "core/apu/noise_channel.h"
#include <iostream>
#include <cmath>

channels::NoiseChannel::NoiseChannel()
    : m_timer{0xFF}
    , m_volumeAndEnvelope{}
    , m_frequencyAndRandomness{}
    , m_control{0xBF}
    , m_enabled{}
    , m_lfsr{}
    , m_sample{}
    , m_pushTimer{1}
    , m_disableTimer{1}
    , m_volume{}
    , m_envelopeTimer{}
{}

void channels::NoiseChannel::clearRegisters()
{
    m_volumeAndEnvelope = 0;
    m_frequencyAndRandomness = 0;
    m_control = 0;
}

void channels::NoiseChannel::pushCycle()
{
    if(m_pushTimer == 0) return;
    else if(--m_pushTimer > 0) return;
    
    setPushTimer();
    
    uint8 result{static_cast<uint8>((m_lfsr & 1) == ((m_lfsr >> 1) & 1))};
    m_lfsr = (m_lfsr & 0x7FFF) | (result << 15);
    if(constexpr uint8 shortModeBit{0x8}; m_frequencyAndRandomness & shortModeBit)
    {
        m_lfsr = (m_lfsr & 0xFF7F) | (result << 7);
    }
    m_lfsr >>= 1;
    m_sample = m_lfsr & 1;
}

void channels::NoiseChannel::disableTimerCycle()
{
    if(constexpr uint8 disableTimerBit{0b0100'0000}; !(m_control & disableTimerBit) || m_disableTimer == 0) return; 
	
	if(--m_disableTimer == 0) m_enabled = false;
}

void channels::NoiseChannel::envelopeCycle()
{
    if(m_envelopeTimer == 0  || !(m_volumeAndEnvelope & envelopeTargetBits)) return;
	if(--m_envelopeTimer > 0) return;

	uint8 newVolume{m_volume};
	if(m_envelopeDir) ++newVolume;
	else --newVolume;
	
	constexpr uint8 maxVolume{0xF};
	if(newVolume <= maxVolume) m_volume = newVolume;
}

bool channels::NoiseChannel::isEnabled() const
{
    return m_enabled;
}

uint8 channels::NoiseChannel::getSample() const
{
    if(m_volumeAndEnvelope & dacBits) return m_sample * m_enabled * m_volume;
    else return 7;
}

uint8 channels::NoiseChannel::getVolumeAndEnvelope() const
{
    return m_volumeAndEnvelope;
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
    m_volumeAndEnvelope = value;
    if(!(m_volumeAndEnvelope & dacBits)) m_enabled = false;
}

void channels::NoiseChannel::setFrequencyAndRandomness(const uint8 value)
{
    m_frequencyAndRandomness = value;
    setPushTimer();
}

void channels::NoiseChannel::setControl(const uint8 value)
{
    m_control = value;
    if(constexpr uint8 triggerBit{0x80}; m_control & triggerBit && (m_volumeAndEnvelope & dacBits)) trigger();
}

void channels::NoiseChannel::setPushTimer()
{
    constexpr uint8 shiftBits{0xF0};
    uint8 shift{static_cast<uint8>((m_frequencyAndRandomness & shiftBits) >> 4)};
    if(shift > 13)
    {
        m_pushTimer = 0;
        return;
    }

    constexpr uint8 dividerBits{3};
    uint8 divider{static_cast<uint8>(m_frequencyAndRandomness & dividerBits)};

    constexpr uint32 hz{0x40000};
    if(divider > 0) m_pushTimer = hz / pow(divider * 2, shift); 
    else m_pushTimer = hz;
}

void channels::NoiseChannel::trigger()
{
    m_enabled = true;
    m_lfsr = 0;
    if(m_disableTimer == 0) m_disableTimer = maxDisableTimerDuration;
    constexpr uint8 volumeBits{0xF0};
    m_volume = (m_volumeAndEnvelope & volumeBits) >> 4;
	m_envelopeTimer = m_volumeAndEnvelope & envelopeTargetBits;
	m_envelopeDir = static_cast<bool>(m_volumeAndEnvelope & envelopeDirBit);
    
    if(m_volumeAndEnvelope == 0xC6)
    {
        std::cout << "VOLUME: " << (int)m_volume 
                << "\nENVELOPE TIMER: " << (int)m_envelopeTimer
                << "\nENVELOPE DIR: " << (m_envelopeDir ? "POSITIVE" : "NEGATIVE") << '\n';
    }
} 