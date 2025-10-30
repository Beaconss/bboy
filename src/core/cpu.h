#pragma once
#include "type_alias.h"
#include <array>

class Bus;
class CPU
{
public:
	CPU(Bus& bus);
	void reset();
	void mCycle();

private:
	using InstructionHandler = void (CPU::*)(); //pointer to a instruction function
	
	struct IState //these values are used in multi-cycle instructions
	{
		uint8 x{};
		uint8 y{};
		uint8 z{};
		uint16 xx{};
		int8 e{};
	};

	enum Register8bit
	{
		b = 0,
		c = 1,
		d = 2,
		e = 3,
		h = 4,
		l = 5,
		indirectHl = 6, //indirect HL, so use HL as a memory address, never use this as an address in the m_registers array
		a = 7,  //accumulator  
	};

	enum Register16bit
	{
		bc = 0,
		de = 1,
		hl = 2,
		sp = 3,
		af = 3, //3 is SP in most instructions and AF in a few
	};

	enum Condition
	{
		notZero = 0, 
		zero = 1,  
		notCarry = 2,
		carry = 3,
	};

	static constexpr std::array<uint8, 5> interruptHandlerAddress 
	{
		0x40, //VBlank
		0x48, //Stat
		0x50, //Timer
		0x58, //Serial
		0x60, //Joypad
	};

	static constexpr uint8 zeroFlag{0b1000'0000};
	static constexpr uint8 negativeFlag{0b0100'0000};
	static constexpr uint8 halfCarryFlag{0b0010'0000};
	static constexpr uint8 carryFlag{0b0001'0000};
	
	void handleInterrupts();
	void interruptRoutine();
	void fetch();
	void execute();
	void endInstruction();

	uint8 getMsb(const uint16 in) const;
	uint8 getLsb(const uint16 in) const;

	uint16 getBc() const;
	uint16 getDe() const;
	uint16 getHl() const;
	void setBc(const uint16 bc);
	void setDe(const uint16 de);
	void setHl(const uint16 hl);

	bool getFz() const;
	bool getFn() const;
	bool getFh() const;
	bool getFc() const;
	void setFz(bool z);
	void setFn(bool n);
	void setFh(bool h);
	void setFc(bool c);

	//todo: HALT and STOP instructions

	//8-bit load instructions:
	void LD_r_r2();
	void LD_r_n();
	void LD_r_HL();
	void LD_HL_r();
	void LD_HL_n();
	void LD_A_BC();
	void LD_A_DE();
	void LD_BC_A();
	void LD_DE_A();
	void LD_A_nn();
	void LD_nn_A();
	void LDH_A_C();
	void LDH_C_A();
	void LDH_A_n();
	void LDH_n_A();
	void LD_A_HLd();
	void LD_HLd_A();
	void LD_A_HLi();
	void LD_HLi_A();

	//16-bit load instructions:
	void LD_rr_nn();
	void LD_nn_SP();
	void LD_SP_HL();
	void PUSH_rr();
	void POP_rr();
	void LD_HL_SP_e();

	//8-bit arithmetic and logical instructions:
	void ADD_r();
	void ADD_HL();
	void ADD_n();
	void ADC_r();
	void ADC_HL();
	void ADC_n();
	void SUB_r();
	void SUB_HL();
	void SUB_n();
	void SBC_r();
	void SBC_HL();
	void SBC_n();
	void CP_r();
	void CP_HL();
	void CP_n();
	void INC_r();
	void INC_HL();
	void DEC_r();
	void DEC_HL();
	void AND_r();
	void AND_HL();
	void AND_n();
	void OR_r();
	void OR_HL();
	void OR_n();
	void XOR_r();
	void XOR_HL();
	void XOR_n();
	void DAA();
	void CPL();
	void CCF();
	void SCF();

	//16-bit arithmetic instructions:
	void INC_rr();
	void DEC_rr();
	void ADD_HL_rr();
	void ADD_SP_e();

	//rotate, shift, and bit operations instructions:
	void RLCA();
	void RRCA();
	void RLA();
	void RRA();
	void RLC_r();
	void RLC_HL();
	void RRC_r();
	void RRC_HL();
	void RL_r();
	void RL_HL();
	void RR_r();
	void RR_HL();
	void SLA_r();
	void SLA_HL();
	void SRA_r();
	void SRA_HL();
	void SWAP_r();
	void SWAP_HL();
	void SRL_r();
	void SRL_HL();
	void BIT_b_r();
	void BIT_b_HL();
	void RES_b_r();
	void RES_b_HL();
	void SET_b_r();
	void SET_b_HL();

	//control flow instructions:
	void JP_nn();
	void JP_HL();
	void JP_cc_nn();
	void JR_e();
	void JR_cc_e();
	void CALL_nn();
	void CALL_cc_nn();
	void RET();
	void RET_cc();
	void RETI();
	void RST_n();

	//other:
	void HALT();
	void STOP();
	void DI();
	void EI();
	void NOP();

	Bus& m_bus;
	IState m_iState;
	void (CPU::*m_currentInstr)(); //pointer to a CPU function that returns void and take no parameters called m_currentInstr
	uint8 m_cycleCounter;

	//interrupt things
	bool m_ime; //interrupt enabler
	bool m_imeEnableNextCycle;
	bool m_isHalted;
	uint8 m_pendingInterrupts;
	uint8 m_interruptIndex;

	//register file:
	uint16 m_pc;
	uint16 m_sp;
	std::array<uint8, 8> m_registers;
	uint8 m_f; //flags
	uint8 m_ir; //instruction register
};