#pragma once
#include <SDL3/SDL_audio.h>
#include <type_alias.h>
#include <array>
#include <algorithm>
#include <vector>

class APU
{
public:
	APU();

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
	void cycle();
	uint8 read(const Index index, const uint8 waveRamIndex = 0) const;
	void write(const Index index, const uint8 value, const uint8 waveRamIndex = 0);

private:
	struct Channel1
	{
		uint8 sweep{};
		uint8 timerAndDuty{};
		uint8 volumeAndEnvelope{};
		uint8 periodLow{};
		uint8 periodHighAndControl{};
	};

	struct Channel2
	{
		uint8 timerAndDuty{};
		uint8 volumeAndEnvelope{};
		uint8 periodLow{};
		uint8 periodHighAndControl{};
	};

	struct Channel3
	{
		uint8 dacEnable{};
		uint8 timer{};
		uint8 volume{};
		uint8 periodLow{};
		uint8 periodHighAndControl{};
	};

	struct Channel4
	{
		uint8 timer;
		uint8 volumeAndEnvelope;
		uint8 frequencyAndRandomness;
		uint8 control;
	};

	SDL_AudioStream* m_audioStream;

	Channel1 m_squareSweepChannel;
	Channel2 m_squareChannel;
	Channel3 m_waveChannel;
	Channel4 m_noiseChannel;

	uint8 m_audioVolume;
	uint8 m_audioPanning;
	uint8 m_audioControl;
	std::array<uint8, 16> m_waveRam;
};

