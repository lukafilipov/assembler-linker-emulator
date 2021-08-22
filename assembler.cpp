#include "assembler.h"
#include <algorithm>
#include <iostream>
#include <bitset>

using namespace std;

int SymbolTableEntry::count = 0;

int first_not(string s, string f)
{
	if (s.find_first_not_of(f) != string::npos)
	{
		return s.find_first_not_of(f);
	}
	else
	{
		return s.length();
	}
}
int first(string s, string f)
{
	if (s.find_first_of(f) != string::npos)
	{
		return s.find_first_of(f);
	}
	else
	{
		return s.length();
	}
}

Assembler::Assembler(string s)
{
	filename = s;
	error = false;
}

void Assembler::assembly()
{
	firstPass(filename);
	secondPass(filename);
	output();
}

void Assembler::firstPass(string filename)
{
	string line;
	ifstream file(filename);
	if (file.is_open())
	{
		string section = "UND";
		int secsize = 0;
		while (getline(file, line))
		{
			int pos;
			string lable = line.substr(first_not(line, " "));
			if ((lable.find_first_of(':') != string::npos) && (section.compare("UND")))
			{
				lable = lable.substr(lable.find_first_not_of("\t "), lable.find_first_of(':') - lable.find_first_not_of("\t "));
				vector<SymbolTableEntry>::iterator it;
				for (it = symtab.begin(); it < symtab.end(); it++)
				{
					if (it->name == lable)
					{
						error = true;
						out = "Dvostruka definicija simbola";
					}
				}
				symtab.emplace_back(lable, section, secsize, 'l', 0);
			}
			if ((pos = line.find(".public")) != string::npos)
			{
				int pos1;
				if ((pos1 = line.find_first_not_of(" \n", pos + 7)) != string::npos)
				{
					int pos2;
					bool end = false;
					while (!end)
					{
						end = (pos2 = line.find_first_of(", ", pos1)) == string::npos;
						string sub = line.substr(pos1, pos2 - pos1);
						if (sub != " ") globalsym.push_back(sub);
						pos1 = line.find_first_not_of(", ", pos2);
					}
				}
			}
			else if ((pos = line.find(".extern")) != string::npos)
			{
				int pos1;
				if ((pos1 = line.find_first_not_of(" ", pos + 7)) != string::npos)
				{
					int pos2;
					bool end = false;
					while (!end)
					{
						end = (pos2 = line.find_first_of(", ", pos1)) == string::npos;
						string sub = line.substr(pos1, pos2 - pos1);
						if(sub != " ") symtab.emplace_back(sub, "UND", 0, 'g', 0);
						pos1 = line.find_first_not_of(", ", pos2);
					}
				}
			}
			else if ((pos = line.find(".text")) != string::npos)
			{
				if (section.compare("UND"))
				{
					symtab.emplace_back(section, section, 0, 'l', secsize);
				}
				string sub = line.substr(pos);
				if (sub.find_first_of(" ,", pos) != string::npos) sub = sub.substr(0, sub.find_first_of(" \n"));
				section = sub;
				secsize = 0;
			}
			else if ((pos = line.find(".data")) != string::npos)
			{
				if (section.compare("UND"))
				{
					symtab.emplace_back(section, section, 0, 'l', secsize);
				}
				string sub = line.substr(pos);
				if (sub.find_first_of(" ,", pos) != string::npos) sub = sub.substr(0, sub.find_first_of(" \n"));
				section = sub;
				secsize = 0;
			}
			else if ((pos = line.find(".bss")) != string::npos)
			{
				if (section.compare("UND"))
				{
					symtab.emplace_back(section, section, 0, 'l', secsize);
				}
				string sub = line.substr(pos);
				if (sub.find_first_of(" ,", pos) != string::npos) sub = sub.substr(0, sub.find_first_of(" \n"));
				section = sub;
				secsize = 0;
			}
			else if ((pos = line.find(".char")) != string::npos)
			{
				int numOfSep = count(line.begin(), line.end(), ',');
				secsize += numOfSep + 1;
				if (section.find("text") != string::npos) if (secsize % 4 != 0) secsize += (4-secsize%4);
			}
			else if ((pos = line.find(".word")) != string::npos)
			{
				int numOfSep = count(line.begin(), line.end(), ',');
				secsize += 2*(numOfSep + 1);
				if (section.find("text") != string::npos) if (secsize % 4 != 0) secsize += 2;
			}
			else if ((pos = line.find(".long")) != string::npos)
			{
				int numOfSep = count(line.begin(), line.end(), ',');
				secsize += 4*(numOfSep + 1);
			}
			else if ((pos = line.find(".align")) != string::npos)
			{
				line = line.substr(pos);
				string sub = line.substr(line.find_first_not_of(' ', pos + 6), string::npos);
				int val1 = stoi(sub);
				int numOfSep = count(line.begin(), line.end(), ',');
				if (numOfSep = 1)
				{
					sub = line.substr(line.find_first_not_of(' ', (line.find_last_of(','))));
					if (secsize%val1 != 0)
						if ((val1 - secsize%val1) <= stoi(sub)) secsize += (val1 - secsize%val1);
				}
				else
				{
					if (secsize%val1 != 0)
						secsize += (val1 - secsize%val1);
				}
				if (section.find("text") != string::npos) if (secsize % 4 != 0) secsize += (4 - secsize % 4);
			}
			else if ((pos = line.find(".skip")) != string::npos)
			{
				line = line.substr(pos);
				string sub = line.substr(line.find_first_not_of(' ', pos + 5), string::npos);
				secsize += stoi(sub);
				if(section.find("text") != string::npos) if (secsize % 4 != 0) secsize += (4 - secsize % 4);
			}
			else if (line.find(".end") != string::npos)
			{
				symtab.emplace_back(section, section, 0, 'l', secsize);
				break;
			}
			if (containsIns(line))
			{
				secsize += 4;
			}
		}
		vector<string>::iterator it;
		for (it = globalsym.begin(); it < globalsym.end(); it++)
		{
			vector<SymbolTableEntry>::iterator iter;
			for (iter = symtab.begin(); iter < symtab.end(); iter++)
			{
				if (*it == iter->name)
				{
					iter->vis = 'g';
					break;
				}
			}
		}
	}
	file.close();
}

