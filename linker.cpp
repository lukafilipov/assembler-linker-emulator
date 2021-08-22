#include "linker.h"
#include <bitset>
#include <sstream>

int FinalSymbolEntry::count = 0;

Linker::Linker(int argc, string argv[])
{
	num_of_files = argc - 2;
	filenames = new string[num_of_files];
	script = argv[1];
	for (int i = 2; i < argc; i++)
	{
		filenames[i - 2] = argv[i];
	}
	is_err = false;
	dot = 0;
}

void Linker::collect()
{
	string line;
	for (int i = 0; i < num_of_files; i++)
	{
		ifstream file(filenames[i]);
		vector<RelocTable> reltab;
		vector<SymbolTableEntry> symtab;
		vector<CodeSection> code_sections;
		if (file.is_open())
		{
			getline(file, line);
			if (line == "#SymbolTable")
			{
				string name, section;
				char type;
				int val, no, size;
				getline(file, line);
				while (line != "")
				{
					stringstream stream(line);
					stream >> name >> section >> val >> type >> no >> size;
					if ((size == 0) && (type == 'g') && (section != "UND"))
					{
						if (symb_duplicate(name) == true) break;
						globals.push_back(name);
					}
					else if ((size != 0))
					{
						int j;
						for (j = 0; j < sections.size(); j++)
						{
							if (sections[j].section.compare(name) > 0)
							{
								sections.insert(sections.begin() + j, FileSect(name, i));
								break;
							}
						}
						if (j == sections.size()) sections.emplace_back(name, i);
					}
					symtab.emplace_back(name, section, val, type, size, no);
					getline(file, line);
				}
				getline(file, line);
			}
			if (line == "#RellocationTable")
			{
				string offset;
				char type;
				int no;
				getline(file, line);
				string section = line.substr(1);
				getline(file, line);
				while (line != "")
				{
					vector<RelocTableEntry> rel_vec;
					while (line.find('#') == string::npos)
					{
						stringstream stream(line);
						stream >> offset >> type >> no;
						rel_vec.emplace_back(stoi(offset), type, no);
						getline(file, line);
					}
					RelocTable tab(section);
					tab.entries = rel_vec;
					reltab.push_back(tab);
					string section = line.substr(1);
					getline(file, line);
				}
				getline(file, line);
			}
			if (line == "#CodeSections")
			{
				string sec;
				string binary = "";
				getline(file, sec);
				while (getline(file, line))
				{
					if ((line[0] == '.') || (line == ""))
					{
						code_sections.emplace_back(sec, binary);
						binary = "";
						sec = line;
						getline(file, line);
					}
					string hex;
					stringstream stream(line);
					while (stream >> hex)
					{
						binary += hex_to_bin(hex[0]);
						binary += hex_to_bin(hex[1]);
					}
				}
			}
			files.emplace_back(reltab, symtab, code_sections, i);
		}
	}
}

