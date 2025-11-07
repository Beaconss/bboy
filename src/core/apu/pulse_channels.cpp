#include "core/apu/pulse_channels.h"

channels::PulseChannel::PulseChannel()
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

channels::Channel1::Channel1()
    : m_sweep{}
    , m_shadowPeriod{}
    , m_sweepTarget{}
    , m_sweepCounter{}
{
    setTimerAndDuty(0xBF);
    setVolumeAndEnvelope(0xF3);
    setPeriodHighAndControl(0xBF);
    m_shadowPeriod = getPeriod();
    m_dac = true;
    m_volume = 0xF;
    m_envelopeTarget = 0x3;
}

channels::Channel2::Channel2()
{
    setTimerAndDuty(0x3F);
    setVolumeAndEnvelope(0);
    setPeriodHighAndControl(0xBF);
}

void channels::PulseChannel::clearRegisters()
{
    m_volumeAndEnvelope = 0;
	m_periodLow = 0;
	m_periodHighAndControl = 0;
	m_dutyStep = 0;
}

void channels::Channel1::clearRegisters()
{
    channels::PulseChannel::clearRegisters();
    m_sweep = 0;
}

void channels::PulseChannel::pushCycle()
{
	if(--m_pushTimer > 0) return;

	setPushTimer();
	constexpr uint8 dutyPattern{0b1100'0000};
	m_sample = dutyPatterns[m_timerAndDuty & (dutyPattern >> 6)][m_dutyStep] * m_volume;
	++m_dutyStep;
	m_dutyStep &= maxDutyStep;
}

void channels::PulseChannel::disableTimerCycle()
{
	if(!(m_periodHighAndControl & disableTimer) || m_disableTimer == 0) return; 
	
	if(--m_disableTimer == 0) m_enabled = false;
}

void channels::PulseChannel::envelopeCycle()
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

void channels::Channel1::sweepCycle()
{
	if(m_sweepTarget == 0 || !(m_sweep & sweepTargetBits)) return;
	if(++m_sweepCounter < m_sweepTarget) return;
	
	m_sweepCounter = 0;
	m_sweepTarget = (m_sweep & sweepTargetBits) >> 4;

	uint16 newPeriod{};	
	if(m_sweep & subtraction)
	{
		newPeriod = m_shadowPeriod - (m_shadowPeriod >> (m_sweep & sweepStep));
		if(newPeriod > maxPeriod) newPeriod = 0; //underflow check
	}
	else newPeriod = m_shadowPeriod + (m_shadowPeriod >> (m_sweep & sweepStep));

	if(newPeriod <= maxPeriod) 
	{
		if(!(m_sweep & sweepStep)) return;
		m_shadowPeriod = newPeriod;
		setPeriod(newPeriod);
	}
	else m_enabled = false;
}

bool channels::PulseChannel::isEnabled() const
{
    return m_enabled;
}

uint8 channels::PulseChannel::getSample() const
{
    if(m_dac) return m_enabled ? m_sample : 0;
    else return 7;
}

uint8 channels::Channel1::getSweep() const
{
    return m_sweep | 0x80;
}

uint8 channels::PulseChannel::getTimerAndDuty() const
{
    return m_timerAndDuty | timer;
}

uint8 channels::PulseChannel::getVolumeAndEnvelope() const
{
    return m_volumeAndEnvelope;
}

uint8 channels::PulseChannel::getPeriodHighAndControl() const
{
    return m_periodHighAndControl | triggerBit | 0b11'1000 | periodHigh;
}

void channels::Channel1::setSweep(uint8 value)
{
	m_sweep = value;
}

void channels::PulseChannel::setTimerAndDuty(const uint8 value)
{
	m_timerAndDuty = value;
	setDisableTimer();
}

void channels::PulseChannel::setVolumeAndEnvelope(const uint8 value)
{
	m_volumeAndEnvelope = value;
	m_dac = m_volumeAndEnvelope & (volumeBits | envelopeDir);
	if(!m_dac) m_enabled = false;
}

void channels::PulseChannel::setPeriodLow(const uint8 value)
{
    m_periodLow = value;
}

void channels::PulseChannel::setPeriodHighAndControl(const uint8 value)
{
	m_periodHighAndControl = value;
	if(m_periodHighAndControl & triggerBit) trigger();
}

void channels::PulseChannel::trigger()
{
	m_enabled = m_dac;
	if(m_disableTimer == 0) setDisableTimer(); 
	setPushTimer();
	
	m_volume = (m_volumeAndEnvelope & volumeBits) >> 4;
	constexpr uint8 envelopeTargetBits{0x3};
	m_envelopeTarget = m_volumeAndEnvelope & m_envelopeTarget;
	m_envelopeCounter = 0;
	m_envelopeDir = static_cast<bool>(m_volumeAndEnvelope & envelopeDir);
}

void channels::Channel1::trigger()
{
	channels::PulseChannel::trigger();
	m_shadowPeriod = getPeriod();
	m_sweepCounter = 0;
	m_sweepTarget = (m_sweep & sweepTargetBits) >> 4;
	if(!(m_sweep & sweepStep)) return;

	uint16 newPeriod{};	
	if(m_sweep & subtraction)
	{
		newPeriod = m_shadowPeriod - (m_shadowPeriod >> (m_sweep & sweepStep));
		if(newPeriod > maxPeriod) newPeriod = 0; //underflow check
	}
	else newPeriod = m_shadowPeriod + (m_shadowPeriod >> (m_sweep & sweepStep));

	if(newPeriod <= maxPeriod) 
	{
		if(!(m_sweep & sweepStep)) return;
		m_shadowPeriod = newPeriod;
		setPeriod(newPeriod);
	}
	else m_enabled = false;
}

uint16 channels::PulseChannel::getPeriod() const
{
	return m_periodLow | ((m_periodHighAndControl & periodHigh) << 8);
}

void channels::PulseChannel::setPeriod(const uint16 value)
{
	m_periodLow = value & 0xFF;
	m_periodHighAndControl = ((m_periodHighAndControl & triggerBit) & disableTimer) | ((value >> 8) & 0b111);
}

void channels::PulseChannel::setPushTimer()
{
	m_pushTimer = ((maxPeriod + 1) - getPeriod());
}

void channels::PulseChannel::setDisableTimer()
{
	m_disableTimer = maxDisableTimerDuration - (m_timerAndDuty & timer);
}