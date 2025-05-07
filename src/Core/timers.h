#pragma once
#include "../type_alias.h"
#include "../hardware_registers.h"

class Gameboy;

class Timers
{
public:
	Timers(Gameboy& parent);

	enum Index
	{
		DIV,
		TIMA,
		TMA,
		TAC,
	};

	enum TimaFrequency
	{
		FREQUENCY_256,
		FREQUENCY_4,
		FREQUENCY_16,
		FREQUENCY_64,
	};

	void write(const Index index, const uint8 value);
	void update(const int cycleCounter);
	void requestInterrupt() const;

private:
	Gameboy& m_parent;
	uint8 m_div; //divider register
	uint8 m_tima; //timer counter
	uint8 m_tma; //timer modulo
	uint8 m_tac; //timer control
};