void Assembler::secondPass(string filename)
{
	string line;
	ifstream file(filename);
	if (file.is_open())
	{
		string section = "UND";
		//string binary = "";
		string binary2 = "";
		int secsize = 0;
		while (getline(file, line) && (error == false))
		{
			int pos;
			if ((pos = line.find(".text")) != string::npos)
			{
				if (section.compare(".bss") && section.compare("UND") && (section.find(".bss") == string::npos))
				{
					CodeSection code(section, binary2);
					code_sections.push_back(code);
					binary2 = "";
				}
				string sub = line.substr(pos);
				if (sub.find_first_of(" ,", pos) != string::npos) sub = sub.substr(0, sub.find_first_of(" \n"));
				section = sub;
				secsize = 0;
			}
			else if ((pos = line.find(".data")) != string::npos)
			{
				if (section.compare(".bss") && section.compare("UND") && (section.find(".bss") == string::npos))
				{
					CodeSection code(section, binary2);
					code_sections.push_back(code);
					binary2 = "";
				}
				string sub = line.substr(pos);
				if (sub.find_first_of(" ,", pos) != string::npos) sub = sub.substr(0, sub.find_first_of(" \n"));
				section = sub;
				secsize = 0;
			}
			else if ((pos = line.find(".bss")) != string::npos)
			{
				if (section.compare(".bss") && section.compare("UND") && (section.find(".bss") == string::npos))
				{
					CodeSection code(section, binary2);
					code_sections.push_back(code);
					binary2 = "";
				}
				string sub = line.substr(pos);
				if (sub.find_first_of(" ,", pos) != string::npos) sub = sub.substr(0, sub.find_first_of(" \n"));
				section = sub;
				secsize = 0;
			}
			if ((pos = token(line, "int")) != -1)
			{
				string binary = "";
				binary += addCond(line, "int");
				binary += flag_change();
				binary += "0000";
				line = line.substr(pos);
				string s = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
				binary += bitset<4>(stoi(s)).to_string();
				for (int i = 0; i < 20; i++) binary += "0";
				for (int i = 0; i < 4; i += 1)
				{
					binary2 += binary.substr(32 - (i + 1) * 8, 8);
				}
				secsize += 4;
			}
			else if ((pos = token(line, "add")) != -1)
			{
				line = line.substr(pos);
				binary2 += calculus(line, "add");
				secsize += 4;
			}
			else if ((pos = token(line, "sub")) != -1)
			{
				line = line.substr(pos);
				binary2 += calculus(line, "sub");
				secsize += 4;
			}
			else if ((pos = token(line, "mul")) != -1)
			{
				line = line.substr(pos);
				binary2 += calculus(line, "mul");
				secsize += 4;
			}
			else if ((pos = token(line, "div")) != -1)
			{
				line = line.substr(pos);
				binary2 += calculus(line, "div");
				secsize += 4;
			}
			else if ((pos = token(line, "cmp")) != -1)
			{
				line = line.substr(pos);
				binary2 += calculus(line, "cmp");
				secsize += 4;
			}
			else if ((pos = token(line, "and")) != -1)
			{
				line = line.substr(pos);
				binary2 += logical(line, "and");
				secsize += 4;
			}
			else if ((pos = token(line, "or")) != -1)
			{
				line = line.substr(pos);
				binary2 += logical(line, "or");
				secsize += 4;
			}
			else if ((pos = token(line, "not")) != -1)
			{
				line = line.substr(pos);
				binary2 += logical(line, "not");
				secsize += 4;
			}
			else if ((pos = token(line, "test")) != -1)
			{
				line = line.substr(pos);
				binary2 += logical(line, "test");
				secsize += 4;
			}
			else if ((pos = token(line, "ldr")) != -1 || ((pos = token(line, "str")) != -1))
			{
				string binary = "";
				line = line.substr(pos);
				if (token(line, "ldr") != -1) binary += addCond(line, "ldr");
				else binary += addCond(line, "str");
				binary += flag_change();
				binary += "1010";
				int numOfSep = count(line.begin(), line.end(), ',');
				if (numOfSep == 3)
				{
					string a = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
					string r = a.substr(a.find_first_not_of(" ,", a.find_first_of(" ,")));
					string f = r.substr(r.find_first_not_of(" ,", r.find_first_of(" ,")));
					string imm = f.substr(f.find_first_not_of(" ,", f.find_first_of(" ,")));
					a = a.substr(0, a.find_first_of(" ,"));
					r = r.substr(0, r.find_first_of(" ,"));
					f = f.substr(0, f.find_first_of(" ,"));
					if (imm.find_first_of(" ") != string::npos) imm = imm.substr(0, imm.find_first_of(" \n"));
					int reg_a = reg_to_int(a);
					int reg_r = reg_to_int(r);
					binary += bitset<5>(reg_a).to_string();
					binary += bitset<5>(reg_r).to_string();
					binary += bitset<3>(stoi(f)).to_string();
					if (token(line, "ldr")) binary += "1";
					else binary += "0";
					binary += bitset<10>(stoi(imm)).to_string();
				}
				else
				{
					string r = line.substr(line.find_first_not_of(" ,", line.find_first_of(" ,")));
					string f = r.substr(r.find_first_not_of(" ,", r.find_first_of(" ,")));
					string imm = f.substr(f.find_first_not_of(" ,", f.find_first_of(" ,")));
					r = r.substr(0, r.find_first_of(" ,"));
					f = f.substr(0, f.find_first_of(" ,"));
					if (imm.find_first_of(" ") != string::npos)  imm = imm.substr(0, imm.find_first_of(" \n"));
					int reg_r = reg_to_int(r);
					binary += bitset<5>(20).to_string();
					binary += bitset<5>(reg_r).to_string();
					binary += bitset<3>(stoi(f)).to_string();
					if (token(line, "ldr")) binary += "1";
					else binary += "0";
					int val = -1;
					vector<SymbolTableEntry>::iterator it;
					for (it = symtab.begin(); it < symtab.end(); it++)
					{
						if ((imm == it->name) && (section == it->section))
						{
							val = it->value;
							break;
						}
					}
					if (val != -1)
					{
						if ((abs)(val - secsize) >= pow(2, 10))
						{
							error = true;
							out = "Greska na adresi " + to_string(secsize) + " sekcije " + section + ": Vrednost ne moze da stane";
							break;
						}
						binary += bitset<10>(val - secsize - 4).to_string();
					}
					else
					{
						error = true;
						out = "Greska na adresi " + to_string(secsize) + " sekcije " + section + ": Nije pronadjena labela";
						break;
					}
				}
				for (int i = 0; i < 4; i += 1)
				{
					binary2 += binary.substr(32 - (i + 1) * 8, 8);
				}
				secsize += 4;
			}
			else if ((pos = token(line, "call")) != -1)
			{
				string binary = "";
				line = line.substr(pos);
				binary += addCond(line, "call");
				binary += flag_change();
				binary += "1100";
				line = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
				string dst = line;
				if (line.find_first_of(" ") != string::npos) dst = line.substr(0, line.find_first_of(" ,\n"));
				int reg_dst = reg_to_int(dst);
				if (reg_dst != -1)
				{
					string imm = line.substr(line.find_first_not_of(" ,", line.find_first_of(" ,")));
					if (imm.find_first_of(" ") != string::npos) imm = imm.substr(0, imm.find_first_of(" \n"));
					binary += bitset<5>(reg_dst).to_string();
					binary += bitset<19>(stoi(imm)).to_string();
				}
				else
				{
					vector<SymbolTableEntry>::iterator it;
					int val = -1;
					for (it = symtab.begin(); it < symtab.end(); it++)
					{
						if ((dst == it->name) && (section == it->section))
						{
							val = it->value;
							break;
						}
					}
					if (val != -1)
					{
						if ((abs)(val - secsize) >= pow(2, 19))
						{
							error = true;
							out = "Greska na adresi " + to_string(secsize) + " sekcije " + section + ": Vrednost ne moze da stane";
							break;
						}
						binary += bitset<5>(20).to_string();
						binary += bitset<19>(val - secsize - 4).to_string();
					}
					else
					{
						error = true;
						out = "Greska na adresi " + to_string(secsize) +" sekcije " + section + ": Nije pronadjena labela";
						break;
					}
				}
				for (int i = 0; i < 4; i += 1)
				{
					binary2 += binary.substr(32 - (i + 1) * 8, 8);
				}
				secsize += 4;
			}
			else if (((pos = token(line, "in")) != -1) || ((pos = token(line, "out")) != -1))
			{
				string binary = "";
				line = line.substr(pos);
				if (token(line, "in") != -1) binary += addCond(line, "in");
				else binary += addCond(line, "out");
				binary += flag_change();
				binary += "1101";
				string dst = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
				string src = dst.substr(dst.find_first_not_of(" ,", dst.find_first_of(" ,")));
				dst = dst.substr(0, dst.find_first_of(" ,"));
				if (src.find_first_of(" ") != string::npos) src = src.substr(0, src.find_first_of(" \n"));
			    int reg_dst = reg_to_int(dst);
				int reg_src = reg_to_int(src);
				binary += bitset<4>(reg_dst).to_string();
				binary += bitset<4>(reg_src).to_string();
				if (((pos = line.find("in")) != string::npos)) binary += "1";
				else binary += "0";
				for (int i = 0; i < 15; i++) binary += "0";
				for (int i = 0; i < 4; i += 1)
				{
					binary2 += binary.substr(32 - (i + 1) * 8, 8);
				}
				secsize += 4;
			}
			else if ((pos = token(line, "mov")) != -1)
			{
				string binary = "";
				line = line.substr(pos);
				binary += addCond(line, "mov");
				binary += flag_change();
				binary += "1110";
				string dst = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
				string src = dst.substr(dst.find_first_not_of(" ,", dst.find_first_of(" ,")));
				dst = dst.substr(0, dst.find_first_of(" ,"));
				if (src.find_first_of(" ") != string::npos) src = src.substr(0, src.find_first_of(" \n"));
				int reg_dst = reg_to_int(dst);
				int reg_src = reg_to_int(src);
				binary += bitset<5>(reg_dst).to_string();
				binary += bitset<5>(reg_src).to_string();
				binary += "0";
				for (int i = 0; i < 13; i++) binary += "0";
				for (int i = 0; i < 4; i += 1)
				{
					binary2 += binary.substr(32 - (i + 1) * 8, 8);
				}
				secsize += 4;
			}
			else if (((pos = token(line, "shr")) != -1) || ((pos = token(line, "shl")) != -1))
			{
				string binary = "";
				line = line.substr(pos);
				if (token(line, "shr") != -1) binary += addCond(line, "shr");
				else binary += addCond(line, "shl");
				binary += flag_change();
				binary += "1110";
				string dst = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
				string src = dst.substr(dst.find_first_not_of(" ,", dst.find_first_of(" ,")));
				string imm = src.substr(src.find_first_not_of(" ,", src.find_first_of(" ,")));
				dst = dst.substr(0, dst.find_first_of(" ,"));
				src = src.substr(0, src.find_first_of(" ,"));
				if (imm.find_first_of(" ") != string::npos) imm = imm.substr(0, imm.find_first_of(" \n"));
				int reg_dst = reg_to_int(dst);
				int reg_src = reg_to_int(src);
				binary += bitset<5>(reg_dst).to_string();
				binary += bitset<5>(reg_src).to_string();
				binary += bitset<5>(stoi(imm)).to_string();
				if (((pos = line.find("shl")) != string::npos)) binary += "1";
				else binary += "0";
				for (int i = 0; i < 8; i++) binary += "0";
				for (int i = 0; i < 4; i += 1)
				{
					binary2 += binary.substr(32 - (i + 1) * 8, 8);
				}
				secsize += 4;
			}
			else if (((pos = token(line, "ldch")) != -1) || ((pos = token(line, "ldcl")) != -1))
			{
				string binary = "";
				line = line.substr(pos);
				if (token(line, "ldch") != -1) binary += addCond(line, "ldch");
				else binary += addCond(line, "ldcl");
				binary += flag_change();
				binary += "1111";
				line = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
				string dst = line.substr(0, line.find_first_of(" ,"));
				int reg_dst = reg_to_int(dst);
				string imm = line.substr(line.find_first_not_of(" ,", line.find_first_of(" ,")));
				if (imm.find_first_of(" ") != string::npos) imm = imm.substr(0, imm.find_first_of(" \n"));
				binary += bitset<4>(reg_dst).to_string();
				if (((pos = line.find("ldch")) != string::npos)) binary += "1";
				else binary += "0";
				binary += "000";
				vector<SymbolTableEntry>::iterator it;
				int val = -1;
				for (it = symtab.begin(); it < symtab.end(); it++)
				{
					if ((imm == it->name))
					{
						if (it->vis == 'l')
						{
							val = it->value;
							make_relocation(section, RelocTableEntry(secsize, 'A', get_sym_num(section)));
						}
						else
						{
							val = 0;
							make_relocation(section, RelocTableEntry(secsize, 'A', get_sym_num(imm)));
						}
						break;
					}
				}
				if (val != -1)
				{
					binary += bitset<16>(val).to_string();
				}
				else
				{
					try 
					{
						binary += bitset<16>(stoi(imm)).to_string();
					}
					catch (const invalid_argument& ia)
					{
						error = true;
						out = "Greska na adresi " + to_string(secsize) + " sekcije " + section + ": Ne postoji labela sa tim nazivom";
						break;
					}
				}
				for (int i = 0; i < 4; i += 1)
				{
					binary2 += binary.substr(32 - (i + 1) * 8, 8);
				}
				secsize += 4;
			}
			else if (((pos = token(line, "ldc")) != -1))
			{
				string binary = "";
				line = line.substr(pos);
				string temp = "";
				temp += addCond(line, "ldc");
				temp += flag_change();
				temp += "1111";
				line = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
				string dst = line.substr(0, line.find_first_of(" ,"));
				int reg_dst = reg_to_int(dst);
				string imm = line.substr(line.find_first_not_of(" ,", line.find_first_of(" ,")));
				if (imm.find_first_of(" ,") != string::npos) imm = imm.substr(0, imm.find_first_of(" \n"));
				temp += bitset<4>(reg_dst).to_string();
				temp += "0";
				temp += "000";
				vector<SymbolTableEntry>::iterator it;
				int val = -1;
				for (it = symtab.begin(); it < symtab.end(); it++)
				{
					if ((imm == it->name))
					{
						if (it->vis == 'l')
						{
							val = it->value;
							make_relocation(section, RelocTableEntry(secsize, 'A', get_sym_num(section)));
						}
						else
						{
							val = 0;
							make_relocation(section, RelocTableEntry(secsize, 'A', get_sym_num(imm)));
						}
						break;
					}
				}
				if (val != -1)
				{
					binary += temp;
					binary += bitset<16>(val).to_string();
					for (int i = 0; i < 4; i += 1)
					{
						binary2 += binary.substr(32 - (i + 1) * 8, 8);
					}
					temp[12] = '1';
					binary += temp;
					binary += bitset<16>(val>>16).to_string();
					for (int i = 0; i < 4; i += 1)
					{
						binary2 += binary.substr(32 - (i + 1) * 8, 8);
					}
				}
				else
				{
					try
					{
						binary += temp;
						binary += bitset<16>(stoi(imm)).to_string();
						for (int i = 0; i < 4; i += 1)
						{
							binary2 += binary.substr(32 - (i + 1) * 8, 8);
						}
						temp[12] = '1';
						binary += temp;
						binary += bitset<16>(stoi(imm) >> 16).to_string();
						for (int i = 0; i < 4; i += 1)
						{
							binary2 += binary.substr(32 - (i + 1) * 8, 8);
						}
					}
					catch (const invalid_argument& ia)
					{
						error = true;
						out = "Greska na adresi " + to_string(secsize) + " sekcije " + section + ": Ne postoji labela sa tim nazivom";
						break;
					}
				}
				secsize += 8;
			}
			else if ((pos = line.find(".char")) != string::npos)
			{
				line = line.substr(pos);
				int numOfSep = count(line.begin(), line.end(), ',');
				string sub = line;
				for (int i = 0; i <= numOfSep; i++)
				{
					sub = sub.substr(sub.find_first_not_of(" ,", sub.find_first_of(" ,")), string::npos);
					binary2 += bitset<8>(stoi(sub)).to_string();
					secsize += 1;
				}
				if ((section.find("text") != string::npos) && (secsize%4 != 0))
					for (int i = 0; i < (4 - secsize % 4); i ++)
					{
						binary2 += "00000000";
					}
				secsize += (4 - secsize % 4);
			}
			else if ((pos = line.find(".word")) != string::npos)
			{
				string binary = "";
				line = line.substr(pos);
				int numOfSep = count(line.begin(), line.end(), ',');
				string sub = line;
				for (int i = 0; i <= numOfSep; i++)
				{
					sub = sub.substr(sub.find_first_not_of(" ,", sub.find_first_of(" ,")), string::npos);
					binary += bitset<16>(stoi(sub)).to_string();
					for (int i = 0; i < 2; i += 1)
					{
						binary2 += binary.substr(16 - (i + 1) * 8, 8);
					}
					secsize += 2;
				}
				if (section.find("text") != string::npos)
					if (secsize % 4 != 0)
					{
						secsize += 2;
						binary += "0000000000000000";
					}
			}
			else if ((pos = line.find(".long")) != string::npos)
			{
				string binary = "";
				line = line.substr(pos);
				int numOfSep = count(line.begin(), line.end(), ',');
				string sub = line;
				vector<SymbolTableEntry>::iterator it;
				for (int i = 0; i <= numOfSep; i++)
				{
					int val = -1;
					sub = sub.substr(sub.find_first_not_of(" ,", sub.find_first_of(" ,")), string::npos);
					string imm = sub;
					if (sub.find_first_of(" ,") != string::npos) imm = sub.substr(0, sub.find_first_of(" ,\n"));
					for (it = symtab.begin(); it < symtab.end(); it++)
					{
						if ((imm == it->name))
						{
							if (it->vis == 'l')
							{
								val = it->value;
								make_relocation(section, RelocTableEntry(secsize, 'A', get_sym_num(section)));
							}
							else
							{
								val = 0;
								make_relocation(section, RelocTableEntry(secsize, 'A', get_sym_num(imm)));
							}
							break;
						}
					}
					if (val != -1)
					{
						binary += bitset<32>(val).to_string();
						for (int i = 0; i < 4; i += 1)
						{
							binary2 += binary.substr(32 - (i + 1) * 8, 8);
						}
					}
					else
					{
						try
						{
							binary += bitset<32>(stoi(imm)).to_string();
							for (int i = 0; i < 4; i += 1)
							{
								binary2 += binary.substr(32 - (i + 1) * 8, 8);
							}
						}
						catch (const invalid_argument& ia)
						{
							error = true;
							out = "Greska na adresi " + to_string(secsize) + " sekcije " + section + ": Ne postoji labela sa tim nazivom";
							break;
						}
					}
				}
				secsize += 4;
			}
			else if ((pos = line.find(".align")) != string::npos)
			{
				line = line.substr(pos);
				string sub = line.substr(line.find_first_not_of(' ', pos + 6), string::npos);
				int val1 = stoi(sub);
				int numOfSep = count(line.begin(), line.end(), ',');
				if (numOfSep = 1)
				{
					sub = line.substr(line.find_first_not_of(' ', (line.find_last_of(','))));
					if (secsize%val1 != 0)
						if ((val1 - secsize%val1) <= stoi(sub))
						{
							secsize += (val1 - secsize%val1);
							for (int i = 0; i < val1 - secsize%val1; i++)
							{
								binary2 += "00000000";
							}
						}
				}
				else
				{
					if (secsize%val1 != 0)
					{
						for (int i = 0; i < val1 - secsize%val1; i++)
						{
							binary2 += "00000000";
						}
						secsize += (val1 - secsize%val1);
					}

				}
				if ((section.find("text") != string::npos) && (secsize % 4 != 0))
					for (int i = 0; i < (4 - secsize % 4); i++)
					{
						binary2 += "00000000";
					}
				secsize += (4 - secsize % 4);
			}
			else if ((pos = line.find(".skip")) != string::npos)
			{
				line = line.substr(pos);
				string sub = line.substr(line.find_first_not_of(' ', pos + 5), string::npos);
				string fill = "";
				if (sub.find_first_of(',') != string::npos) fill = sub.substr(sub.find_first_not_of(", ", (sub.find_first_of(" ,\n"))));
				secsize += stoi(sub);
				if(fill != "")
				for (int i = 0; i < stoi(sub); i++)
				{
					binary2 += bitset<8>(stoi(fill)).to_string();
				}
				else
				for (int i = 0; i < stoi(sub); i++)
				{
					binary2 += bitset<8>(stoi(fill)).to_string();
				}
				if ((section.find("text") != string::npos) && (secsize % 4 != 0))
					for (int i = 0; i < (4 - secsize % 4); i++)
					{
						binary2 += "00000000";
					}
				secsize += (4 - secsize % 4);
			}
			else if (line.find(".end") != string::npos)
			{
				CodeSection code(section, binary2);
				code_sections.push_back(code);
				break;
			}
		}
	}
	file.close();
}

