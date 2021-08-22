#pragma once
#include "assembler.h"

struct InputFile
{
	int file_no;
	vector<RelocTable> reltab;
	vector<SymbolTableEntry> symtab;
	vector<CodeSection> code_sections;
	InputFile(vector<RelocTable> r, vector<SymbolTableEntry> s, vector<CodeSection> c, int f)
	{
		reltab = r;
		symtab = s;
		code_sections = c;
		file_no = f;
	}
};

struct FinalSymbolEntry
{
	static int count;
	string name;
	int value;
	int file_no;
	int No;
	FinalSymbolEntry(string nam, int val, int n, int num = count)
	{
		name = nam;
		value = val;
		file_no = n;
		No = num;
		count++;
	}
};

struct RellocGlobal
{
	int offset;
	string symbol;
	RellocGlobal(int o, string s)
	{
		offset = o;
		symbol = s;
	}
};

struct RellocLocal
{
	int offset;
	string section;
	int file_no;
	RellocLocal(int o, string s, int n)
	{
		offset = o;
		section = s;
		file_no = n;
	}
};

struct FileSect
{
	string section;
	int file_no;
	FileSect(string s, int n)
	{
		section = s;
		file_no = n;
	}
};
class Linker
{
	vector<InputFile> files;
	vector<FileSect> sections;
	vector<string> globals;
	vector<RellocGlobal> rel_glob;
	vector<RellocLocal> rel_loc;
	vector<FinalSymbolEntry> final_symb_table;
	string *filenames;
	string script;
	string error;
	bool is_err;
	int num_of_files;

	vector<string> program;
	int dot;

public:
	Linker(int argc, string argv[]);
	void collect();
	int link();
	int sect_not_duplicate(string sect);
	bool symb_duplicate(string name);
	string hex_to_bin(char s);
	int calculate(string s);
	int return_value(string s);
	void resolve_relloc();
	vector<string> get_program();

};