int Linker::link()
{
	ifstream file(script);
	string line;
	while (getline(file, line) && (is_err == false))
	{
		if (line.find("=") != string::npos)
		{
			string sub = line.substr(0, line.find("="));
			int pos;
			if ((pos = sub.find('.')) != string::npos)
			{
				if ((pos == 0) || (line[pos - 1] == ' '))
				{
					if ((line[pos + 1] == ' ') || (line[pos + 1] == '='))
					{
						sub = line.substr(line.find("="), line.find_first_not_of(" ="));
						int res;
						if (((res = calculate(sub)) >= dot) && (is_err == false))
						{
							for (int i = dot; i < res; i++) program.push_back("00000000");
							dot = res;
						}
						else if((is_err == false))
						{
							is_err = true;
							error = "Vrednost dodeljena tacki je manja od dosadasnje";
						}
					}
				}
			}
			else
			{
				string sym = line.substr(line.find_first_not_of(" "), line.find_first_of(" ="));
				if (symb_duplicate(sym)) break;
				else
				{
					string sub = line.substr(line.find("="), line.find_first_not_of(" ="));
					int res = calculate(sub);
					if(is_err == false) final_symb_table.emplace_back(sym, res, -1);				
				}

			}
		}
		else
		{
			int file_no;
			if ((file_no = sect_not_duplicate(line)) != -1)
			{
				for (int i = 0; i < files[file_no].symtab.size(); i++)
				{
					if (files[file_no].symtab[i].section == line)
					{
						final_symb_table.emplace_back(files[file_no].symtab[i].name, files[file_no].symtab[i].value + dot, file_no);
					}
				}
				for (int i = 0; i < files[file_no].reltab.size(); i++)
				{
					if (files[file_no].reltab[i].section == line)
					{
						vector<RelocTableEntry> vec = files[file_no].reltab[i].entries;
						for (int j = 0; j < vec.size(); j++)
						{
							if (files[file_no].symtab[vec[j].No].size == 0)
							{
								rel_glob.emplace_back(vec[j].ofset + dot, files[file_no].symtab[vec[j].No].name);
							}
							else
							{
								rel_loc.emplace_back(vec[j].ofset + dot, files[file_no].symtab[vec[j].No].name, file_no);
							}
						}
						break;
					}
				}
				string binary;
				for (int i = 0; i < files[file_no].code_sections.size(); i++)
				{
					if (files[file_no].code_sections[i].section == line)
					{
						binary = files[file_no].code_sections[i].code;
						break;
					}
				}
				for (int i = 0; i < binary.length(); i += 8)
				{
					program.push_back(binary.substr(i, 8));
					dot += 1;
				}

			}

		}
	}
	for (int k = 0; k < sections.size(); k++)
	{
		int file_no = sections[k].file_no;
		string section = sections[k].section;
		for (int i = 0; i < files[file_no].symtab.size(); i++)
		{
			if (files[file_no].symtab[i].section == section)
			{
				final_symb_table.emplace_back(files[file_no].symtab[i].name, files[file_no].symtab[i].value + dot, file_no);
			}
		}
		for (int i = 0; i < files[file_no].reltab.size(); i++)
		{
			if (files[file_no].reltab[i].section == section)
			{
				vector<RelocTableEntry> vec = files[file_no].reltab[i].entries;
				for (int j = 0; j < vec.size(); j++)
				{
					if (files[file_no].symtab[vec[j].No].size == 0)
					{
						rel_glob.emplace_back(vec[j].ofset + dot, files[file_no].symtab[vec[j].No].name);
					}
					else
					{
						rel_loc.emplace_back(vec[j].ofset + dot, files[file_no].symtab[vec[j].No].name, file_no);
					}
				}
				break;
			}
		}
		string binary;
		for (int i = 0; i < files[file_no].code_sections.size(); i++)
		{
			if (files[file_no].code_sections[i].section == section)
			{
				binary = files[file_no].code_sections[i].code;
				break;
			}
		}
		for (int i = 0; i < binary.length(); i += 8)
		{
			program.push_back(binary.substr(i, 8));
			dot += 1;
		}
	}
	resolve_relloc();
	for (int i = 0; i < final_symb_table.size(); i++)
	{
		if (final_symb_table[i].name == "_start") return final_symb_table[i].value;
	}
	return -1;
}

void Linker::resolve_relloc()
{
	for (int i = 0; i < rel_glob.size(); i++)
	{
		int j;
		for (j = 0; j < final_symb_table.size(); j++)
		{
			if (rel_glob[i].symbol == final_symb_table[j].name)
			{
				string val = "";
				for (int k = 0; k < 4; k++)
				{
					val += program[rel_glob[i].offset + 3 - k];
				}
				int new_val = stoi(val, nullptr, 2) + final_symb_table[j].value;
				bitset<32> value(new_val);
				val = value.to_string();
				for (int k = 0; k < 4; k++)
				{
					program[rel_glob[i].offset + 3 - k] = val.substr(k * 8, 8);
				}
				break;
			}
		}
		if (j == final_symb_table.size())
		{
			is_err = true;
			error = "Ne moze se razresiti simbol na adresi" + rel_glob[i].offset;
			break;
		}
	}
	for (int i = 0; i < rel_loc.size(); i++)
	{
		int j;
		for (j = 0; j < final_symb_table.size(); j++)
		{
			if ((rel_glob[i].symbol == final_symb_table[j].name) && (rel_loc[i].file_no == final_symb_table[j].file_no))
			{
				string val = "";
				for (int k = 0; k < 4; k++)
				{
					val += program[rel_loc[i].offset + 3 - k];
				}
				int new_val = stoi(val, nullptr, 2) + final_symb_table[j].value;
				bitset<32> value(new_val);
				val = value.to_string();
				for (int k = 0; k < 4; k++)
				{
					program[rel_loc[i].offset + 3 - k] = val.substr(k * 8, 8);
				}
				break;
			}
		}
		if (j == final_symb_table.size())
		{
			is_err = true;
			error = "Ne moze se razresiti simbol na adresi" + rel_loc[i].offset;
			break;
		}
	}
}

