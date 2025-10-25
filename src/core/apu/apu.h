#pragma once
#include <type_alias.h>
#include <core/apu/audio_thread.h>

#include <SDL3/SDL_audio.h>

#include <array>
#include <queue>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cmath>
#include <atomic>

class APU
{
public:
	APU();
	~APU();

	enum Index
	{
		ch1Sw,
		ch1TimDuty,
		ch1VolEnv,
		ch1PeLow,
		ch1PeHighCtrl,
		ch2TimDuty,
		ch2VolEnv,
		ch2PeLow,
		ch2PeHighCtrl,
		ch3DacEn,
		ch3Tim,
		ch3Vol,
		ch3PeLow,
		ch3PeHighCtrl,
		ch4Tim,
		ch4VolEnv,
		ch4FreRand,
		ch4Ctrl,
		audioVolume,
		audioPanning,
		audioCtrl,
		waveRam,
	};

	void reset();
	void setupFrame(float frameTime);
	void doCycles(const int cycleCount);	
	void unlockThread();
	uint8 read(const Index index, const uint8 waveRamIndex = 0);
	void write(const Index index, const uint8 value, const uint16 currentCycle, const uint8 waveRamIndex = 0);

private:
	friend class AudioThread;

	struct Timestamp
	{
		Index registerToSet{};
		uint16 cycle{};
		uint8 value{};
		uint8 waveRamIndex{};
	};

	struct PulseChannel
	{
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
		static constexpr uint8 timer{0b0011'1111};

		uint8 timerAndDuty{0x3F};
		uint8 volumeAndEnvelope{0};
		uint8 periodLow{0xFF};
		uint8 periodHighAndControl{0xBF};

		bool enabled{};
		uint8 sample{};
		int disableTimer{64};
		int pushTimer{4 * (2048 - 0x7FF)};
		uint8 dutyStep{};

		void pushCycle();
		void disableTimerCycle();
		void trigger();
		void setPushTimer();
		void setTimerAndDuty(const uint8 value);
		void setPeriodHighAndControl(const uint8 value);
	};

	struct Channel1 : PulseChannel
	{
		uint8 sweep{0x80};
	};
	
	struct Channel2 : PulseChannel
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
	static constexpr int maxDisableTimerDuration{64};

	struct Channel4
	{
		uint8 timer{0xFF};
		uint8 volumeAndEnvelope{0};
		uint8 frequencyAndRandomness{0};
		uint8 control{0xBF};
	};

	void mCycle(const int cycle);
	void loadTimestamp(const Timestamp& timestamp);
	void finishFrame();
	void putAudio();

	static constexpr int mCyclesPerFrame{17556};
	static constexpr int frequency{44100};
	static constexpr uint8 audioEnable{0x80};
	
	AudioThread m_audioThread;
	SDL_AudioStream* m_audioStream;
	std::vector<float> m_samplesBuffer; 
	std::queue<Timestamp> m_timestamps;
	std::queue<Timestamp> m_thisFrameTimestamps;

	uint16 m_frameSequencerCounter;
	uint16 m_remainingCycles;
	uint16 m_thisFrameSamples;
	uint16 m_nearestNeighbourCounter;
	uint32 m_nearestNeighbourTarget;

	Channel1 m_channel1;
	Channel2 m_channel2;
	Channel3 m_channel3;
	Channel4 m_channel4;

	uint8 m_audioVolume;
	uint8 m_audioPanning;
	uint8 m_audioControl;
	std::array<uint8, 16> m_waveRam;
};