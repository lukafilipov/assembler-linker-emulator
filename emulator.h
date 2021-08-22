#pragma once
#include <vector>
#include <string>

using namespace std;

class Emulator
{
	int r[16];
	int PC, SP, LR, PSW;

	//int stack_offset;
	//string stack[1000];
	vector<string> program;
	string instr;

public:
	Emulator(vector<string> program, int start);
	void emulate();
	string get_word(int addr);
	void set_word(int addr, string word);
	bool N();
	bool Z();
	bool C();
	bool O();
	void execute();
	void push(int val);
	string pop();
	int get(int num);
	void set(int num, int val);
	void calculate(int num);
	void logical(int num);
	void set_flags(int val1, int val2, int res, int num = 3);


};