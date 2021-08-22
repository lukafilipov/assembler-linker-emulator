#include "emulator.h"
#include <bitset>
#include <cmath>


Emulator::Emulator(vector<string> vec, int start)
{
	PC = start;
	SP = vec.size() + 1000;
	PSW = 0;
	program = vec;
	program.resize(program.size() + 1000, "00000000");
}

void Emulator::emulate()
{
	while (true)
	{
		instr = get_word(PC);
		PC += 4;
		if (instr.substr(4, 8) == "00000000") break;
		int cond = stoi(instr.substr(0, 3), nullptr, 2);
		switch (cond)
		{
		case 0:
			if (Z()) execute();
			break;
		case 1:
			if (!Z()) execute();
			break;
		case 2:
			if (!Z() && (N() == O())) execute();
			break;
		case 3:
			if (N() == O()) execute();
			break;
		case 4: 
			if (N() != O()) execute();
			break;
		case 5:
			if (Z() || (N() != O())) execute();
			break;
		case 7:
			execute();
		}
	}
}

void Emulator::execute()
{
	int type = stoi(instr.substr(4, 4), nullptr, 2);
	switch (type)
	{
	case 0:
		LR = PC;
		push(PSW);
		PC = stoi(instr.substr(8, 4), nullptr, 2) * 4;
		break;
	case 1:
		calculate(1);
		break;
	case 2:
		calculate(2);
		break;
	case 3:
		calculate(3);
		break;
	case 4:
		calculate(4);
		break;
	case 5:
		calculate(5);
		break;
	case 6:
		logical(6);
		break;
	case 7:
		logical(7);
		break;
	case 8:
		logical(8);
		break;
	case 9:
		logical(9);
		break;
	case 10:
	{
		int reg_a = stoi(instr.substr(8, 5), nullptr, 2);
		int reg_r = stoi(instr.substr(13, 5), nullptr, 2);
		int f = stoi(instr.substr(18, 3), nullptr, 2);
		char ls = instr[21];
		int imm = stoi(instr.substr(22, 10), nullptr, 2);
		if (reg_a == 20)
		{
			bitset<32> b(get(reg_r));
			if (f == 4) imm += 4; else if (f == 5) imm -= 4;
			if (ls == '1')set(reg_r, stoi(get_word(PC + imm), nullptr, 2));
			else set_word(stoi(get_word(PC + imm), nullptr, 2), b.to_string());
			if (f == 2) imm += 4; else if (f == 3) imm -= 4;
		}
		else
		{
			bitset<32> b(get(reg_r));
			if (f == 4) set(reg_a, reg_a + 4); else if (f == 5) set(reg_a, reg_a - 4);
			if (ls == '1') set(reg_r, stoi(get_word(get(reg_a) + imm), nullptr, 2));
			else set_word(get(reg_a) + imm, b.to_string());
			if (f == 2) imm += 4; else if (f == 3) imm -= 4;
		}
		break;
	}
	case 12:
	{
		int reg = stoi(instr.substr(8, 5), nullptr, 2);
		int imm = stoi(instr.substr(13, 19), nullptr, 2);
		push(PSW);
		if (reg == 20)
		{
			LR = PC;
			PC = PC + imm;
		}
		else
		{
			LR = PC;
			PC = get(reg) + imm;
		}
		break;
	}
	case 13:
		
		break;
	case 14:
	{
		int dst = stoi(instr.substr(8, 5), nullptr, 2);
		int src = stoi(instr.substr(13, 5), nullptr, 2);
		int imm = stoi(instr.substr(18, 5), nullptr, 2);
		if (instr[23] == '1') set(dst, get(src) << (unsigned)imm);
		else set(dst, get(src) >> (unsigned)imm);
		set_flags(get(src), imm, get(dst), 14);
		if ((instr[3] == '1') && (dst == 16)) PSW = stoi(pop(), nullptr, 2);
		break;
	}
	case 15:
	{
		int dst = stoi(instr.substr(8, 4), nullptr, 2);
		char hl = instr[12];
		int imm = stoi(instr.substr(16, 16), nullptr, 2);
		if (hl == '0') set(dst, imm);
		else set(dst, get(dst) + (imm << 16));
	}
	}
}

void Emulator::push(int val)
{
	bitset<32> bits(val);
	string a = bits.to_string();
	for (int k = 0; k < 4; k++)
	{
		SP -= 1;
		program[SP] = a.substr((3-k)*8, 8);
	}
}

string Emulator::pop()
{
	string ret = "";
	for (int k = 0; k < 4; k++)
	{
		ret = program[SP] + ret;
		SP += 1;
	}
	return ret;
}

