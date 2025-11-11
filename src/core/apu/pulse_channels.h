#pragma once
#include "type_alias.h"
#include <array>
#include <optional>

namespace channels
{
class PulseChannelBase
{
public:
	virtual void clearRegisters();
	void pushCycle();
	void disableTimerCycle();
	void envelopeCycle();

	bool isEnabled() const;
	uint8 getSample() const;
	
	uint8 getTimerAndDuty() const;
	uint8 getVolumeAndEnvelope() const;
	uint8 getPeriodHighAndControl() const;

	void setTimerAndDuty(const uint8 value);
	void setVolumeAndEnvelope(const uint8 value);
	void setPeriodLow(const uint8 value);
	void setPeriodHighAndControl(const uint8 value);
protected:
	PulseChannelBase();
	
	virtual void trigger();
	uint16 getPeriod() const;
	void setPeriod(const uint16 value);
	
	static constexpr uint16 maxPeriod{0x7FF};
	
	bool m_enabled;
	uint8 m_volume;
	uint8 m_envelopeTimer;
private:
	void setPushTimer();

	static constexpr int maxDisableTimerDuration{64};
	static constexpr int maxDutyStep{7};
	static constexpr int dutyPatternsStep{8};
	static constexpr int dutyPatternsCount{4};
	static constexpr std::array<std::array<uint8, dutyPatternsStep>, dutyPatternsCount> dutyPatterns
	{{
		{0,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,1},
		{1,0,0,0,0,1,1,1},
		{0,1,1,1,1,1,1,0},
	}};
	static constexpr uint8 disableTimerBit{0b0100'0000};
	static constexpr uint8 timerBits{0b0011'1111};
	static constexpr uint8 volumeBits{0xF0};
	static constexpr uint8 envelopeTargetBits{0x3};
	static constexpr uint8 envelopeDirBit{0x8};
	static constexpr uint8 dacBits{volumeBits | envelopeDirBit};
	static constexpr uint8 triggerBit{0x80}; 

	uint8 m_timerAndDuty;
	uint8 m_volumeAndEnvelope;
	uint8 m_periodLow;
	uint8 m_periodHighAndControl;

	uint8 m_sample;
	uint8 m_disableTimer;
	uint16 m_pushTimer;
	uint8 m_dutyStep;
	bool m_envelopeDir;
};

class SweepPulseChannel : public PulseChannelBase
{
public:
	SweepPulseChannel();

	void clearRegisters() override;
	void sweepCycle();

	uint8 getSweep() const;
	void setSweep(uint8 value);
private:
	void trigger() override;
	void sweepIteration();

	static constexpr uint8 sweepTargetBits{0b0111'0000};
	static constexpr uint8 subtractionBit{0x8};
	static constexpr uint8 sweepStepBits{0b111};

	uint8 m_sweep;
		
	uint16 m_shadowPeriod;
	uint8 m_sweepTimer;
};
	
class PulseChannel : public PulseChannelBase
{
public:
	PulseChannel();
};
}