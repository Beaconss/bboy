#include "gameboy.h"

Gameboy::Gameboy()
	: m_cpu{*this}
	, m_memory{}
	, m_timers{*this}
	, m_cycleCounter{}
{
}

//in the main loop I will batch a number of these and render afterwards
void Gameboy::cycle() //1 machine cycle
{
	++m_cycleCounter; //this is used to update things every x cycles
	m_cpu.cycle();

	//...
	m_timers.update(m_cycleCounter);
	if(m_cycleCounter % 256 == 0) m_cycleCounter = 0;
}

uint8 Gameboy::readMemory(const uint16 addr) const
{
	//TODO: account for memory regions
	return m_memory[addr]; 
}

void Gameboy::writeMemory(const uint16 addr, const uint8 value)
{
	using namespace hardwareReg;
	switch(addr)
	{
	case DIV: m_timers.write(Timers::DIV, value); break;
	case TIMA: m_timers.write(Timers::TIMA, value); break;
	case TMA: m_timers.write(Timers::TMA, value); break;
	case TAC: m_timers.write(Timers::TAC, value); break;
	case IF: m_memory[addr] = value & 0xF0; break;
	default: m_memory[addr] = value; break;
	}
}