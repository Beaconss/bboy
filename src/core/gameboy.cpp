#include "gameboy.h"

Gameboy::Gameboy()
	: m_memoryBus{*this}
	, m_cpu{m_memoryBus}
	, m_ppu{m_memoryBus}
	, m_timers{m_memoryBus}
{
}

void Gameboy::cycle() //1 machine cycle
{
	m_memoryBus.cycle();
	for(int i{0}; i < 4; ++i) m_timers.cycle(); //both timers and ppu works with t-cycles, so 4 for machine cycle
	m_cpu.cycle();
	for(int i{0}; i < 4; ++i) m_ppu.cycle(); 
}

