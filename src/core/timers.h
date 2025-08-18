#pragma once
#include "../type_alias.h"
#include "../hardware_registers.h"
#include "bus.h"

#include <array>

class Bus;

class Timers
{
public:
	Timers(Bus& bus);

	enum Index
	{
		DIV,
		TIMA,
		TMA,
		TAC,
	};

	void reset();
	void cycle();
	uint8 read(const Index index) const;
	void write(const Index index, const uint8 value);

private:
	enum TimaFrequency //in t-cycles
	{
		FREQUENCY_1024,
		FREQUENCY_16,
		FREQUENCY_64,
		FREQUENCY_256,
	};

	static constexpr std::array<uint16, 4> timaBitPositions
	{
		1024 >> 1,
		16 >> 1,
		64 >> 1,
		256 >> 1
	};

	static constexpr uint8 TIMA_RESET_START_CYCLE{1};
	static constexpr uint8 TIMA_RESET_END_CYCLE{5};

	void requestTimerInterrupt() const;

	Bus& m_bus;

	uint8 m_timaResetCounter;
	bool m_lastAndResult;

	uint16 m_div; //divider register
	uint8 m_tima; //timer counter
	uint8 m_tma; //timer modulo
	uint8 m_tac; //timer control
};