vector<string> Linker::get_program()
{
	return program;
}

int Linker::sect_not_duplicate(string sec)
{
	int no = 0;
	int file_no, to_erase = 0, i = -1;
	vector<FileSect>::iterator it;
	for (it = sections.begin(); it < sections.end(); it++)
	{
		i++;
		if (it->section == sec)
		{
			no++;
			if (no == 1)
			{
				to_erase = i;
				file_no = it->file_no;
			}
		}
	}
	if (no > 1)
	{
		is_err = true;
		error = "Referise se sekcija koja nije jedinstvena " + sec;
	}
	else if (no == 0)
	{
		is_err = true;
		error = "Referise se nepostojeca sekcija " + sec;
	}
	if (is_err == true) return -1;
	else
	{
		sections.erase(sections.begin() + to_erase);
		return file_no;
	}
}

bool Linker::symb_duplicate(string name)
{
	vector<string>::iterator it;
	for (it = globals.begin(); it < globals.end(); it++)
	{
		if (*it == name)
		{
			is_err = true;
			error = "Dvostruka definicija simbola " + name;
			break;
		}
	}
	if (is_err == true) return true;
	else return false;
}

string Linker::hex_to_bin(char input)
{
	if (input >= '0' && input <= '9')
		return bitset<4>(input - '0').to_string();
	if (input >= 'A' && input <= 'F')
		return bitset<4>(input - 'A' + 10).to_string();
}

int Linker::return_value(string s)
{
	int result = 0;
	try
	{
		result = stoi(s);
	}
	catch (const invalid_argument& ia)
	{
		int i;
		for (i = 0; i < final_symb_table.size(); i++)
		{
			if (final_symb_table[i].name == s)
			{
				result = final_symb_table[i].value;
				break;
			}
		}
		if (i == final_symb_table.size())
		{
			is_err = true;
			error = "Do sada ne postoji takav simbol";
		}
	}
	if (is_err == true) return -1;
	else return result;

}

int Linker::calculate(string s)
{
	if (s.find("align") != string::npos)
	{
		string arg = s.substr(s.find_first_not_of(" ", s.find("align") + 6));
		if (arg[0] == '.')
		{
			arg = arg.substr(arg.find_first_not_of(" ,", arg.find_first_of(" ,")));
			if (arg.find(' ') != string::npos) arg.substr(0, arg.find_first_of(' '));
			return (dot + stoi(arg) - dot % stoi(arg));
		}
		else
		{
			string arg1 = arg.substr(0, arg.find_first_of(" ,"));
			arg = arg.substr(arg.find_first_not_of(" ,", arg.find_first_of(" ,")));
			if (arg.find_first_of(" ") != string::npos) arg = arg.substr(0, arg.find_first_of(" "));
			return (stoi(arg1) + stoi(arg) - stoi(arg1) % stoi(arg));
		}
	}
	else
	{
		string x1;
		bool end = false;
		if (s.find_first_of(" -+") != string::npos)
		{
			x1 = s.substr(0, s.find_first_of(" -+"));
			s = s.substr(s.find_first_of("-+"));
		}
		else end = true;
		int result = return_value(x1);
		while ((end == false) && (is_err == false))
		{
			string sign = s;
			s = s.substr(s.find_first_not_of("+- "));
			x1 = s;
			if (s.find_first_of(" -+") != string::npos)
			{
				x1 = s.substr(0, s.find_first_of(" -+"));
				s = s.substr(s.find_first_of("-+"));
			}
			else
			{
				end = true;
			}
			if (sign[0] == '+')
			{
				result += return_value(x1);
			}
			else
			{
				result -= return_value(x1);
			}
		}
		return result;
	}
}
