#include "apu.h"

APU::APU()
	: m_audioStream{}
	, m_squareSweepChannel{}
	, m_squareChannel{}
	, m_waveChannel{}
	, m_noiseChannel{}
	, m_audioVolume{}
	, m_audioPanning{}
	, m_audioControl{}
	, m_waveRam{}
{
	static SDL_AudioSpec spec{SDL_AudioFormat::SDL_AUDIO_U8, 1, 44100};
	m_audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	SDL_ResumeAudioStreamDevice(m_audioStream);
}

void APU::reset()
{
	m_squareSweepChannel = Channel1{};
	m_squareChannel = Channel2{};
	m_waveChannel = Channel3{};
	m_noiseChannel = Channel4{};
	m_audioVolume = 0;
	m_audioPanning = 0;
	m_audioControl = 0;
	std::ranges::fill(m_waveRam, 0);
}

void APU::cycle()
{
	return;
}

uint8 APU::read(const Index index, uint8 waveRamIndex) const
{
	switch(index)
	{
	case CH1_SW: return m_squareSweepChannel.sweep;
	case CH1_TIM_DUTY: return m_squareSweepChannel.timerAndDuty;
	case CH1_VOL_ENV: return m_squareSweepChannel.volumeAndEnvelope;
	case CH1_PE_LOW: return m_squareSweepChannel.periodLow;
	case CH1_PE_HI_CTRL: return m_squareSweepChannel.periodHighAndControl;
	case CH2_TIM_DUTY: return m_squareChannel.timerAndDuty;
	case CH2_VOL_ENV: return m_squareChannel.volumeAndEnvelope;
	case CH2_PE_LOW: return m_squareChannel.periodLow;
	case CH2_PE_HI_CTRL: return m_squareChannel.periodHighAndControl;
	case CH3_DAC_EN: return m_waveChannel.dacEnable;
	case CH3_TIM: return m_waveChannel.timer;
	case CH3_VOL: return m_waveChannel.volume;
	case CH3_PE_LOW: return m_waveChannel.periodLow;
	case CH3_PE_HI_CTRL: return m_waveChannel.periodHighAndControl;
	case CH4_TIM: return m_noiseChannel.timer;
	case CH4_VOL_ENV: return m_noiseChannel.volumeAndEnvelope;
	case CH4_FRE_RAND: return m_noiseChannel.frequencyAndRandomness;
	case CH4_CTRL: return m_noiseChannel.control;
	case AU_VOL: return m_audioVolume;
	case AU_PAN: return m_audioPanning;
	case AU_CTRL: return m_audioControl;
	case WAVE_RAM: return m_waveRam[waveRamIndex];
	default: return 0xFF;
	}
}

void APU::write(const Index index, const uint8 value, uint8 waveRamIndex)
{
	switch(index)
	{
	case CH1_SW: m_squareSweepChannel.sweep = value; break;
	case CH1_TIM_DUTY: m_squareSweepChannel.timerAndDuty = value; break;
	case CH1_VOL_ENV: m_squareSweepChannel.volumeAndEnvelope = value; break;
	case CH1_PE_LOW: m_squareSweepChannel.periodLow = value; break;
	case CH1_PE_HI_CTRL: m_squareSweepChannel.periodHighAndControl = value; break;
	case CH2_TIM_DUTY: m_squareChannel.timerAndDuty = value; break;
	case CH2_VOL_ENV: m_squareChannel.volumeAndEnvelope = value; break;
	case CH2_PE_LOW: m_squareChannel.periodLow = value; break;
	case CH2_PE_HI_CTRL: m_squareChannel.periodHighAndControl = value; break;
	case CH3_DAC_EN: m_waveChannel.dacEnable = value; break;
	case CH3_TIM: m_waveChannel.timer = value; break;
	case CH3_VOL: m_waveChannel.volume = value; break;
	case CH3_PE_LOW: m_waveChannel.periodLow = value; break;
	case CH3_PE_HI_CTRL: m_waveChannel.periodHighAndControl = value; break;
	case CH4_TIM: m_noiseChannel.timer = value; break;
	case CH4_VOL_ENV: m_noiseChannel.volumeAndEnvelope = value; break;
	case CH4_FRE_RAND: m_noiseChannel.frequencyAndRandomness = value; break;
	case CH4_CTRL: m_noiseChannel.control = value; break;
	case AU_VOL: m_audioVolume = value; break;
	case AU_PAN: m_audioPanning = value; break;
	case AU_CTRL: m_audioControl = value; break;
	case WAVE_RAM: m_waveRam[waveRamIndex] = value; break;
	}
}