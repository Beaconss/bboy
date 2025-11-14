#include "core/apu/pulse_channels.h"
#include <iostream>

channels::PulseChannelBase::PulseChannelBase()
    : m_timerAndDuty{}
	, m_periodLow{0xFF}
	, m_periodHighAndControl{}
	, m_enabled{}
	, m_sample{}
	, m_pushTimer{static_cast<uint16>((maxPeriod + 1) - getPeriod())}
	, m_disableTimer{1}
	, m_dutyStep{}
	, m_envelope{}
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
	trigger();
}

channels::PulseChannel::PulseChannel()
{
    setTimerAndDuty(0x3F);
    setVolumeAndEnvelope(0);
    setPeriodHighAndControl(0xBF);
	trigger();
}

void channels::PulseChannelBase::clearRegisters()
{
    m_envelope.setVolumeAndEnvelope(0);
	m_periodLow = 0;
	m_periodHighAndControl = 0;
	m_dutyStep = 0;
}

void channels::PulseChannelBase::pushCycle()
{
	if(m_pushTimer == 0) return;
	else if(--m_pushTimer > 0) return;

	setPushTimer();
	constexpr uint8 dutyPattern{0b1100'0000};
	m_sample = dutyPatterns[m_timerAndDuty & (dutyPattern >> 6)][m_dutyStep];
	++m_dutyStep;
	m_dutyStep &= maxDutyStep;
}

void channels::PulseChannelBase::envelopeCycle()
{
	m_envelope.cycle();
}

void channels::PulseChannelBase::disableTimerCycle()
{
	if(!(m_periodHighAndControl & disableTimerBit) || m_disableTimer == 0) return; 
	
	if(--m_disableTimer == 0) disable();
}

bool channels::PulseChannelBase::isEnabled() const
{
    return m_enabled;
}

uint8 channels::PulseChannelBase::getSample() const
{
    if(m_envelope.dac()) return m_sample * m_enabled * m_envelope.getVolume();
    else return 7;
}

const channels::Envelope& channels::PulseChannelBase::getEnvelope() const
{
	return m_envelope;
}

uint8 channels::PulseChannelBase::getTimerAndDuty() const
{
    return m_timerAndDuty | 0b0011'1111;
}

uint8 channels::PulseChannelBase::getPeriodHighAndControl() const
{
    return m_periodHighAndControl | 0b1011'1111;
}

void channels::PulseChannelBase::setTimerAndDuty(const uint8 value)
{
	m_timerAndDuty = value;
	static constexpr uint8 timerBits{0b0011'1111};
	m_disableTimer = maxDisableTimerDuration - (m_timerAndDuty & timerBits);
}

void channels::PulseChannelBase::setVolumeAndEnvelope(const uint8 value)
{
	m_envelope.setVolumeAndEnvelope(value);
	if(!(m_envelope.dac())) disable();
}

void channels::PulseChannelBase::setPeriodLow(const uint8 value)
{
    m_periodLow = value;
}

void channels::PulseChannelBase::setPeriodHighAndControl(const uint8 value)
{
	m_periodHighAndControl = value;
	if(m_periodHighAndControl & triggerBit && m_envelope.dac()) trigger();
}

void channels::PulseChannelBase::trigger()
{
	m_enabled = true;
	setPushTimer();
	if(m_disableTimer == 0) m_disableTimer = maxDisableTimerDuration; 
	m_envelope.trigger();
}

void channels::PulseChannelBase::disable()
{
	m_enabled = false;
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
	else disable();
}