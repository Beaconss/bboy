#pragma once
#include "type_alias.h"

namespace channels
{
class Envelope
{
public:
    Envelope();

    void cycle();

    uint8 getVolumeAndEnvelope() const;
    void setVolumeAndEnvelope(const uint8 value);

    uint8 getVolume() const;
    bool dac() const;
    void trigger();
private:
    static constexpr uint8 targetBits{0x7};

    uint8 m_volumeAndEnvelope;

    uint8 m_volume;
    uint8 m_target;
    uint8 m_timer;
    bool m_dir;
};
}