int Emulator::get(int num)
{
	if (num < 16) return r[num];
	else switch (num)
	{
	case 16: return PC;
	case 17: return LR;
	case 18: return SP;
	case 19: return PSW;
	}
}

void Emulator::set(int num, int val)
{
	if (num < 16) r[num] = val;
	else switch (num)
	{
	case 16: PC = val; break;
	case 17: LR = val; break;
	case 18: SP = val; break;
	case 19: PSW = val; break;
	}
}

void Emulator::calculate(int num)
{
	if (instr[13] == '0')
	{
		int dst = stoi(instr.substr(8, 5), nullptr, 2);
		int src = stoi(instr.substr(14, 5), nullptr, 2);
		if (num == 1) {set_flags(get(dst), get(src), get(dst) + get(src), 1); set(dst, get(dst) + get(src)); }
		else if(num == 2) {set_flags(get(dst), get(src), get(dst) - get(src), 2); set(dst, get(dst) - get(src)); }
		else if (num == 3) {set_flags(get(dst), get(src), get(dst) * get(src)); set(dst, get(dst) * get(src)); }
		else if (num == 4) {set_flags(get(dst), get(src), get(dst) / get(src)); set(dst, get(dst) / get(src)); }
		else if (num == 5)  set_flags(get(dst), get(src), get(dst) - get(src), 2);
	}
	else
	{
		int dst = stoi(instr.substr(8, 5), nullptr, 2);
		int src = stoi(instr.substr(14, 18), nullptr, 2);
		if (num == 1) { set_flags(get(dst), src, get(dst) + src, 1); set(dst, get(dst) + src); }
		else if (num == 2) { set_flags(get(dst), src, get(dst) - src, 2); set(dst, get(dst) - src); }
		else if (num == 3) { set_flags(get(dst), src, get(dst) * src); set(dst, get(dst) * src); }
		else if (num == 4) { set_flags(get(dst), src, get(dst) / src); set(dst, get(dst) / src); }
		else if (num == 5)  set_flags(get(dst), src, get(dst) - src, 2);
	}
}

void Emulator::logical(int num)
{
		int dst = stoi(instr.substr(8, 5), nullptr, 2);
		int src = stoi(instr.substr(13, 5), nullptr, 2);
		if (num == 6) { set_flags(get(dst), get(src), get(dst) & get(src)); set(dst, get(dst) & get(src)); }
		else if (num == 7) { set_flags(get(dst), get(src), get(dst) | get(src)); set(dst, get(dst) | get(src)); }
		else if (num == 8) { set_flags(get(dst), get(src), ~get(src)); set(dst, ~get(src)); }
		else if (num == 9)  set_flags(get(dst), get(src), get(dst) & get(src));
}

void Emulator::set_flags(int val1, int val2, int res, int num)
{
	if (instr[3] == '1')
	{
		if (res == 0) PSW |= 8;
		if (res < 0) PSW |= 1;
		if (num == 1)
		{		
			if (((res >> 31) & 1 != (val1 >> 31) & 1) && ((((val2 >> 31) & 1) == (val1 >> 31) & 1))) PSW |= 4;
			else (PSW &= (~4));
			if ((unsigned)res < (unsigned)val1) PSW |= 2;
			else PSW &= (~2);
		}
		if ((num == 2) || (num == 5))
		{
			if (((res >> 31) & 1 != (val1 >> 31) & 1) && ((((val2 >> 31) & 1) == (val1 >> 31) & 1))) PSW |= 4;
			else (PSW &= (~4));
			if ((unsigned)res > (unsigned)val1) PSW |= 2;
			else PSW &= (~2);
		}
		if (num == 14)
		{
			if (instr[23] == '1')
			{
				if (val2 == 0) PSW &= (~2);
				else if (((unsigned)val1) & (1 << (32 - val2))) PSW |= 2;
				else PSW &= (~2);
			}
			else
			{
				if (val2 == 0) PSW &= (~2);
				else if (((unsigned)val1) & (1 >> (32 - val2))) PSW |= 2;
				else PSW &= (~2);
			}
		}
	}
}

string Emulator::get_word(int addr)
{
	string val = "";
	for (int k = 0; k < 4; k++)
	{
		val += program[addr + 3 - k];
	}
	return val;
}

void Emulator::set_word(int addr, string word)
{
	for (int k = 0; k < 4; k++)
	{
		program[addr + 3 - k] = word.substr(k * 8, 8);
	}
}

bool Emulator::Z()
{
	return PSW & 8;
}

bool Emulator::O()
{
	return PSW & 4;
}

bool Emulator::C()
{
	return PSW & 2;
}

bool Emulator::N()
{
	return PSW & 1;
}



