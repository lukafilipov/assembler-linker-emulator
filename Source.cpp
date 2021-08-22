#include "assembler.h"

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
				if (find(globalsym.begin(), globalsym.end(), lable) != globalsym.end())
				{
					symtab.emplace_back(lable, section, secsize, 'g', 0);
				}
				else
				{
					symtab.emplace_back(lable, section, secsize, 'l', 0);
				}
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
						globalsym.push_back(sub);
						pos1 = line.find_first_not_of(", ", pos2);
					}
				}
			}
			else if ((pos = line.find(".extern")) != string::npos)
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
						symtab.emplace_back(sub, "UND", 0, 'g', 0);
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
				string sub = line.substr(pos, line.find_first_of(" \n", pos) - pos);
				section = sub;
				secsize = 0;
			}
			else if ((pos = line.find(".data")) != string::npos)
			{
				if (section.compare("UND"))
				{
					symtab.emplace_back(section, section, 0, 'l', secsize);
				}
				string sub = line.substr(pos, line.find_first_of(" \n", pos) - pos);
				section = sub;
				secsize = 0;
			}
			else if ((pos = line.find(".bss")) != string::npos)
			{
				if (section.compare("UND"))
				{
					symtab.emplace_back(section, section, 0, 'l', secsize);
				}
				string sub = line.substr(pos, line.find_first_of(" \n", pos) - pos);
				section = sub;
				secsize = 0;
			}
			else if ((pos = line.find(".char")) != string::npos)
			{
				secsize += 1;
			}
			else if ((pos = line.find(".word")) != string::npos)
			{
				secsize += 2;
			}
			else if ((pos = line.find(".long")) != string::npos)
			{
				secsize += 4;
			}
			else if ((pos = line.find(".align")) != string::npos)
			{
				string sub = line.substr(line.find_first_not_of(' ', pos + 6), string::npos);
				int val1 = stoi(sub);
				int numOfSep = count(line.begin(), line.end(), ',');
				if (numOfSep > 1)
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

			}
			else if ((pos = line.find(".skip")) != string::npos)
			{
				string sub = line.substr(line.find_first_not_of(' ', pos + 5), string::npos);
				secsize += stoi(sub);
			}
			else if (containsIns(line))
			{
				secsize += 4;
			}
			else if (line.find(".end") != string::npos)
			{
				symtab.emplace_back(section, section, 0, 'l', secsize);
				break;
			}
		}
	}
	file.close();
}