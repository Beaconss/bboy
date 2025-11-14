#pragma once
#include "type_alias.h"
#include "core/apu/audio_thread.h"
#include "core/apu/pulse_channels.h"
#include "core/apu/wave_channel.h"
#include "core/apu/noise_channel.h"
#include <array>
#include <queue>
#include <vector>

class MMU;
struct SDL_AudioStream;
class APU
{
public:
	APU(MMU& bus);
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
	uint8 read(const Index index, const uint8 waveRamIndex = 0);
	void write(const Index index, const uint8 value, const uint8 waveRamIndex = 0);

private:
	friend class AudioThread;

	void clearRegisters();
	bool clearedWhenOff(Index reg) const;
	void mCycle();
	void catchUp();
	void finishFrame();
	void pushAudio();

	static constexpr int mCyclesPerFrame{17556};
	static constexpr int frequency{44100};
	static constexpr uint8 audioEnable{0x80};
	
	MMU& m_bus;
	AudioThread m_audioThread;
	SDL_AudioStream* m_audioStream;
	std::vector<float> m_samplesBuffer;
	std::vector<float> m_outSamples;
	uint16 m_frameSequencerCounter;
	uint8 m_frameSequencerStep;
	uint16 m_nextCycleToExecute;

	channels::SweepPulseChannel m_channel1;
	channels::PulseChannel m_channel2;
	channels::WaveChannel m_channel3;
	channels::NoiseChannel m_channel4;

	uint8 m_audioVolume;
	uint8 m_audioPanning;
	uint8 m_audioControl;
};