string Assembler::addCond(string line, string mnem)
{
	if (line[token(line, mnem) + mnem.length()] == ' ')
	{
		return "111";
	}
	string cond = line.substr(token(line, mnem) + mnem.length(), 2);
	if (cond == "eq")
	{
		return "000";
	}
	else if (cond == "ne")
	{
		return "001";
	}
	else if (cond == "gt")
	{
		return "010";
	}
	else if (cond == "ge")
	{
		return "011";
	}
	else if (cond == "it")
	{
		return "100";
	}
	else if (cond == "le")
	{
		return "101";
	}
	else if (cond == "al")
	{
		return "111";
	}
}

string Assembler::flag_change()
{
	return "1";
}

string Assembler::calculus(string line, string s)
{
	string binary = "";
	binary += addCond(line, s);
	binary += flag_change();
	if (s == "add") binary += "0001";
	else if(s == "sub") binary += "0010";
	else if (s == "mul") binary += "0011";
	else if (s == "div") binary += "0100";	
	else if (s == "cmp") binary += "0101";
	string dst = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
	string src = dst.substr(dst.find_first_not_of(" ,", dst.find_first_of(" ,")));
	dst = dst.substr(0, dst.find_first_of(" ,"));
	if (src.find_first_of(' ') != string::npos) src = src.substr(0, src.find_first_of(" \n"));
	int reg_dst = reg_to_int(dst);
	int reg_src = reg_to_int(src);
	if (reg_src != -1)
	{
		binary += bitset<5>(reg_dst).to_string();
		binary += "0";
		binary += bitset<5>(reg_src).to_string();
		for (int i = 0; i < 13; i++) binary += "0";
	}
	else
	{
		binary += bitset<5>(reg_dst).to_string();
		binary += "1";
		binary += bitset<18>(stoi(src)).to_string();
	}
	string bin = "";
	for (int i = 0; i < 4; i += 1)
	{
		bin += binary.substr(32 - (i + 1) * 8, 8);
	}
	return bin;
}

