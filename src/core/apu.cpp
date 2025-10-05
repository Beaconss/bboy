#include "apu.h"

void playAudio(SDL_AudioStream* stream)
{
	std::vector<float> buf(44100);
	for(int i{}; i < buf.size(); ++i)
	{
		buf[i] = i % 200 < 100 ? 1.0f : 0.0f;
	}
	SDL_PutAudioStreamData(stream, buf.data(), int(buf.size() * sizeof(float)));
	buf.clear();
}

APU::APU()
	: m_audioStream{}
	, m_samplesBuffer{}
	, m_cycleCounter{}
	, m_channel1{}
	, m_channel2{}
	, m_channel3{}
	, m_channel4{}
	, m_audioVolume{}
	, m_audioPanning{}
	, m_audioControl{}
	, m_waveRam{}
{
	reset();
}

void APU::reset()
{
	SDL_AudioSpec spec{SDL_AudioFormat::SDL_AUDIO_F32, 1, 44100};
	m_audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	SDL_ResumeAudioStreamDevice(m_audioStream);
	std::ranges::fill(m_samplesBuffer, 0.0f);
	m_cycleCounter = 0;
	m_channel1 = Channel1{};
	m_channel2 = Channel2{};
	m_channel3 = Channel3{};
	m_channel4 = Channel4{};
	m_audioVolume = 0x77;
	m_audioPanning = 0xF3;
	m_audioControl = 0xF1;
	std::ranges::fill(m_waveRam, 0);
}

void APU::mCycle()
{
	++m_cycleCounter;
	if(!m_channel2.enabled || m_channel2.disableTimer == 0) return;

	constexpr int FRAME_SEQUENCER_TIME{2048}; //in m-cycles
	if(((m_cycleCounter % (FRAME_SEQUENCER_TIME * 2)) == 0))
	{
		m_channel2.disableTimerCycle();
		m_cycleCounter = 0;
	}
	for(int i{}; i < 4; ++i)
	{
		m_channel2.waveCycle();
		static int a{};
		if(++a == 95)
		{
			m_samplesBuffer.push_back(m_channel2.sample ? 1.0f : -1.0f);
			a = 0;
		}
	}
}

void APU::pushAudio()
{
	if(m_samplesBuffer.size() > ONE_FRAME_SAMPLES) m_samplesBuffer.resize(ONE_FRAME_SAMPLES);
	SDL_PutAudioStreamData(m_audioStream, m_samplesBuffer.data(), int(m_samplesBuffer.size() * sizeof(float)));
	m_samplesBuffer.clear();
}

uint8 APU::read(const Index index, uint8 waveRamIndex) const
{
	switch(index)
	{
	case CH1_SW: return m_channel1.sweep;
	case CH1_TIM_DUTY: return m_channel1.timerAndDuty & 0b1100'0000;
	case CH1_VOL_ENV: return m_channel1.volumeAndEnvelope;
	case CH1_PE_LOW: return 0xFF;
	case CH1_PE_HI_CTRL: return m_channel1.periodHighAndControl & 0b0100'0000;
	case CH2_TIM_DUTY: return m_channel2.timerAndDuty & 0b1100'0000;
	case CH2_VOL_ENV: return m_channel2.volumeAndEnvelope;
	case CH2_PE_LOW: return	0xFF;
	case CH2_PE_HI_CTRL: return m_channel2.periodHighAndControl & 0b0100'0000;
	case CH3_DAC_EN: return m_channel3.dacEnable;
	case CH3_TIM: return m_channel3.timer;
	case CH3_VOL: return m_channel3.volume;
	case CH3_PE_LOW: return m_channel3.periodLow;
	case CH3_PE_HI_CTRL: return m_channel3.periodHighAndControl;
	case CH4_TIM: return m_channel4.timer;
	case CH4_VOL_ENV: return m_channel4.volumeAndEnvelope;
	case CH4_FRE_RAND: return m_channel4.frequencyAndRandomness;
	case CH4_CTRL: return m_channel4.control;
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
	case CH1_SW: m_channel1.sweep = value; break;
	case CH1_TIM_DUTY: m_channel1.timerAndDuty = value; m_channel1.disableTimer = MAX_DISABLE_TIMER_DURATION - (value & 0b0011'1111); break;
	case CH1_VOL_ENV: m_channel1.volumeAndEnvelope = value; break;
	case CH1_PE_LOW: m_channel1.periodLow = value; break;
	case CH1_PE_HI_CTRL: 
		m_channel1.periodHighAndControl = value; 
		//if(constexpr uint8 TRIGGER{0x80}; value & TRIGGER) triggerChannel1();
		break;
	case CH2_TIM_DUTY: m_channel2.timerAndDuty = value; m_channel2.disableTimer = MAX_DISABLE_TIMER_DURATION - (value & 0b0011'1111); break;
	case CH2_VOL_ENV: m_channel2.volumeAndEnvelope = value; break;
	case CH2_PE_LOW: m_channel2.periodLow = value; break;
	case CH2_PE_HI_CTRL: 
		m_channel2.periodHighAndControl = value; 
		if(constexpr uint8 TRIGGER{0x80}; value & TRIGGER) m_channel2.trigger();
		break;
	case CH3_DAC_EN: m_channel3.dacEnable = value; break;
	case CH3_TIM: m_channel3.timer = value; break;
	case CH3_VOL: m_channel3.volume = value; break;
	case CH3_PE_LOW: m_channel3.periodLow = value; break;
	case CH3_PE_HI_CTRL: m_channel3.periodHighAndControl = value; break;
	case CH4_TIM: m_channel4.timer = value; break;
	case CH4_VOL_ENV: m_channel4.volumeAndEnvelope = value; break;
	case CH4_FRE_RAND: m_channel4.frequencyAndRandomness = value; break;
	case CH4_CTRL: m_channel4.control = value; break;
	case AU_VOL: m_audioVolume = value; break;
	case AU_PAN: m_audioPanning = value; break;
	case AU_CTRL: m_audioControl = value; break;
	case WAVE_RAM: m_waveRam[waveRamIndex] = value; break;
	}
}

bool APU::Channel2::waveCycle()
{
	if(--pushTimer == 0)
	{
		setPushTimer();
		constexpr uint8 DUTY_PATTERN{0b1100'0000};
		sample = DUTY_PATTERNS[timerAndDuty & DUTY_PATTERN >> 6][dutyStep];
		++dutyStep;
		dutyStep &= MAX_DUTY_STEP;
		return true;
	}
	return false;
}

void APU::Channel2::disableTimerCycle()
{
	if(disableTimer > 0) 
	{
		if(--disableTimer == 0)
		{
			enabled = false;
			sample = 0;
		}
	}
}

void APU::Channel2::trigger()
{
	enabled = true;
	setPushTimer();
	if(disableTimer == 0) disableTimer = MAX_DISABLE_TIMER_DURATION;
}

void APU::Channel2::setPushTimer()
{
	constexpr uint8 PERIOD_HIGH{0b111};
	int period{periodLow | ((periodHighAndControl & PERIOD_HIGH) << 8)};
	pushTimer = (4 * (2048 - period));
}

