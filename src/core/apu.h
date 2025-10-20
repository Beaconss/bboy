#pragma once
#include <SDL3/SDL_audio.h>
#include <type_alias.h>
#include <array>
#include <algorithm>
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>

class APU
{
public:
	APU();

	struct PromiseCycle
	{

	};

	struct Awaitable
	{

	};

	enum Index
	{
		CH1_SW,
		CH1_TIM_DUTY,
		CH1_VOL_ENV,
		CH1_PE_LOW,
		CH1_PE_HI_CTRL,
		CH2_TIM_DUTY,
		CH2_VOL_ENV,
		CH2_PE_LOW,
		CH2_PE_HI_CTRL,
		CH3_DAC_EN,
		CH3_TIM,
		CH3_VOL,
		CH3_PE_LOW,
		CH3_PE_HI_CTRL,
		CH4_TIM,
		CH4_VOL_ENV,
		CH4_FRE_RAND,
		CH4_CTRL,
		AU_VOL,
		AU_PAN,
		AU_CTRL,
		WAVE_RAM,
	};

	void reset();
	void doCycles(const int cycleCount);
	void doRemainingCycles();
	void pushAudio();
	uint8 read(const Index index, const uint8 waveRamIndex = 0) const;
	void write(const Index index, const uint8 value, const uint8 waveRamIndex = 0);

private:
	struct PulseChannelBase
	{
		static constexpr int MAX_DUTY_STEP{7};
		static constexpr int DUTY_PATTERNS_STEPS{8};
		static constexpr int DUTY_PATTERNS_COUNT{4};
		static constexpr std::array<std::array<uint8, DUTY_PATTERNS_STEPS>, DUTY_PATTERNS_COUNT> DUTY_PATTERNS
		{{
			{0,0,0,0,0,0,0,1},
			{1,0,0,0,0,0,0,1},
			{1,0,0,0,0,1,1,1},
			{0,1,1,1,1,1,1,0},
		}};

		uint8 timerAndDuty{0x3F};
		uint8 volumeAndEnvelope{0};
		uint8 periodLow{0xFF};
		uint8 periodHighAndControl{0xBF};

		bool enabled{};
		uint8 sample{};
		int disableTimer{64};
		int pushTimer{4 * (2048 - 0x7FF)};
		uint8 dutyStep{};

		bool pushCycle();
		void disableTimerCycle();
		void trigger();
		void setPushTimer();
	};

	struct Channel1 : PulseChannelBase
	{
		uint8 sweep{0x80};
	};

	struct Channel2 : PulseChannelBase
	{
	};

	struct Channel3
	{
		uint8 dacEnable{0x7F};
		uint8 timer{0xFF};
		uint8 volume{0x9F};
		uint8 periodLow{0xFF};
		uint8 periodHighAndControl{0xBF};
	};
	static constexpr int MAX_DISABLE_TIMER_DURATION{64};

	struct Channel4
	{
		uint8 timer{0xFF};
		uint8 volumeAndEnvelope{0};
		uint8 frequencyAndRandomness{0};
		uint8 control{0xBF};
	};
	
	void mCycle();

	static constexpr int CYCLES_PER_FRAME{17556};
	static constexpr int ONE_FRAME_SAMPLES{44100 / 60};
	static constexpr uint8 AUDIO_ENABLE{0x80};
	static constexpr uint8 TRIGGER{0x80};
	
	SDL_AudioStream* m_audioStream;
	std::vector<float> m_samplesBuffer; 

	int m_frameSequencerCounter;
	int m_remainingCycles;

	Channel1 m_channel1;
	Channel2 m_channel2;
	Channel3 m_channel3;
	Channel4 m_channel4;

	uint8 m_audioVolume;
	uint8 m_audioPanning;
	uint8 m_audioControl;
	std::array<uint8, 16> m_waveRam;
};

