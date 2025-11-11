#include "core/apu/wave_channel.h"
#include "wave_channel.h"
#include <iostream>

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
    , m_disableTimer{}
{
    setDacEnable(0x7F);
    setTimer(0xFF);
    setPeriodHighAndControl(0xBF);
    std::fill(m_waveRam.begin(), m_waveRam.end(), 0xFF);
}

void channels::WaveChannel::clearRegisters()
{
    setDacEnable(0);
    setOutLevel(0);
    setPeriodLow(0);
    setPeriodHighAndControl(0);
    m_waveIndex = 0;
}

bool channels::WaveChannel::isEnabled()
{
    return m_enabled;
}

uint8 channels::WaveChannel::getSample()
{
    uint8 volume = (m_outLevel & volumeBits) >> 5; 
    if(m_dacEnable & dacBit) return (m_enabled && (volume > 0)) ? (m_sample >> (volume - 1)) : 0;
    else return 7;
}

void channels::WaveChannel::pushCycle()
{
    if(--m_pushTimer > 0) return;

    setPushTimer();
    if(m_waveIndex & 1) m_sample = m_waveRam[(m_waveIndex - 1) / 2] & 0xF;
    else m_sample = m_waveRam[m_waveIndex / 2] >> 4;

    ++m_waveIndex;
    constexpr int maxIndex{31};
    m_waveIndex %= (maxIndex + 1);
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
    return m_waveRam[index];
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
    //std::cout << "wave ram written to, index: " << (int)index << "\nvalue: " << (int)value << "\n\n";
    m_waveRam[index] = value;
}

void channels::WaveChannel::trigger()
{
    m_enabled = m_dacEnable & dacBit;
    if(m_disableTimer == 0) m_disableTimer = maxDisableTimerDuration;
    setPushTimer();
    m_waveIndex = 0;
}

void channels::WaveChannel::setPushTimer()
{
    constexpr uint8 periodHighBits{0b111};
    m_pushTimer = ((maxPeriod + 1) - (m_periodLow | ((m_periodHighAndControl & periodHighBits) << 8)))/*this should be multiplied by 2 but it sounds better like this*/;  
}