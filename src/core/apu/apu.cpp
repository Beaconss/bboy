#include "core/apu/apu.h"
#include "core/bus.h"
#include <SDL3/SDL_audio.h>
#include <cmath>
#include <iostream>

APU::APU(Bus& bus)
	: m_bus(bus)
	, m_audioThread{*this}
	, m_audioStream{}
	, m_samplesBuffer{}
	, m_frameSequencerCounter{}
	, m_nextCycleToExecute{}
	, m_lastFrametime{}
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
	m_frameSequencerCounter = 0;
	m_nextCycleToExecute = 1;
	m_lastFrametime = 1000.f / 59.7f;
	setNearestNeighbour();
	m_channel1 = channels::Channel1{};
	m_channel2 = channels::Channel2{};
	m_channel3 = Channel3{};
	m_channel4 = Channel4{};
	m_audioVolume = 0x77;
	m_audioPanning = 0xF3;
	m_audioControl = 0xF0;
	std::fill(m_waveRam.begin(), m_waveRam.end(), 0);
}

void APU::unlockThread()
{
	m_audioThread.unlock();
}

void APU::setFrametime(float frametime)
{
	m_lastFrametime = frametime;
}

uint8 APU::read(const Index index, uint8 waveRamIndex)
{
	if(index == ch1PeLow || index == ch2PeLow) return 0xFF;
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
	case audioCtrl: return m_audioControl | 0x70 | (0u << 3 | 0u << 2 | static_cast<uint8>(m_channel2.isEnabled()) << 1 | static_cast<uint8>(m_channel1.isEnabled()));
	
	case waveRam: return m_waveRam[waveRamIndex];
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

	case ch3DacEn: m_channel3.dacEnable = value; break;
	case ch3Tim: m_channel3.timer = value; break;
	case ch3Vol: m_channel3.volume = value; break;
	case ch3PeLow: m_channel3.periodLow = value; break;
	case ch3PeHighCtrl: m_channel3.periodHighAndControl = value; break;
	case ch4Tim: m_channel4.timer = value; break;
	case ch4VolEnv: m_channel4.volumeAndEnvelope = value; break;
	case ch4FreRand: m_channel4.frequencyAndRandomness = value; break;
	case ch4Ctrl: m_channel4.control = value; break;

	case audioVolume: m_audioVolume = value; break;
	case audioPanning: m_audioPanning = value; break;
	case audioCtrl: 
		m_audioControl = value & audioEnable; 
		if(!(m_audioControl & audioEnable)) clearRegisters();
		break;

	case waveRam: m_waveRam[waveRamIndex] = value; break;
	}
}

void APU::clearRegisters()
{
	m_channel1.clearRegisters();
	m_channel2.clearRegisters();
}

bool APU::clearedWhenOff(Index reg) const
{
	return reg != ch1TimDuty
			&& reg != ch2TimDuty 
			&& reg != audioCtrl 
			&& reg != waveRam;
}

void APU::mCycle()
{
	constexpr int frameSequencerTimer{2048}; //in m-cycles
	++m_frameSequencerCounter;
	if(((m_frameSequencerCounter % (frameSequencerTimer * 2)) == 0))
	{
		m_channel1.disableTimerCycle();
		m_channel2.disableTimerCycle();
	}
	if(((m_frameSequencerCounter % (frameSequencerTimer * 4)) == 0)) m_channel1.sweepCycle();
	if((m_frameSequencerCounter % (frameSequencerTimer * 8)) == 0)
	{
		m_channel1.envelopeCycle();
		m_channel2.envelopeCycle();
		m_frameSequencerCounter = 0;
	}

	if(!(m_audioControl & audioEnable)) return;
	m_channel1.pushCycle();
	m_channel2.pushCycle();
	
	for(int i{}; i < 4; ++i)
	{
		if(++m_nearestNeighbourCounter == m_nearestNeighbourTarget)
		{
			m_nearestNeighbourCounter = 0;
			constexpr uint8 ch1Left{0b1'0000};
			constexpr uint8 ch1Right{0b1};
			constexpr uint8 ch2Left{0b10'0000};
			constexpr uint8 ch2Right{0b10};
	
			float ch1Sample{digitalToAnalog[m_channel1.getSample()]};
			float ch2Sample{digitalToAnalog[m_channel2.getSample()]};
	
			float leftSample{((m_audioPanning & ch1Left ? ch1Sample : 0) + 
							  (m_audioPanning & ch2Left ? ch2Sample : 0))
							  / 2.f};
			float rightSample{((m_audioPanning & ch1Right ? ch1Sample : 0) +
							  (m_audioPanning & ch2Right ? ch2Sample : 0))
							  / 2.f};
							  
			m_samplesBuffer.push_back(leftSample);			
			m_samplesBuffer.push_back(rightSample);
		}
	}
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
	setNearestNeighbour();
	m_nextCycleToExecute = 1;
	putAudio();
}

void APU::setNearestNeighbour()
{
	constexpr int tCyclesPerFrame{mCyclesPerFrame * 4};
	int thisFrameSamples{static_cast<int>(frequency / (1000.f / m_lastFrametime))};
	m_nearestNeighbourTarget = static_cast<uint32>(tCyclesPerFrame / thisFrameSamples);
	m_nearestNeighbourCounter = 0; 
}

void APU::putAudio()
{
	if(SDL_GetAudioStreamQueued(m_audioStream) > 20000) SDL_ClearAudioStream(m_audioStream);
	SDL_PutAudioStreamData(m_audioStream, m_samplesBuffer.data(), static_cast<int>(m_samplesBuffer.size() * sizeof(float)));
	m_samplesBuffer.clear();
}