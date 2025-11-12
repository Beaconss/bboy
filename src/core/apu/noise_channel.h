#pragma once
#include "type_alias.h"

namespace channels
{
class NoiseChannel
{
public:
    NoiseChannel();

    void clearRegisters();
    void pushCycle();
    void disableTimerCycle();
    void envelopeCycle();

    bool isEnabled() const;
    uint8 getSample() const;

    uint8 getVolumeAndEnvelope() const;
    uint8 getFrequencyAndRandomness() const;
    uint8 getControl() const;

    void setTimer(const uint8 value);
    void setVolumeAndEnvelope(const uint8 value);
    void setFrequencyAndRandomness(const uint8 value);
    void setControl(const uint8 value);
private:
    void setPushTimer();
    void trigger();

    static constexpr int maxDisableTimerDuration{64};
	static constexpr uint8 envelopeTargetBits{0x7};
	static constexpr uint8 envelopeDirBit{0x8};
	static constexpr uint8 dacBits{0xF0 | envelopeDirBit};

    uint8 m_timer;
	uint8 m_volumeAndEnvelope;
	uint8 m_frequencyAndRandomness;
	uint8 m_control;

    bool m_enabled;
    uint16 m_lfsr;
    uint8 m_sample;
    uint32 m_pushTimer;
    uint8 m_disableTimer;
    uint8 m_volume;
    uint8 m_envelopeTimer;
    bool m_envelopeDir;
};
}