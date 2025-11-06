#include "core/apu/pulse_channels.h"

PulseChannel::PulseChannel()
    : m_timerAndDuty{}
	, m_volumeAndEnvelope{}
	, m_periodLow{0xFF}
	, m_periodHighAndControl{}
	, m_enabled{}
	, m_dac{}
	, m_sample{}
	, m_disableTimer{1}
	, m_pushTimer{((maxPeriod + 1) - maxPeriod)}
	, m_dutyStep{}
	, m_volume{}
	, m_envelopeTarget{}
	, m_envelopeCounter{}
	, m_envelopeDir{}
{
	trigger();
}

Channel1::Channel1()
    : m_sweep{}
    , m_shadowPeriod{}
    , m_sweepTarget{}
    , m_sweepCounter{}
    , m_sweepStep{}
    , m_sweepEnabled{}
{
    //use setters here and in channel 2 constructor
    setTimerAndDuty(0xBF);
    setVolumeAndEnvelope(0xF3);
    setPeriodHighAndControl(0xBF);
    m_shadowPeriod = getPeriod();
    m_dac = true;
    m_volume = 0xF;
    m_envelopeTarget = 0x3;
}

Channel2::Channel2()
{
    setTimerAndDuty(0x3F);
    setVolumeAndEnvelope(0);
    setPeriodHighAndControl(0xBF);
}

void PulseChannel::clearRegisters()
{
    m_volumeAndEnvelope = 0;
	m_periodLow = 0;
	m_periodHighAndControl = 0;
	m_dutyStep = 0;
}

void Channel1::clearRegisters()
{
    PulseChannel::clearRegisters();
    m_sweep = 0;
}

void PulseChannel::pushCycle()
{
	if(--m_pushTimer > 0) return;

	setPushTimer();
	constexpr uint8 dutyPattern{0b1100'0000};
	m_sample = dutyPatterns[m_timerAndDuty & (dutyPattern >> 6)][m_dutyStep] * m_volume;
	++m_dutyStep;
	m_dutyStep &= maxDutyStep;
}

void PulseChannel::disableTimerCycle()
{
	if(!(m_periodHighAndControl & disableTimer) || m_disableTimer == 0) return; 
	
	if(--m_disableTimer == 0) m_enabled = false;
}

void PulseChannel::envelopeCycle()
{
	if(m_envelopeTarget == 0) return;
	if(++m_envelopeCounter < m_envelopeTarget) return;
	
	m_envelopeCounter = 0;
	if(envelopeDir) 
    {
        constexpr uint8 maxVolume{0xF};
        if(m_volume < maxVolume) ++m_volume;
    }
	else if(m_volume > 0) --m_volume; 
}

void Channel1::sweepCycle()
{
	
}

bool PulseChannel::isEnabled() const
{
    return m_enabled;
}

uint8 PulseChannel::getSample() const
{
    if(m_dac) return m_enabled ? m_sample : 0;
    else return 7;
}

uint8 Channel1::getSweep() const
{
    return m_sweep | 0x80;
}

uint8 PulseChannel::getTimerAndDuty() const
{
    return m_timerAndDuty | timer;
}

uint8 PulseChannel::getVolumeAndEnvelope() const
{
    return m_volumeAndEnvelope;
}

uint8 PulseChannel::getPeriodHighAndControl() const
{
    return m_periodHighAndControl | disableTimer;
}

void Channel1::setSweep(uint8 value)
{
	m_sweep = value;
}

void PulseChannel::setTimerAndDuty(const uint8 value)
{
	m_timerAndDuty = value;
	m_disableTimer = maxDisableTimerDuration - (m_timerAndDuty & timer);
}

void PulseChannel::setVolumeAndEnvelope(const uint8 value)
{
	m_volumeAndEnvelope = value;
	m_dac = m_volumeAndEnvelope & (volumeBits | envelopeDir);
	if(!m_dac) m_enabled = false;
}

void PulseChannel::setPeriodLow(const uint8 value)
{
    m_periodLow = value;
}

void PulseChannel::setPeriodHighAndControl(const uint8 value)
{
	m_periodHighAndControl = value;
	if(constexpr uint8 triggerBit{0x80}; m_periodHighAndControl & triggerBit && m_dac) trigger();
}

void PulseChannel::trigger()
{
	m_enabled = true;
	if(m_disableTimer == 0) m_disableTimer = maxDisableTimerDuration - (m_timerAndDuty & timer);
	setPushTimer();
	
	m_volume = (m_volumeAndEnvelope & volumeBits) >> 4;
	constexpr uint8 envelopeTargetBits{0x3};
	m_envelopeTarget = m_volumeAndEnvelope & m_envelopeTarget;
	m_envelopeCounter = 0;
	m_envelopeDir = static_cast<bool>(m_volumeAndEnvelope & envelopeDir);
}

void Channel1::trigger()
{
	PulseChannel::trigger();
	
}

void Channel1::sweepIteration()
{
	
}

std::optional<uint16> Channel1::calculatePeriod()
{
	return std::nullopt;	
}

uint16 PulseChannel::getPeriod() const
{
	return m_periodLow | ((m_periodHighAndControl & periodHigh) << 8);
}

void PulseChannel::setPushTimer()
{
	m_pushTimer = ((maxPeriod + 1) - getPeriod());
}