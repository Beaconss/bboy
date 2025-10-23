#include <core/apu/apu.h>

APU::APU()
	: m_audioThread{*this}
	, m_audioStream{}
	, m_samplesBuffer{}
	, m_frameSequencerCounter{}
	, m_remainingCycles{}
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

APU::~APU()
{
	SDL_DestroyAudioStream(m_audioStream);
}

void APU::reset()
{
	m_audioThread.reset();
	SDL_AudioSpec spec{SDL_AudioFormat::SDL_AUDIO_F32, 1, FREQUENCY};
	SDL_ClearAudioStream(m_audioStream);
	m_audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	SDL_ResumeAudioStreamDevice(m_audioStream);
	std::ranges::fill(m_samplesBuffer, 0.0f);
	m_frameSequencerCounter = 0;
	m_remainingCycles = CYCLES_PER_FRAME;
	m_channel1 = Channel1{};
	m_channel2 = Channel2{};
	m_channel3 = Channel3{};
	m_channel4 = Channel4{};
	m_audioVolume = 0x77;
	m_audioPanning = 0xF3;
	m_audioControl = 0xF0;
	std::ranges::fill(m_waveRam, 0);
}

void APU::doCycles(const int cycleCount) 
{
	for(int i{}; i < cycleCount; ++i) mCycle();
	m_remainingCycles -= cycleCount;
}

void APU::unlockThread()
{
	m_audioThread.unlock();
}

void APU::doRemainingCycles()
{
	for(int i{}; i < m_remainingCycles; ++i) mCycle();
	m_remainingCycles = CYCLES_PER_FRAME;
}

void APU::putAudio(float frameTime)
{
	int thisFrameSamples{std::lroundf(FREQUENCY / (1000.f / frameTime))};
	if(m_samplesBuffer.size() > thisFrameSamples) m_samplesBuffer.resize(thisFrameSamples);
	SDL_PutAudioStreamData(m_audioStream, m_samplesBuffer.data(), int(m_samplesBuffer.size() * sizeof(float)));
	m_samplesBuffer.clear();
}

uint8 APU::read(const Index index, uint8 waveRamIndex)
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
	case AU_CTRL: return (m_audioControl & 0xF0) | (0u << 3 | 0u << 2 | static_cast<uint8>(m_channel2.enabled) << 1 | static_cast<uint8>(m_channel1.enabled));
	case WAVE_RAM: return m_waveRam[waveRamIndex];
	default: return 0xFF;
	}
}

void APU::write(const Index index, const uint8 value, uint8 waveRamIndex)
{
	if(index == WAVE_RAM)
	{
		m_waveRam[waveRamIndex] = value;
		return;
	}
	else if(index != AU_CTRL && !(m_audioControl & AUDIO_ENABLE)) return;

	switch(index)
	{
	case CH1_SW: m_channel1.sweep = value; break;
	case CH1_TIM_DUTY: m_channel1.timerAndDuty = value; m_channel1.disableTimer = MAX_DISABLE_TIMER_DURATION - (m_channel1.timerAndDuty & 0b0011'1111); break;
	case CH1_VOL_ENV: m_channel1.volumeAndEnvelope = value; break;
	case CH1_PE_LOW: m_channel1.periodLow = value; break;
	case CH1_PE_HI_CTRL: 
		m_channel1.periodHighAndControl = value; 
		if(value & TRIGGER) m_channel1.trigger();
		break;
	case CH2_TIM_DUTY: m_channel2.timerAndDuty = value; m_channel2.disableTimer = MAX_DISABLE_TIMER_DURATION - (m_channel2.timerAndDuty & 0b0011'1111); break;
	case CH2_VOL_ENV: m_channel2.volumeAndEnvelope = value; break;
	case CH2_PE_LOW: m_channel2.periodLow = value; break;
	case CH2_PE_HI_CTRL: 
		m_channel2.periodHighAndControl = value; 
		if(value & TRIGGER) m_channel2.trigger();
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
	case AU_CTRL: m_audioControl = value & 0x80; break;
	}
}

void APU::mCycle()
{
	constexpr int FRAME_SEQUENCER_TIME{2048}; //in m-cycles
	++m_frameSequencerCounter;
	if(((m_frameSequencerCounter % (FRAME_SEQUENCER_TIME * 2)) == 0))
	{
		m_channel1.disableTimerCycle();
		m_channel2.disableTimerCycle();
		m_frameSequencerCounter = 0;
	}

	if(!(m_audioControl & AUDIO_ENABLE)) return;

	for(int i{}; i < 4; ++i)
	{
		m_channel1.pushCycle();
		m_channel2.pushCycle();

		static int nearestNeighbourCounter{};
		if(++nearestNeighbourCounter == 95)
		{
			int sample{(m_channel1.enabled ? m_channel1.sample : 0) + (m_channel2.enabled ? m_channel2.sample : 0)};
			m_samplesBuffer.push_back(sample ? 0.005f : -0.005f);
			nearestNeighbourCounter = 0;
		}
	}
}

bool APU::PulseChannelBase::pushCycle()
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

void APU::PulseChannelBase::disableTimerCycle()
{
	if(disableTimer > 0) 
	{
		if(--disableTimer == 0) enabled = false;
	}
}

void APU::PulseChannelBase::trigger()
{
	enabled = true;
	setPushTimer();
	if(disableTimer == 0) disableTimer = MAX_DISABLE_TIMER_DURATION;
}

void APU::PulseChannelBase::setPushTimer()
{
	constexpr uint8 PERIOD_HIGH{0b111};
	int period{periodLow | ((periodHighAndControl & PERIOD_HIGH) << 8)};
	pushTimer = (4 * (2048 - period));
}