string Assembler::logical(string line, string s)
{
	string binary = "";
	binary += addCond(line, s);
	binary += flag_change();
	if (s == "and") binary += "0110";
	else if (s == "or") binary += "0111";
	else if (s == "not") binary += "1000";
	else if (s == "test") binary += "1001";
	string dst = line.substr(line.find_first_not_of(" ", line.find_first_of(" ")));
	string src = dst.substr(dst.find_first_not_of(" ,", dst.find_first_of(" ,")));
	dst = dst.substr(0, dst.find_first_of(" ,"));
	if(src.find_first_of(' ') != string::npos) src = src.substr(0, src.find_first_of(" \n"));
	int reg_dst = reg_to_int(dst);
	int reg_src = reg_to_int(src);
	binary += bitset<5>(reg_dst).to_string();
	binary += bitset<5>(reg_src).to_string();
	for (int i = 0; i < 14; i++) binary += "0";
	string bin = "";
	for (int i = 0; i < 4; i += 1)
	{
		bin += binary.substr(32 - (i + 1) * 8, 8);
	}
	return bin;
}

int Assembler::reg_to_int(string s)
{
	string reg;
	for (int i = 0; i < 16; i++)
	{
		reg = "r";
		reg += to_string(i);
		if (reg == s) return i;
	}
	std::transform(s.begin(), s.end(), s.begin(), toupper);
	if (s == "PC") return 16;
	if (s == "LR") return 17;
	if (s == "SP") return 18;
	if (s == "PSW") return 19;
	return -1;

}

