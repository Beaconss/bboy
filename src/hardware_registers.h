#pragma once
#include "type_alias.h"

namespace hardwareReg
{
	constexpr uint16 DIV = 0xFF04; //divider register
	constexpr uint16 TIMA = 0xFF05; //timer counter
	constexpr uint16 TMA = 0xFF06; //timer modulo
	constexpr uint16 TAC = 0xFF07; //timer control
	constexpr uint16 IF = 0xFF0F; //interrupt flag
	constexpr uint16 IE = 0xFFFF; //interrupt enable
}