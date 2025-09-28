#pragma once
#include <type_alias.h>
#include <array>

namespace hardwareReg
{
	constexpr uint16 P1 = 0xFF00; //joypad

	//timers registers
	constexpr uint16 DIV{0xFF04}; //divider register
	constexpr uint16 TIMA{0xFF05}; //timer counter
	constexpr uint16 TMA{0xFF06}; //timer modulo
	constexpr uint16 TAC{0xFF07}; //timer control

	constexpr uint16 IF{0xFF0F}; //interrupt flag

	//apu registers
	constexpr uint16 CH1_SW{0xFF10};
	constexpr uint16 CH1_TIM_DUTY{0xFF11};
	constexpr uint16 CH1_VOL_ENV{0xFF12};
	constexpr uint16 CH1_PE_LOW{0xFF13};
	constexpr uint16 CH1_PE_HI_CTRL{0xFF14};
	constexpr uint16 CH2_TIM_DUTY{0xFF16};
	constexpr uint16 CH2_VOL_ENV{0xFF17};
	constexpr uint16 CH2_PE_LOW{0xFF18};
	constexpr uint16 CH2_PE_HI_CTRL{0xFF19};
	constexpr uint16 CH3_DAC_EN{0xFF1A};
	constexpr uint16 CH3_TIM{0xFF1B};
	constexpr uint16 CH3_VOL{0xFF1C};
	constexpr uint16 CH3_PE_LOW{0xFF1D};
	constexpr uint16 CH3_PE_HI_CTRL{0xFF1E};
	constexpr uint16 CH4_TIM{0xFF20};
	constexpr uint16 CH4_VOL_ENV{0xFF21};
	constexpr uint16 CH4_FRE_RAND{0xFF22};
	constexpr uint16 CH4_CTRL{0xFF23};
	constexpr uint16 AU_VOL{0xFF24};
	constexpr uint16 AU_PAN{0xFF25};
	constexpr uint16 AU_CTRL{0xFF26};
	constexpr std::array<uint16, 16> WAVE_RAM
	{
		0xFF30,
		0xFF31,
		0xFF32,
		0xFF33,
		0xFF34,
		0xFF35,
		0xFF36,
		0xFF37,
		0xFF38,
		0xFF39,
		0xFF3A,
		0xFF3B,
		0xFF3C,
		0xFF3D,
		0xFF3E,
		0xFF3F,
	};

	//PPU registers
	constexpr uint16 LCDC{0xFF40}; //LCD control
	constexpr uint16 STAT{0xFF41}; //LDC status
	constexpr uint16 SCY{0xFF42}; //viewport y position
	constexpr uint16 SCX{0xFF43}; //viewport x position
	constexpr uint16 LY{0xFF44}; //LCD y coordinate (current scanline number)
	constexpr uint16 LYC{0xFF45}; //LY compare
	constexpr uint16 DMA{0xFF46}; //OAM DMA source address and start
	constexpr uint16 BGP{0xFF47}; //BG palette data
	constexpr uint16 OBP0{0xFF48}; //OBJ palette 0 data
	constexpr uint16 OBP1{0xFF49}; //OBJ palette 1 data
	constexpr uint16 WY{0xFF4A}; //window y position
	constexpr uint16 WX{0xFF4B}; //window x position plus 7

	constexpr uint16 IE{0xFFFF}; //interrupt enable
}