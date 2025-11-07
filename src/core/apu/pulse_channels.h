#pragma once
#include "type_alias.h"
#include <array>
#include <optional>

namespace channels
{
class PulseChannel
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
	PulseChannel();
	
	virtual void trigger();
	uint16 getPeriod() const;
	void setPeriod(const uint16 value);
	
	static constexpr uint16 maxPeriod{0x7FF};
	
	bool m_dac;
	bool m_enabled;
	uint8 m_volume;
	uint8 m_envelopeTarget;
private:
	void setPushTimer();
	void setDisableTimer();

	static constexpr int maxDisableTimerDuration{63}; //this could be 64
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
	static constexpr uint8 disableTimer{0b0100'0000};
	static constexpr uint8 timer{0b0011'1111};
	static constexpr uint8 volumeBits{0xF0};
	static constexpr uint8 envelopeDir{0x8};
	static constexpr uint8 triggerBit{0x80}; 
	static constexpr uint8 periodHigh{0b111};

	uint8 m_timerAndDuty;
	uint8 m_volumeAndEnvelope;
	uint8 m_periodLow;
	uint8 m_periodHighAndControl;

	uint8 m_sample;
	int m_disableTimer;
	int m_pushTimer;
	uint8 m_dutyStep;
	uint8 m_envelopeCounter;
	bool m_envelopeDir;
};

class Channel1 : public PulseChannel
{
public:
	Channel1();

	void clearRegisters() override;
	void sweepCycle();

	uint8 getSweep() const;
	void setSweep(uint8 value);
private:
	void trigger() override;

	static constexpr uint8 sweepTargetBits{0b0111'0000};
	static constexpr uint8 subtraction{0x8};
	static constexpr uint8 sweepStep{0b111};

	uint8 m_sweep;
		
	uint16 m_shadowPeriod;
	uint8 m_sweepTarget;
	uint8 m_sweepCounter;
};
	
class Channel2 : public PulseChannel
{
public:
	Channel2();
};
}