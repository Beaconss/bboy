#include "timers.h"
#include "memory_bus.h"

Timers::Timers(MemoryBus& bus)
	: m_bus{bus}
	, m_timaResetCounter{}
	, m_lastAndResult{}
	, m_div{0xAB}
	, m_tima{}
	, m_tma{}
	, m_tac{0xF8}
{
}

//hope those 3 tests dont matter much, maybe I will fix them, maybe not
void Timers::cycle()
{
	++m_div;
	if(m_timaResetCounter >= TIMA_RESET_START_CYCLE)
	{
		if(++m_timaResetCounter == TIMA_RESET_END_CYCLE) //when 4 t-cycles passed(and not aborted) 
		{
			m_tima = m_tma;
			requestTimerInterrupt();
			m_timaResetCounter = 0;
		}
	}
	
	const uint16 divBit{static_cast<uint16>(m_div & ((timaBitPositions[m_tac & 0b11])))}; //formula to extract the right bit based on the tima frequency
	const bool result{static_cast<bool>(divBit && (m_tac & 0b100))}; //bit 2 is the enable bit

	if(m_lastAndResult && !result) //falling edge 
	{
		if(++m_tima == 0) m_timaResetCounter = TIMA_RESET_START_CYCLE;
	}
	
	m_lastAndResult = result;
}

uint8 Timers::read(const Index index) const
{	
	switch(index)
	{
	case DIV: return static_cast<uint8>(m_div >> 8); //in memory only div's upper 8 bits are mapped
	case TIMA: return m_tima;
	case TMA: return m_tma;
	case TAC: return m_tac;
	default: return 0xFF;
	}
}

void Timers::write(Index index, uint8 value)
{
	switch(index)
	{
	case DIV: m_div = 0; break;
	case TIMA:
		if(!(m_timaResetCounter == TIMA_RESET_END_CYCLE))
		{
			m_tima = value;
			m_timaResetCounter = 0;
		}
		break;
	case TMA: m_tma = value; break;
	case TAC: m_tac = value & 0x0F; break;
	}
}

void Timers::requestTimerInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF) | 0b100); //bit 2 is timer interrupt
}