#pragma once
#include "type_alias.h"
#include "core/apu/audio_thread.h"
#include "core/apu/pulse_channels.h"
#include "core/apu/wave_channel.h"
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
	uint16 m_nearestNeighbourCounter;
	uint32 m_nearestNeighbourTarget;

	channels::SweepPulseChannel m_channel1;
	channels::PulseChannel m_channel2;
	channels::WaveChannel m_channel3;
	Channel4 m_channel4;

	uint8 m_audioVolume;
	uint8 m_audioPanning;
	uint8 m_audioControl;
};