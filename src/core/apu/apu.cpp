#include "core/apu/apu.h"
#include "core/mmu.h"
#include <SDL3/SDL_audio.h>
#include <iostream>

APU::APU(MMU& mmu)
	: m_bus(mmu)
	, m_audioThread{*this}
	, m_audioStream{}
	, m_samplesBuffer{}
	, m_outSamples{}
	, m_frameSequencerCounter{}
	, m_frameSequencerStep{}
	, m_nextCycleToExecute{}
	, m_channel1{}
	, m_channel2{}
	, m_channel3{}
	, m_channel4{}
	, m_audioVolume{}
	, m_audioPanning{}
	, m_audioControl{}
{
	SDL_AudioSpec spec{SDL_AudioFormat::SDL_AUDIO_F32, 2, frequency};
	m_audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	SDL_ResumeAudioStreamDevice(m_audioStream);
	reset();
}

APU::~APU()
{
	SDL_DestroyAudioStream(m_audioStream);
}

void APU::reset()
{
	m_audioThread.waitToFinish();
	SDL_ClearAudioStream(m_audioStream);
	m_samplesBuffer.clear();
	m_outSamples.clear();
	m_frameSequencerCounter = 0;
	m_nextCycleToExecute = 1;
	m_channel1 = channels::SweepPulseChannel{};
	m_channel2 = channels::PulseChannel{};
	m_channel3 = channels::WaveChannel{};
	m_channel4 = channels::NoiseChannel{};
	m_audioVolume = 0x77;
	m_audioPanning = 0xF3;
	m_audioControl = 0xF0;
}

void APU::unlockThread()
{
	m_audioThread.unlock();
}

uint8 APU::read(const Index index, uint8 waveRamIndex)
{
	if(index == ch1PeLow || index == ch2PeLow || index == ch3Tim || index == ch3PeLow || index == ch4Tim) return 0xFF;

	catchUp();
	switch(index) 
	{
	case ch1Sw: return m_channel1.getSweep();
	case ch1TimDuty: return m_channel1.getTimerAndDuty();
	case ch1VolEnv: return m_channel1.getVolumeAndEnvelope();
	case ch1PeHighCtrl: return m_channel1.getPeriodHighAndControl();
	case ch2TimDuty: return m_channel2.getTimerAndDuty();
	case ch2VolEnv: return m_channel2.getVolumeAndEnvelope();
	case ch2PeHighCtrl: return m_channel2.getPeriodHighAndControl();

	case ch3DacEn: return m_channel3.getDacEnable();
	case ch3Vol: return m_channel3.getOutLevel();
	case ch3PeHighCtrl: return m_channel3.getPeriodHighAndControl();

	case ch4VolEnv: return m_channel4.getVolumeAndEnvelope();
	case ch4FreRand: return m_channel4.getFrequencyAndRandomness();
	case ch4Ctrl: return m_channel4.getControl();

	case audioVolume: return m_audioVolume;
	case audioPanning: return m_audioPanning;
	case audioCtrl: return m_audioControl | 0x70 | (m_channel4.isEnabled() << 3 | m_channel3.isEnabled() << 2 | (m_channel2.isEnabled()) << 1 | (m_channel1.isEnabled()));
	
	case waveRam: return m_channel3.getWaveRam(waveRamIndex);
	default: return 0xFF;
	}
}

void APU::write(const Index index, const uint8 value, uint8 waveRamIndex)
{	
	if(!(m_audioControl & audioEnable) && clearedWhenOff(index)) return;

	catchUp();
	switch(index)
	{
	case ch1Sw: m_channel1.setSweep(value); break;
	case ch1TimDuty: m_channel1.setTimerAndDuty(value); break;
	case ch1VolEnv: m_channel1.setVolumeAndEnvelope(value); break;
	case ch1PeLow: m_channel1.setPeriodLow(value); break;
	case ch1PeHighCtrl: m_channel1.setPeriodHighAndControl(value); break;
	case ch2TimDuty: m_channel2.setTimerAndDuty(value); break;
	case ch2VolEnv: m_channel2.setVolumeAndEnvelope(value); break;
	case ch2PeLow: m_channel2.setPeriodLow(value); break;
	case ch2PeHighCtrl: m_channel2.setPeriodHighAndControl(value); break;

	case ch3DacEn: m_channel3.setDacEnable(value); break;
	case ch3Tim: m_channel3.setTimer(value); break;
	case ch3Vol: m_channel3.setOutLevel(value); break;
	case ch3PeLow: m_channel3.setPeriodLow(value); break;
	case ch3PeHighCtrl: m_channel3.setPeriodHighAndControl(value); break;

	case ch4Tim: m_channel4.setTimer(value); break;
	case ch4VolEnv: m_channel4.setVolumeAndEnvelope(value); break;
	case ch4FreRand: m_channel4.setFrequencyAndRandomness(value); break;
	case ch4Ctrl: m_channel4.setControl(value); break;

	case audioVolume: m_audioVolume = value; break;
	case audioPanning: m_audioPanning = value; break;
	case audioCtrl: 
		m_audioControl = value & audioEnable; 
		if(!(m_audioControl & audioEnable)) clearRegisters();
		break;

	case waveRam: m_channel3.setWaveRam(value, waveRamIndex); break;
	}
}

