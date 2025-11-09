#include "core/apu/wave_channel.h"
#include "wave_channel.h"

channels::WaveChannel::WaveChannel()
    : m_dacEnable{}
    , m_timer{}
    , m_outLevel{0x9F}
    , m_periodLow{0xFF}
    , m_periodHighAndControl{}
    , m_waveRam{}
    , m_enabled{}
    , m_pushTimer{(maxPeriod + 1) - maxPeriod}
    , m_sample{}
    , m_waveIndex{1}
    , m_period{}
    , m_disableTimer{}
    , m_volume{(m_outLevel & volumeBits) >> 5}
{
    setDacEnable(0x7F);
    setTimer(0xFF);
    setPeriodHighAndControl(0xBF);
}

void channels::WaveChannel::clearRegisters()
{
    m_dacEnable = 0;
    m_outLevel = 0;
    m_periodLow = 0;
    m_periodHighAndControl = 0;
}

bool channels::WaveChannel::isEnabled()
{
    return m_enabled;
}

uint8 channels::WaveChannel::getSample()
{
    if(m_dacEnable & dacBit) return m_enabled ? m_sample : 0;
    else return 7;
}

void channels::WaveChannel::pushCycle()
{

}

void channels::WaveChannel::disableTimerCycle()
{
    if(constexpr uint8 disableTimerBit{0b0100'0000}; !(m_periodHighAndControl & disableTimerBit) || m_disableTimer == 0) return; 
	
	if(--m_disableTimer == 0) m_enabled = false;
}

uint8 channels::WaveChannel::getDacEnable() const
{
    return m_dacEnable | 0x7F;
}

uint8 channels::WaveChannel::getOutLevel() const
{
    return m_outLevel | 0b1001'1111;
}

uint8 channels::WaveChannel::getPeriodHighAndControl() const
{
    return m_periodHighAndControl | 0b1011'1111;
}

uint8 channels::WaveChannel::getWaveRam(const uint8 index) const
{
    //todo: return also if this index is being used this cycle
    if(m_enabled) return 0xFF;
    else return m_waveRam[index];
}

void channels::WaveChannel::setDacEnable(const uint8 value)
{
    m_dacEnable = value;
    if(constexpr uint8 dac{0x80}; !(m_dacEnable & dac)) m_enabled = false;
}

void channels::WaveChannel::setTimer(const uint8 value)
{
    m_timer = value;
    m_disableTimer = maxDisableTimerDuration - m_timer;
}

void channels::WaveChannel::setOutLevel(const uint8 value)
{
    m_outLevel = value;
}

void channels::WaveChannel::setPeriodLow(const uint8 value)
{
    m_periodLow = value;
}

void channels::WaveChannel::setPeriodHighAndControl(const uint8 value)
{
    m_periodHighAndControl = value;
    if(constexpr uint8 triggerBit{0x80}; m_periodHighAndControl & triggerBit) trigger();
}

void channels::WaveChannel::setWaveRam(const uint8 value, uint8 index)
{
    //todo: write also if this index is being used this cycle
    if(m_enabled) return;
    m_waveRam[index] = value;
}

void channels::WaveChannel::trigger()
{
    m_enabled = m_dacEnable & dacBit;
    if(m_disableTimer == 0) m_disableTimer = maxDisableTimerDuration - m_timer;
    constexpr uint8 periodHigh{0x7};    
    m_period = m_periodLow | ((m_periodHighAndControl & periodHigh) << 8);
    m_volume = (m_outLevel & volumeBits) >> 5;
    m_waveIndex = 0;
}
