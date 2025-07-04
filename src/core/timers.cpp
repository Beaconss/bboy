#include "timers.h"
#include "memory_bus.h"

Timers::Timers(MemoryBus& bus)
	: m_bus{bus}
	, m_cycleCounter{}
	, m_div{0x18}
	, m_tima{}
	, m_tma{}
	, m_tac{0xF8}
	, m_updateTimaNextCycle{false}
{
}

//will need to remake this

uint8 Timers::read(const Index index) const
{
	switch(index)
	{
	case DIV: return m_div;
	case TIMA: return m_tima;
	case TMA: return m_tma;
	case TAC: return m_tac;
	default: return 0;
	}
}

void Timers::write(Index index, uint8 value)
{
	switch(index)
	{
	case DIV: m_div = 0; break; //when div is written to it gets reset
	case TIMA: 
		if(!m_updateTimaNextCycle) m_tima = value; //if this is the cycle where tima will be updated ignore writeMemory 
		break;
	case TMA: m_tma = value; break;
	case TAC: m_tac = value & 0x0F; break; //only lower 4 bits are used
	}
}

void Timers::cycle()
{  
	++m_cycleCounter; //after resetting to 0 go to 1  
	if(m_updateTimaNextCycle)  
	{  
		m_tima = m_tma;  
		timerInterrupt();  
		m_updateTimaNextCycle = false;  
	}  

	if(m_cycleCounter % 64 == 0) ++m_div; //update div every 64 cycles  

	if(m_tac & 0b100) //if bit 2(enable bit)  
	{  
		switch(m_tac & 0b11)  
		{  
		case FREQUENCY_256:  
			if(m_cycleCounter % 256 == 0)  
			{  
				if(++m_tima == 0x00) m_updateTimaNextCycle = true; //tima gets updated the cycle after it overflows  
			} break;  
		case FREQUENCY_4:  
			if(m_cycleCounter % 4 == 0)  
			{  
				if(++m_tima == 0x00) m_updateTimaNextCycle = true;  
			} break;  
		case FREQUENCY_16:  
			if(m_cycleCounter % 16 == 0)  
			{  
				if(++m_tima == 0x00) m_updateTimaNextCycle = true;  
			} break;  
		case FREQUENCY_64:  
			if(m_cycleCounter % 64 == 0)  
			{  
				if(++m_tima == 0x00) m_updateTimaNextCycle = true;   
			} break;  
		}   
	}  
	if(m_cycleCounter % 256 == 0) m_cycleCounter = 0; //reset to 0 at 256 as its the highest frequency  
}

void Timers::timerInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF) | 0b100); //bit 2 is timer interrupt
}
