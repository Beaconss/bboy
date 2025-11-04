#pragma once
#include "type_alias.h"
#include "core/apu/audio_thread.h"
#include <array>
#include <queue>
#include <vector>

class Bus;
struct SDL_AudioStream;
class APU
{
public:
	APU(Bus& bus);
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
	void unlockThread();
	void setFrametime(float frametime);
	uint8 read(const Index index, const uint8 waveRamIndex = 0);
	void write(const Index index, const uint8 value, const uint8 waveRamIndex = 0);

private:
	friend class AudioThread;

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

		uint8 timerAndDuty{};
		uint8 volumeAndEnvelope{};
		uint8 periodLow{};
		uint8 periodHighAndControl{};

		bool enabled{};
		bool dac{};
		uint8 sample{};
		int disableTimer{1};
		int pushTimer{(0x800 - 0x7FF)};
		uint8 dutyStep{};
		uint8 volume{};
		uint8 envelopeTarget{};
		uint8 envelopeCounter{};
		bool envelopeDir{};

		void pushCycle();
		void disableTimerCycle();
		void envelopeCycle();
		void setTimerAndDuty(const uint8 value);
		void setVolumeAndEnvelope(const uint8 value);
		void setPeriodHighAndControl(const uint8 value);
		void setPushTimer();
		void trigger();
	};

	struct Channel1 : PulseChannel
	{
		uint8 sweep{0x80};
		uint8 sweepOutput{};
		uint8 sweepTarget{};
		uint8 sweepCounter{};
		bool sweepEnabled{};
		Channel1()
		{
			timerAndDuty = 0xBF;
			volumeAndEnvelope = 0xF3;
			periodLow = 0xFF;
			periodHighAndControl = 0xBF;
			dac = true;
			volume = 0xF;
			envelopeTarget = 0x3;
		}
	};
	
	struct Channel2 : PulseChannel
	{
		Channel2()
		{
			timerAndDuty = 0x3F;
			volumeAndEnvelope = 0;
			periodLow = 0xFF;
			periodHighAndControl = 0xBF;
		}
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

	void clearRegisters();
	bool clearedWhenOff(Index reg) const;
	void mCycle();
	void catchUp();
	void finishFrame();
	void setNearestNeighbour();
	void putAudio();

	static constexpr auto digitalToAnalog{[]
										{
											constexpr size_t size{16};
											std::array<float, size> out{};
											constexpr float conversionStep{2.f / (size - 1)};
											for(size_t i{}; i < size; ++i)
											{
												out[i] = 1.f - (conversionStep * i);
											}
											return out;
										}()};

	static constexpr int mCyclesPerFrame{17556};
	static constexpr int frequency{44100};
	static constexpr uint8 audioEnable{0x80};
	
	Bus& m_bus;
	AudioThread m_audioThread;
	SDL_AudioStream* m_audioStream;
	std::vector<float> m_samplesBuffer; 
	uint16 m_frameSequencerCounter;
	uint16 m_nextCycleToExecute;

	std::atomic<float> m_lastFrametime;
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