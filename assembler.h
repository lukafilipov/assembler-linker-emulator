#pragma once
#include <string>
#include <fstream>
#include <vector>

using namespace std;

struct SymbolTableEntry
{
	static int count;
	string name;
	string section;
	int value;
	char vis;
	int No;
	int size;
	SymbolTableEntry(string nam, string sec, int val, char visib, int siz, int num = count)
	{
		name = nam;
		section = sec;
		value = val;
		vis = visib;
		No = num;
		size = siz;
		count++;
	}
};

struct CodeSection
{
	string section;
	string code;
	CodeSection(string sec, string content)
	{
		section = sec;
		code = content;
	}
};
struct RelocTableEntry
{
	int ofset;
	char type;
	int No;
	RelocTableEntry(int of, char t, int n)
	{
		ofset = of;
		type = t;
		No = n;
	}
};

struct RelocTable
{
	string section;
	vector<RelocTableEntry> entries;
	RelocTable(string sec)
	{
		section = sec;
	}
};

class Assembler
{
private:
	string out;
	bool error;
	vector<RelocTable> reltab;
	vector<SymbolTableEntry> symtab;
	vector<string> globalsym;
	vector<CodeSection> code_sections;
	string filename;

	string addCond(string line, string mnem);
	string flag_change();
	string calculus(string line, string s);
	string logical(string line, string s);
	int reg_to_int(string s);
	int token(string line, string mnem);
	int get_sym_num(string s);
	bool containsIns(string s);
	void make_relocation(string sect, RelocTableEntry ent);
	
	friend struct SymbolTableEntry;
public:
	Assembler(string);
	void firstPass(string);
	void secondPass(string);
	void assembly();
	void output();
};