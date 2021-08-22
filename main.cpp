#include <fstream>
#include "assembler.h"
#include "linker.h"
#include "emulator.h"
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
	/* string s(argv[1]);
	Assembler ass(s);
	ass.firstPass(s);
	ass.secondPass(s);
	ass.output(); */
	string *s = new string[argc];
	for (int i = 0; i < argc; i++) s[i] = argv[i];
	Linker l(argc, s);
	l.collect();
	int start = l.link();
	Emulator e(l.get_program(), start);
	e.emulate();
}