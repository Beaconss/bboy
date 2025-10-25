#include <core/apu/apu.h>

APU::APU()
	: m_audioThread{*this}
	, m_audioStream{}
	, m_samplesBuffer{}
	, m_timestamps{}
	, m_thisFrameTimestamps{}
	, m_frameSequencerCounter{}
	, m_remainingCycles{}
	, m_thisFrameSamples{}
	, m_nearestNeighbourCounter{}
	, m_nearestNeighbourTarget{}
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
	SDL_AudioSpec spec{SDL_AudioFormat::SDL_AUDIO_F32, 1, frequency};
	SDL_ClearAudioStream(m_audioStream);
	m_audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	SDL_ResumeAudioStreamDevice(m_audioStream);
	m_samplesBuffer.clear();
	std::queue<Timestamp> emptyQueue{};
	m_timestamps = emptyQueue;
	m_thisFrameTimestamps = emptyQueue;
	m_frameSequencerCounter = 0;
	m_remainingCycles = mCyclesPerFrame;
	m_thisFrameSamples = 0;
	m_nearestNeighbourCounter = 0;
	m_nearestNeighbourTarget = 0;
	m_channel1 = Channel1{};
	m_channel2 = Channel2{};
	m_channel3 = Channel3{};
	m_channel4 = Channel4{};
	m_audioVolume = 0x77;
	m_audioPanning = 0xF3;
	m_audioControl = 0xF0;
	std::ranges::fill(m_waveRam, 0);
}

void APU::setupFrame(float lastFrameTime)
{
	constexpr int tCyclesPerFrame{mCyclesPerFrame * 4};
	m_thisFrameSamples = static_cast<uint16>((frequency / std::lroundf((1000.f / lastFrameTime))));
	m_nearestNeighbourTarget = std::lroundf(static_cast<float>(tCyclesPerFrame / m_thisFrameSamples));
	m_nearestNeighbourCounter = 0;
}

void APU::doCycles(const int cycleCount) 
{
	for(int i{}; i < cycleCount; ++i) mCycle(i);
	m_remainingCycles -= cycleCount;
}

void APU::unlockThread()
{
	m_audioThread.unlock();
}

