#pragma once
#include "type_alias.h"

namespace hardwareReg
{
	constexpr uint16 JOYP = 0xFF00; //joypad

	//Timers registers
	constexpr uint16 DIV = 0xFF04; //divider register
	constexpr uint16 TIMA = 0xFF05; //timer counter
	constexpr uint16 TMA = 0xFF06; //timer modulo
	constexpr uint16 TAC = 0xFF07; //timer control

	constexpr uint16 IF = 0xFF0F; //interrupt flag

	//PPU related registers
	constexpr uint16 LCDC = 0xFF40; //LCD control
	constexpr uint16 STAT = 0xFF41; //LDC status
	constexpr uint16 SCY = 0xFF42; //viewport y position
	constexpr uint16 SCX = 0xFF43; //viewport x position
	constexpr uint16 LY = 0xFF44; //LCD y coordinate (current scanline number)
	constexpr uint16 LYC = 0xFF45; //LY compare
	constexpr uint16 DMA = 0xFF46; //OAM DMA source address and start
	constexpr uint16 BGP = 0xFF47; //BG palette data
	constexpr uint16 OBP0 = 0xFF48; //OBJ palette 0 data
	constexpr uint16 OBP1 = 0xFF49; //OBJ palette 1 data
	constexpr uint16 WY = 0xFF4A; //window y position
	constexpr uint16 WX = 0xFF4B; //window x position plus 7

	constexpr uint16 IE = 0xFFFF; //interrupt enable
}