int Assembler::token(string line, string s)
{
	string l = line;
	int p = line.find(s);
	int pos = 0;
	int val = -1;
	while ((p = l.find(s)) != string::npos)
	{
		if ((p == 0) || (l[p-1] == ' ') || (l[p - 1] == ':'))
		{
			if ((l[p + s.length()] == ' ') || ((toupper(l[p + s.length()]) == 'S') && (l[p + s.length() + 1] == ' ')))
			{
				pos += p;
				val = pos;
				break;
			}
			string cond = l.substr(l.find(s) + s.length(), 3);
			if ((cond == "eq ") || (cond == "ne ") || (cond == "gt ") || (cond == "ge ") ||
				(cond == "lt ") || (cond == "le ") || (cond == "al "))
			{
				val = p;
				break;
			}		
		}
		pos += p + 1;
		l = l.substr(p + 1);
	}
	return val;
}

int Assembler::get_sym_num(string s)
{
	vector<SymbolTableEntry>::iterator it;
	for (it = symtab.begin(); it < symtab.end(); it++)
	{
		if (it->name == s) return it->No;
	}
}

void Assembler::output()
{
	ofstream result;
	result.open("Izlaz.txt");
	if (error == true) result << out;
	else
	{
		vector<SymbolTableEntry>::iterator it;
		for (it = symtab.begin(); it < symtab.end(); it++)
		{
			result << it->name << " ";
			for (int i = 0; i < 14 - it->name.length(); i++) result << ' ';
			result << it->section << " ";
			for (int i = 0; i < 14 - it->section.length(); i++) result << ' ';
			result << it->value << " ";
			result << it->vis << " ";
			result << it->No << " ";
			result << it->size << endl;
		}
		result << endl << endl;
		int j;
		for (j = 0; j < reltab.size(); j++)
		{
			result << reltab[j].section <<endl;
			for (int i = 0; i < reltab[j].entries.size(); i++)
			{
				result << reltab[j].entries[i].ofset << " ";
				for (int k = 0; k < 5 - to_string(reltab[j].entries[i].ofset).length(); k++) result << ' ';
				result << reltab[j].entries[i].type << "  ";
				result << reltab[j].entries[i].No << endl;
			}
			result << endl;

		}

		for (j = 0; j < code_sections.size(); j++)
		{
			result << code_sections[j].section << endl;
			for (int i = 0; i < code_sections[j].code.length(); i++)
			{
				if ((i % 8 == 0) && (i>0)) result << " ";
				if ((i % 64 == 0) && (i>0)) result << endl;
				result << code_sections[j].code[i];
			}
			result << endl << endl;
		}
	}
}

bool Assembler::containsIns(string s)
{
	if (token(s, "int") != -1 || token(s, "add") != -1 || token(s, "sub") != -1 || token(s, "mul") != -1 || token(s, "div") != -1
		|| token(s, "cmp") != -1 || token(s, "and") != -1 || token(s, "or") != -1 || token(s, "not") != -1 || token(s, "test") != -1
		|| token(s, "ldr") != -1 || token(s, "str") != -1 || token(s, "call") != -1 || token(s, "in") != -1 || token(s, "out") != -1
		|| token(s, "mov") != -1 || token(s, "shr") != -1 || token(s, "shl") != -1 || token(s, "ldch") != -1 
		|| token(s, "ldcl") != -1)
		return true;
	else return false;
}

void Assembler::make_relocation(string sect, RelocTableEntry ent)
{
	vector<RelocTable>::iterator it;
	bool find = false;
	for (it = reltab.begin(); it < reltab.end(); it++)
	{
		if (it->section == sect)
		{
			it->entries.push_back(ent);
			find = true;
			break;
		}
	}
	if (find == false)
	{
		RelocTable tab(sect);
		tab.entries.push_back(ent);
		reltab.push_back(tab);
	}
}