void APU::clearRegisters()
{
	m_channel1.clearRegisters();
	m_channel2.clearRegisters();
	m_channel3.clearRegisters();
	m_channel4.clearRegisters();
}

bool APU::clearedWhenOff(Index reg) const
{
	return reg != ch1TimDuty
			&& reg != ch2TimDuty
			&& reg != ch3Tim
			&& reg != ch4Tim 
			&& reg != audioCtrl 
			&& reg != waveRam;
}

void APU::mCycle()
{
	constexpr int frameSequencerTarget{2048}; //in m-cycles
	if(++m_frameSequencerCounter == frameSequencerTarget)
	{
		m_frameSequencerCounter = 0;
		if((m_frameSequencerStep % 2) == 0)
		{
			m_channel1.disableTimerCycle();
			m_channel2.disableTimerCycle();
			m_channel3.disableTimerCycle();
			m_channel4.disableTimerCycle();
		}
		if(m_frameSequencerStep == 2 || m_frameSequencerStep == 6) m_channel1.sweepCycle();
		if(m_frameSequencerStep == 7)
		{
			m_channel1.envelopeCycle();
			m_channel2.envelopeCycle();
			m_channel4.envelopeCycle();
		}
		
		constexpr uint8 maxFrameSequencerStep{7};
		++m_frameSequencerStep;
		m_frameSequencerStep &= 7;	
	}
	
	if(!(m_audioControl & audioEnable)) return;
	
	m_channel1.pushCycle();
	m_channel2.pushCycle();
	m_channel3.pushCycle();
	m_channel3.pushCycle();
	m_channel4.pushCycle();

	constexpr uint8 ch1Right{0b1};
	constexpr uint8 ch2Right{0b10};
	constexpr uint8 ch3Right{0b100};
	constexpr uint8 ch4Right{0b1000};
	constexpr uint8 ch1Left{0b1'0000};
	constexpr uint8 ch2Left{0b10'0000};
	constexpr uint8 ch3Left{0b100'0000};
	constexpr uint8 ch4Left{0b1000'0000};

	float ch1Sample{digitalToAnalog[m_channel1.getSample()]};
	float ch2Sample{digitalToAnalog[m_channel2.getSample()]};
	float ch3Sample{digitalToAnalog[m_channel3.getSample()]};
	float ch4Sample{digitalToAnalog[m_channel4.getSample()]};

	float rightSample{((ch1Sample * (m_audioPanning & ch1Right)) +
					   (ch2Sample * ((m_audioPanning & ch2Right) >> 1)) +
					   (ch3Sample * ((m_audioPanning & ch3Right) >> 2)) +
					   (ch4Sample * ((m_audioPanning & ch4Right) >> 3)))
					   / 4.f};
	
	float leftSample{((ch1Sample * ((m_audioPanning & ch1Left) >> 4)) +
					  (ch2Sample * ((m_audioPanning & ch2Left) >> 5)) +
					  (ch3Sample * ((m_audioPanning & ch3Left) >> 6)) +
					  (ch4Sample * ((m_audioPanning & ch4Left) >> 7)))  
					  / 4.f};

	rightSample *= .8f;
	leftSample *= .8f;
    m_samplesBuffer.push_back(leftSample);
    m_samplesBuffer.push_back(rightSample);
}

void APU::catchUp() 
{
	m_audioThread.waitToFinish();
	for(int i{m_nextCycleToExecute}; i <= m_bus.currentCycle(); ++i) mCycle();
	m_nextCycleToExecute = m_bus.currentCycle() + 1;
}

void APU::finishFrame()
{
	for(int i{m_nextCycleToExecute}; i <= mCyclesPerFrame; ++i) mCycle();
	m_nextCycleToExecute = 1;
	putAudio();
}

void APU::putAudio()
{
	constexpr int target{static_cast<int>(((mCyclesPerFrame * 59.7) / frequency) + 1)};
	int counter{};
	for(size_t i{1}; i < mCyclesPerFrame; ++i)
	{
		if(i % target == 0)	
		{
			m_outSamples.push_back(m_samplesBuffer[counter]);
			m_outSamples.push_back(m_samplesBuffer[counter + 1]);
		}
		counter += 2;
	}
	
	if(SDL_GetAudioStreamQueued(m_audioStream) > frequency) SDL_ClearAudioStream(m_audioStream);
	SDL_PutAudioStreamData(m_audioStream, m_outSamples.data(), static_cast<int>(m_outSamples.size() * sizeof(float)));

	m_samplesBuffer.clear();
	m_outSamples.clear();
}