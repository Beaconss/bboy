#include "core/apu/envelope_component.h"

channels::EnvelopeComponent::EnvelopeComponent()
    : m_volumeAndEnvelope{}
    , m_volume{}
    , m_target{}
    , m_timer{}
    , m_dir{}
{}

void channels::EnvelopeComponent::cycle()
{
    if(m_timer == 0  || !(m_volumeAndEnvelope & targetBits)) return;
	else if(--m_timer > 0) return;

    m_timer = m_target;
	uint8 newVolume{m_volume};
	if(m_dir) ++newVolume;
	else --newVolume;
	
	constexpr uint8 maxVolume{0xF};
	if(newVolume <= maxVolume) m_volume = newVolume;
}

uint8 channels::EnvelopeComponent::getVolumeAndEnvelope() const
{
    return m_volumeAndEnvelope;
}

void channels::EnvelopeComponent::setVolumeAndEnvelope(const uint8 value)
{
    m_volumeAndEnvelope = value;
}

uint8 channels::EnvelopeComponent::getVolume() const
{
    return m_volume;
}

bool channels::EnvelopeComponent::dac() const
{
    constexpr uint8 dacBits{0xF8};
    return m_volumeAndEnvelope & dacBits;
}

void channels::EnvelopeComponent::trigger()
{
    constexpr uint8 volumeBits{0xF0};
    m_volume = (m_volumeAndEnvelope & volumeBits) >> 4;
    m_target = m_volumeAndEnvelope & targetBits;
	m_timer = m_target;
	constexpr uint8 dirBit{0x8};
	m_dir = static_cast<bool>(m_volumeAndEnvelope & dirBit);
}
