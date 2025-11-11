#include "core/apu/pulse_channels.h"
#include <iostream>

channels::PulseChannelBase::PulseChannelBase()
    : m_timerAndDuty{}
	, m_volumeAndEnvelope{}
	, m_periodLow{0xFF}
	, m_periodHighAndControl{}
	, m_enabled{}
	, m_sample{}
	, m_disableTimer{1}
	, m_pushTimer{static_cast<uint16>((maxPeriod + 1) - getPeriod())}
	, m_dutyStep{}
	, m_volume{}
	, m_envelopeTimer{}
	, m_envelopeDir{}
{
	trigger();
}

channels::SweepPulseChannel::SweepPulseChannel()
    : m_sweep{}
    , m_shadowPeriod{}
    , m_sweepTimer{}
{
    setTimerAndDuty(0xBF);
    setVolumeAndEnvelope(0xF3);
    setPeriodHighAndControl(0xBF);
    m_shadowPeriod = getPeriod();
    m_volume = 0xF;
    m_envelopeTimer = 0x3;
}

channels::PulseChannel::PulseChannel()
{
    setTimerAndDuty(0x3F);
    setVolumeAndEnvelope(0);
    setPeriodHighAndControl(0xBF);
}

void channels::PulseChannelBase::clearRegisters()
{
    m_volumeAndEnvelope = 0;
	m_periodLow = 0;
	m_periodHighAndControl = 0;
	m_dutyStep = 0;
}

void channels::PulseChannelBase::pushCycle()
{
	if(--m_pushTimer > 0) return;

	setPushTimer();
	constexpr uint8 dutyPattern{0b1100'0000};
	m_sample = dutyPatterns[m_timerAndDuty & (dutyPattern >> 6)][m_dutyStep];
	++m_dutyStep;
	m_dutyStep &= maxDutyStep;
}

void channels::PulseChannelBase::disableTimerCycle()
{
	if(!(m_periodHighAndControl & disableTimerBit) || m_disableTimer == 0) return; 
	
	if(--m_disableTimer == 0) m_enabled = false;
}

void channels::PulseChannelBase::envelopeCycle()
{
	if(m_envelopeTimer == 0  || (m_volumeAndEnvelope & envelopeTargetBits) == 0) return;
	if(--m_envelopeTimer > 0) return;

	uint8 newVolume{m_volume};
	if(envelopeDirBit) ++newVolume;
	else --newVolume;
	
	constexpr uint8 maxVolume{0xF};
	if(newVolume <= maxVolume) m_volume = newVolume;
}

bool channels::PulseChannelBase::isEnabled() const
{
    return m_enabled;
}

uint8 channels::PulseChannelBase::getSample() const
{
    if(m_volumeAndEnvelope & dacBits) return m_enabled ? m_sample * m_volume : 0;
    else return 7;
}

uint8 channels::PulseChannelBase::getTimerAndDuty() const
{
    return m_timerAndDuty | timerBits;
}

uint8 channels::PulseChannelBase::getVolumeAndEnvelope() const
{
    return m_volumeAndEnvelope;
}

uint8 channels::PulseChannelBase::getPeriodHighAndControl() const
{
    return m_periodHighAndControl | triggerBit | 0b11'1000 | 0b111;
}

void channels::PulseChannelBase::setTimerAndDuty(const uint8 value)
{
	m_timerAndDuty = value;
	m_disableTimer = maxDisableTimerDuration - (m_timerAndDuty & timerBits);
}

void channels::PulseChannelBase::setVolumeAndEnvelope(const uint8 value)
{
	m_volumeAndEnvelope = value;
	if(!(m_volumeAndEnvelope & dacBits)) m_enabled = false;
}

void channels::PulseChannelBase::setPeriodLow(const uint8 value)
{
    m_periodLow = value;
}

void channels::PulseChannelBase::setPeriodHighAndControl(const uint8 value)
{
	m_periodHighAndControl = value;
	if(m_periodHighAndControl & triggerBit && (m_volumeAndEnvelope & dacBits)) trigger();
}

void channels::PulseChannelBase::trigger()
{
	m_enabled = true;
	if(m_disableTimer == 0) m_disableTimer = maxDisableTimerDuration; 
	setPushTimer();
	
	m_volume = (m_volumeAndEnvelope & volumeBits) >> 4;
	m_envelopeTimer = m_volumeAndEnvelope & envelopeTargetBits;
	m_envelopeDir = static_cast<bool>(m_volumeAndEnvelope & envelopeDirBit);
}

uint16 channels::PulseChannelBase::getPeriod() const
{
	constexpr uint8 periodHighBits{0x7};
	return m_periodLow | ((m_periodHighAndControl & periodHighBits) << 8);
}

void channels::PulseChannelBase::setPeriod(const uint16 value)
{
	m_periodLow = value & 0xFF;
	m_periodHighAndControl = ((m_periodHighAndControl & triggerBit) & disableTimerBit) | ((value >> 8) & 0b111);
}

void channels::PulseChannelBase::setPushTimer()
{
	m_pushTimer = ((maxPeriod + 1) - getPeriod());
}


void channels::SweepPulseChannel::clearRegisters()
{
    channels::PulseChannelBase::clearRegisters();
    m_sweep = 0;
}

void channels::SweepPulseChannel::sweepCycle()
{
	if(m_sweepTimer == 0 || (m_sweep & sweepTargetBits) == 0) return;
	if(--m_sweepTimer > 0) return;
	
	m_sweepTimer = (m_sweep & sweepTargetBits) >> 4;
	sweepIteration();
}

uint8 channels::SweepPulseChannel::getSweep() const
{
    return m_sweep | 0x80;
}

void channels::SweepPulseChannel::setSweep(uint8 value)
{
	m_sweep = value;
}

void channels::SweepPulseChannel::trigger()
{
	channels::PulseChannelBase::trigger();
	m_shadowPeriod = getPeriod();
	m_sweepTimer = (m_sweep & sweepTargetBits) >> 4;
	if(!(m_sweep & sweepStepBits)) return;

	sweepIteration();
}

void channels::SweepPulseChannel::sweepIteration()
{
	uint16 newPeriod{};	
	if(m_sweep & subtractionBit)
	{
		newPeriod = m_shadowPeriod - (m_shadowPeriod >> (m_sweep & sweepStepBits));
		if(newPeriod > maxPeriod) newPeriod = 0; //underflow check
	}
	else newPeriod = m_shadowPeriod + (m_shadowPeriod >> (m_sweep & sweepStepBits));

	if(newPeriod <= maxPeriod)
	{
		if(!(m_sweep & sweepStepBits)) return;
		m_shadowPeriod = newPeriod;
		setPeriod(newPeriod);
	}
	else m_enabled = false;
}