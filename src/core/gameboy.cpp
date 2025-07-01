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
	m_timers.cycle();
	m_memoryBus.cycle();
	m_cpu.cycle();
	for(int i{0}; i < 4; ++i) m_ppu.cycle(); //each cycle is treated as a t-cycle, so 4 per m-cycle
}

