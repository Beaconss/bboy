#include "core/apu/pulse_channels.h"

channels::PulseChannelBase::PulseChannelBase()
    : m_timerAndDuty{}
	, m_volumeAndEnvelope{}
	, m_periodLow{0xFF}
	, m_periodHighAndControl{}
	, m_enabled{}
	, m_sample{}
	, m_disableTimer{1}
	, m_pushTimer{((maxPeriod + 1) - getPeriod())}
	, m_dutyStep{}
	, m_volume{}
	, m_envelopeTarget{}
	, m_envelopeCounter{}
	, m_envelopeDir{}
	, m_envelopeEnabled{true}
{
	trigger();
}

channels::SweepPulseChannel::SweepPulseChannel()
    : m_sweep{}
    , m_shadowPeriod{}
    , m_sweepTarget{}
    , m_sweepCounter{}
{
    setTimerAndDuty(0xBF);
    setVolumeAndEnvelope(0xF3);
    setPeriodHighAndControl(0xBF);
    m_shadowPeriod = getPeriod();
    m_volume = 0xF;
    m_envelopeTarget = 0x3;
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
	m_sample = dutyPatterns[m_timerAndDuty & (dutyPattern >> 6)][m_dutyStep] * m_volume;
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
	if(m_envelopeTarget == 0) return;
	if(++m_envelopeCounter < m_envelopeTarget) return;
	m_envelopeCounter = 0;

	if(!m_envelopeEnabled) return;

	uint8 newVolume{m_volume};
	if(envelopeDirBit) ++newVolume;
	else --newVolume;
	
	constexpr uint8 maxVolume{0xF};
	if(newVolume <= maxVolume) m_volume = newVolume;
	else m_envelopeEnabled = false;
}

bool channels::PulseChannelBase::isEnabled() const
{
    return m_enabled;
}

uint8 channels::PulseChannelBase::getSample() const
{
    if(m_volumeAndEnvelope & dacBits) return m_enabled ? m_sample : 0;
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
    return m_periodHighAndControl | triggerBit | 0b11'1000 | periodHighBits;
}

void channels::PulseChannelBase::setTimerAndDuty(const uint8 value)
{
	m_timerAndDuty = value;
	setDisableTimer();
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
	if(m_periodHighAndControl & triggerBit) trigger();
}

void channels::PulseChannelBase::trigger()
{
	m_enabled = m_volumeAndEnvelope & dacBits;
	if(m_disableTimer == 0) setDisableTimer(); 
	setPushTimer();
	
	m_volume = (m_volumeAndEnvelope & volumeBits) >> 4;
	constexpr uint8 envelopeTargetBits{0x3};
	m_envelopeTarget = m_volumeAndEnvelope & m_envelopeTarget;
	m_envelopeCounter = 0;
	m_envelopeDir = static_cast<bool>(m_volumeAndEnvelope & envelopeDirBit);
	m_envelopeEnabled = true;
}

uint16 channels::PulseChannelBase::getPeriod() const
{
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

void channels::PulseChannelBase::setDisableTimer()
{
	m_disableTimer = maxDisableTimerDuration - (m_timerAndDuty & timerBits);
}


void channels::SweepPulseChannel::clearRegisters()
{
    channels::PulseChannelBase::clearRegisters();
    m_sweep = 0;
}

void channels::SweepPulseChannel::sweepCycle()
{
	if(m_sweepTarget == 0 || !(m_sweep & sweepTargetBits)) return;
	if(++m_sweepCounter < m_sweepTarget) return;
	
	m_sweepCounter = 0;
	m_sweepTarget = (m_sweep & sweepTargetBits) >> 4;

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
	m_sweepCounter = 0;
	m_sweepTarget = (m_sweep & sweepTargetBits) >> 4;
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