uint8 APU::read(const Index index, uint8 waveRamIndex)
{
	switch(index) 
	{
	case ch1Sw: return m_channel1.sweep;
	case ch1TimDuty: return (m_channel1.timerAndDuty & 0b1100'0000) | 0b0011'111;
	case ch1VolEnv: return m_channel1.volumeAndEnvelope;
	case ch1PeLow: return 0xFF;
	case ch1PeHighCtrl: return (m_channel1.periodHighAndControl & 0b0100'0000) | 0b1011'1111;
	case ch2TimDuty: return (m_channel2.timerAndDuty & 0b1100'0000) | 0b0011'1111;
	case ch2VolEnv: return m_channel2.volumeAndEnvelope;
	case ch2PeLow: return	0xFF;
	case ch2PeHighCtrl: return (m_channel2.periodHighAndControl & 0b0100'0000) | 0b1011'1111;
	case ch3DacEn: return m_channel3.dacEnable;
	case ch3Tim: return m_channel3.timer;
	case ch3Vol: return m_channel3.volume;
	case ch3PeLow: return m_channel3.periodLow;
	case ch3PeHighCtrl: return m_channel3.periodHighAndControl;
	case ch4Tim: return m_channel4.timer;
	case ch4VolEnv: return m_channel4.volumeAndEnvelope;
	case ch4FreRand: return m_channel4.frequencyAndRandomness;
	case ch4Ctrl: return m_channel4.control;
	case audioVolume: return m_audioVolume;
	case audioPanning: return m_audioPanning;
	case audioCtrl: return (m_audioControl & 0xF0) | (0u << 3 | 0u << 2 | static_cast<uint8>(m_channel2.enabled) << 1 | static_cast<uint8>(m_channel1.enabled));
	case waveRam: return m_waveRam[waveRamIndex];
	default: return 0xFF;
	}
}

void APU::write(const Index index, const uint8 value, const uint16 currentCycle, uint8 waveRamIndex)
{
	#ifdef _DEBUG
	std::cout << "RED LEGEND\n";
	if(!m_thisFrameTimestamps.empty())
	{
		for(const auto& timestamp : m_thisFrameTimestamps._Get_container())
		{
			if(timestamp.cycle > currentCycle) std::cout << "there is a timestamp at cycle " << timestamp.cycle 
										<< "\nand its writing a timestamp at cycle " << currentCycle << "\n\n";
		}
	}
	#endif
	if(!(m_audioControl & audioEnable) && index != audioCtrl && index != waveRam) return;
	m_thisFrameTimestamps.push(Timestamp{index, currentCycle, value, waveRamIndex});
}

void APU::mCycle(const int cycle)
{
	if(!(m_timestamps.empty()) && m_timestamps.front().cycle == cycle)
	{
		loadTimestamp(m_timestamps.front());
		m_timestamps.pop();
	}
	
	constexpr int frameSequencerTimer{2048}; //in m-cycles
	++m_frameSequencerCounter;
	if(((m_frameSequencerCounter % (frameSequencerTimer * 2)) == 0))
	{
		m_channel1.disableTimerCycle();
		m_channel2.disableTimerCycle();
		m_frameSequencerCounter = 0;
	}

	if(!(m_audioControl & audioEnable)) return;

	for(int i{}; i < 4; ++i)
	{
		m_channel1.pushCycle();
		m_channel2.pushCycle();

		if(++m_nearestNeighbourCounter == m_nearestNeighbourTarget)
		{
			int sample{(m_channel1.enabled ? m_channel1.sample : 0) + (m_channel2.enabled ? m_channel2.sample : 0)};
			m_samplesBuffer.push_back(sample ? 0.005f : -0.005f);
			m_nearestNeighbourCounter = 0;
		}
	}
}

void APU::loadTimestamp(const Timestamp& timestamp)
{
	switch(timestamp.registerToSet)
	{
	case ch1Sw: m_channel1.sweep = timestamp.value; break;
	case ch1TimDuty: m_channel1.setTimerAndDuty(timestamp.value); break;
	case ch1VolEnv: m_channel1.volumeAndEnvelope = timestamp.value; break;
	case ch1PeLow: m_channel1.periodLow = timestamp.value; break;
	case ch1PeHighCtrl: m_channel1.setPeriodHighAndControl(timestamp.value); break;
	case ch2TimDuty: m_channel2.setTimerAndDuty(timestamp.value); break;
	case ch2VolEnv: m_channel2.volumeAndEnvelope = timestamp.value; break;
	case ch2PeLow: m_channel2.periodLow = timestamp.value; break;
	case ch2PeHighCtrl: m_channel2.setPeriodHighAndControl(timestamp.value); break;
	case ch3DacEn: m_channel3.dacEnable = timestamp.value; break;
	case ch3Tim: m_channel3.timer = timestamp.value; break;
	case ch3Vol: m_channel3.volume = timestamp.value; break;
	case ch3PeLow: m_channel3.periodLow = timestamp.value; break;
	case ch3PeHighCtrl: m_channel3.periodHighAndControl = timestamp.value; break;
	case ch4Tim: m_channel4.timer = timestamp.value; break;
	case ch4VolEnv: m_channel4.volumeAndEnvelope = timestamp.value; break;
	case ch4FreRand: m_channel4.frequencyAndRandomness = timestamp.value; break;
	case ch4Ctrl: m_channel4.control = timestamp.value; break;
	case audioVolume: m_audioVolume = timestamp.value; break;
	case audioPanning: m_audioPanning = timestamp.value; break;
	case audioCtrl: m_audioControl = timestamp.value & 0x80; break;
	case waveRam: m_waveRam[timestamp.waveRamIndex] = timestamp.value; break;
	}
}

void APU::finishFrame()
{
	m_timestamps = m_thisFrameTimestamps;
	std::queue<Timestamp> empty{};
	m_thisFrameTimestamps = empty;
	for(int i{1}; i <= m_remainingCycles; ++i) mCycle(i);
	m_remainingCycles = mCyclesPerFrame;
	putAudio();
}

void APU::putAudio()
{
	m_samplesBuffer.resize(m_thisFrameSamples);
	SDL_PutAudioStreamData(m_audioStream, m_samplesBuffer.data(), int(m_samplesBuffer.size() * sizeof(float)));
	m_samplesBuffer.clear();
}

void APU::PulseChannel::pushCycle()
{
	if(--pushTimer == 0)
	{
		setPushTimer();
		constexpr uint8 dutyPattern{0b1100'0000};
		sample = dutyPatterns[timerAndDuty & dutyPattern >> 6][dutyStep];
		++dutyStep;
		dutyStep &= maxDutyStep;
		return;
	}
}

void APU::PulseChannel::disableTimerCycle()
{
	if(disableTimer > 0) 
	{
		if(--disableTimer == 0) enabled = false;
	}
}

void APU::PulseChannel::trigger()
{
	enabled = true;
	setPushTimer();
	if(disableTimer == 0) disableTimer = maxDisableTimerDuration;
}

void APU::PulseChannel::setPushTimer()
{
	constexpr uint8 periodHigh{0b111};
	int period{periodLow | ((periodHighAndControl & periodHigh) << 8)};
	pushTimer = (4 * (2048 - period));
}

void APU::PulseChannel::setTimerAndDuty(const uint8 value)
{
	timerAndDuty = value;
	disableTimer = maxDisableTimerDuration - (timerAndDuty & timer);
}

void APU::PulseChannel::setPeriodHighAndControl(const uint8 value)
{
	periodHighAndControl = value;
	constexpr uint8 triggerBit{0x80};
	if(value & triggerBit) trigger();
}