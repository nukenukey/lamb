#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <stack>

using namespace std;

// compiler for lamb syntax to nasm (the netwide assembler) to linux for x86-64 machines
// Copyright (C) 2025  Michael Burdick

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// feel free to me at <micklebubble22@gmail.com> if you have questions

string specChars[] = { ":", ";", "(", ")", " ", "\t", "!", ",", ".", "\'", "\"", "`", "&" };
// bool show = false;
// ofstream newFile;

struct morpheme {
	string label;
	string value;
};

struct morpheme moFact(string l, string v) {
	struct morpheme ret;
	ret.label = l;
	ret.value = v;
	return ret;
}

bool allDigit(string in) {
	for (int i = 0; i < (int)in.length(); i++) {
		if (!isdigit(in.at(i))) {
			return false;
		}
	}
	return true;
}

void error(uint line, string in) {
	cerr << "\t> error on line " << line << ": " << in << endl;
}

void fatal(string in) {
	cerr << in << endl;
	exit(1);
}

bool special(string in) {
	for (int i = 0; i < (int)(sizeof(specChars) / sizeof(string)); i++) {
		if (in.compare(specChars[i]) == 0) {
			return true;
		}
	}
	return false;
}

vector<morpheme> tokener (string in, uint lineC, bool debug) {
	vector<morpheme> ret;
	if (debug) {
		cout << "tokenizing line " << lineC << endl;
		cout << "peeks and buffs:" << endl;
	}
	string buff = "";
	string peek = "";
	bool isString = false;
	in += "`";
	for (uint i = 0; i < in.length(); i++) {
		peek = in.substr(i, 1);
		if (special(peek) && !(isString && (peek.at(0) == ' ' || peek.at(0) == '\t'))) {
			if (buff.length() > 0) {
				if (!isdigit(buff.at(0))) {
					ret.push_back(moFact("id", buff));
				}
				else {
					ret.push_back(moFact("intLit", buff));
				}
				if (peek.compare("`") == 0)
					continue;
			}
			if (peek.compare(")") == 0)
				ret.push_back(moFact("cParen", peek));
			else if (peek.compare("(") == 0)
				ret.push_back(moFact("oParen", peek));
			else if (peek.compare(":") == 0)
				ret.push_back(moFact("colon", peek));
			else if (peek.compare(";") == 0)
				ret.push_back(moFact("semi", peek));
			else if (peek.compare("!") == 0)
				ret.push_back(moFact("ex", peek));
			else if (peek.compare(",") == 0)
				ret.push_back(moFact("comma", peek));
			else if (peek.compare(".") == 0)
				ret.push_back(moFact("period", peek));
			else if (peek.compare("\'") == 0)
				ret.push_back(moFact("quo", peek));
			else if (peek.compare("&") == 0)
				ret.push_back(moFact("amp", peek));
			else if (peek.compare("\"") == 0) {
				if (isString)
					isString = false;
				else
					isString = true;
				ret.push_back(moFact("dQuo", peek));
			}
			buff = "";
		}
		else if (isalpha(peek.at(0)) || isdigit(peek.at(0)) || peek.at(0) == '_' || peek.at(0) == '\\' || isString || peek.at(0) == '/') {
			buff += peek;
		}
	}
	return ret;
}

vector<string> varNames;
unordered_map<string, uint> varMap;
unordered_map<string, uint> varSizeMap;
vector<string> retNames;
unordered_map<string, uint> retMap;
 // i dont really know how to use this yet but i still think i need it.
string funScope;
vector<string> scope;
 // tracks current scope
stack<uint> ifcount;
stack<uint> stackcount;
stack<uint> loopcount;
 // number to differentiate ifs and loops and stuff
uint ifVars;
uint stringLitCount = 0;
unordered_map<uint, string> stringLitMap;
unordered_map<string, vector<string>> params;

vector<morpheme> lexer(vector<morpheme> in, bool debug) {
	in.shrink_to_fit();
	if (in.empty())
		return in;
	vector<morpheme> ret;
	
	if (in.front().label.compare("colon") == 0) {
		ret.push_back(moFact("resolve", scope.back()));
		in[0].label = "\0";
		if (in.size() > 1 && in[1].label.compare("ex") == 0) {
			ret.push_back(moFact("ex", ""));
			in[1].label = "\0";
		}
	}
	if (in.front().label.compare("ex") == 0) {
		ret.push_back(moFact("ex", ""));
		in[0].label = "\0";
		if (in.size() > 1 && in[1].label.compare("colon") == 0) {
			ret.push_back(moFact("resolve", ""));
			in[1].label = "\0";
		}
	}
 // I dont love this if stuff
	for (uint i = 0; i < in.size(); i++) {
		if (in[i].label.compare("\0") == 0)
			continue;
		if (in[i].label.compare("intLit") == 0) {
			ret.push_back(moFact("intLit", in[i].value));
			continue;
		}
		if (in.size() >= 3 && in[i].label.compare("quo") == 0) {
			ret.push_back(moFact("charLit", in[i + 1].value));
			i += 3;
			continue;
		}
		if (in.size() >= 2 && in[i].label.compare("dQuo") == 0) {
			ret.push_back(moFact("stringLit", to_string(stringLitCount)));
			string cont;
			uint j;
			for (j = i + 1; in[j].label.compare("dQuo") != 0; j++) {
				cont += in[j].value;
			}
			stringLitMap[stringLitCount] = cont;
			stringLitCount++;
			i += j;
			continue;
		}
		else if (in.back().label.compare("colon") == 0) {
			if (in[i].label.compare("id") == 0) {
				if (in[i].value.compare("loop") == 0) {
					scope.push_back("loop");
					loopcount.top()++;
					loopcount.push(loopcount.top());
					ret.push_back(moFact("literal", "loop" + to_string(loopcount.top()) + ":\n"));
					break;
				}
				else if (in[i].value.compare("if") == 0) {
					scope.push_back("if");
					ifcount.top()++;
					ret.push_back(moFact("if", to_string(ifcount.top())));
					ifcount.push(ifcount.top());
					in.pop_back();
					continue;
				}
			}
		}
		else if (in[i].label.compare("amp") == 0) {
			ret.push_back(in[i]);
			continue;
		}
		else if (in[i].label.compare("id") == 0) {
			if (in.size() >= 3 && (i <= in.size() - 2) && (in[i + 1].label.compare("oParen") == 0)) {
				if (in.size() > 3 && in[in.size() - 2].label.compare("colon") == 0) {
					in[i].value = "_" + in[i].value;
					ret.push_back(moFact("funDef", in[i].value));
					retNames.push_back(in[i].value);
					funScope = in[i].value;
					scope.push_back("fun");
					if (in[i + 2].label.compare("cParen") != 0) {
						vector<string> temp;
						uint j;
						for (j = i + 2; in[j].label.compare("cParen") != 0; j++) {
							if (in[j].label.compare("comma") == 0)
								continue;
							temp.push_back(in[j].value);
						}
						if (debug) {
							cout << "params:" << endl;
							for (uint j = 0; j < temp.size(); j++) {
								cout << "\t" << temp[j] << endl;
							}
						}
						params[funScope] = temp;
						ret.push_back(moFact("literal", "sub rsp, " + to_string(temp.size() * 8) + "\n"));
						ret.push_back(moFact("literal", "mov [rbp], rdi\n"));
						if (temp.size() > 1)
							ret.push_back(moFact("literal", "mov [rbp - 8], rsi\n"));	
						if (temp.size() > 2)
							ret.push_back(moFact("literal", "mov [rbp - 16], rdx\n"));
						if (temp.size() > 3)
							ret.push_back(moFact("literal", "mov [rbp - 24], rcx\n"));
						if (temp.size() > 4)
							ret.push_back(moFact("literal", "mov [rbp - 32], r8\n"));
						if (temp.size() > 5)
							ret.push_back(moFact("literal", "mov [rbp - 40], r9\n"));
						if (temp.size() > 6)
							fatal("SORRY, no more than six params for now :<");
						cout << "TAKE PARAMS" << endl;
 // testing
 // take params
					}
					if (in.back().value.compare("null") != 0) {
						retMap[in[i].value] = (uint)stoi(in[in.size() - 1].value);
					}
					else {
						retMap[in[i].value] = 0;
					}
					break;
				}
				else {
 // funcCalls
					if (in[i + 2].label.compare("cParen") != 0) {	
						ret.push_back(moFact("startGiveParams", ""));
						vector<morpheme> temp;
						uint parens = 1;
						for (uint j = i + 2; parens > 0; j++) {
							if (in[j].label.compare("oParen") == 0)
								parens++;
							if (in[j].label.compare("cParen") == 0)
								parens--;
							if (in[j].label.compare("comma") == 0 || parens == 0) {
								vector<morpheme> rec = lexer(temp, debug);
								for (uint o = 0; o < rec.size(); o++)
									ret.push_back(rec[o]);
								temp.clear();
								temp.shrink_to_fit();
								continue;
							}
							temp.push_back(in[j]);
						}
						ret.push_back(moFact("endGiveParams", ""));
					}
					ret.push_back(moFact("funCall", "_" + in[i].value));
					break;
				}
			}
			else {
 // keywords
				if (in[i].value.compare("import") == 0) {
					string toImport;
					for (uint j = i + 1; j < in.size(); j++)
						toImport += in[j].value;
					ret.push_back(moFact("import", toImport));
					if (debug)
						cout << "  import:" << endl << toImport << endl;
					break;
				}
				else if (in[i].value.compare("ret") == 0) {
					ret.push_back(moFact("ret", ""));
				}
				else if (in[i].value.compare("else") == 0) {
					ret.push_back(moFact("else", to_string(ifcount.top())));
				}
				else if (in[i].value.compare("go") == 0) {
					ret.push_back(moFact("goto", in[i + 1].value));
				}
				else if (in[i].value.compare("set") == 0) {
					ret.push_back(moFact("set", ""));
				}
				else if (in[i].value.compare("break") == 0) {
					for (uint j = scope.size(); scope[j].compare("loop") != 0 || j > 0; j--) {}
					ret.push_back(moFact("literal", "jmp loop" + to_string(loopcount.top() - 1) + "End:\n"));
				}
				else {
 // references
					bool temp = false;
					for (uint j = 0; j < params[funScope].size(); j++) {
						if (params[funScope][j].compare(in[i].value) == 0) {
							temp = true;
							if (in.size() > (i + 1)) {
								if (in[i + 1].label.compare("amp") == 0) {
									ret.push_back(in[i + 1]);
									in[i + 1].label = "\0";
								}
							}
							if (j == 0)
								ret.push_back(moFact("param", "[rbp]"));
							else
								ret.push_back(moFact("param", "[rbp - " + to_string(j * 8) + "]"));
							break;
						}
		// ughhhhhhhh this sucks so much
					}
					if (!temp)
						fatal("unrecognized reference of value: " + in[i].value);
				}
				continue;
			}
		}
		else if (in[i].label.compare("semi") == 0) {
			if (scope.back().compare("loop") == 0) {
				loopcount.pop();
				ret.push_back(moFact("literal", "jmp loop" + to_string(loopcount.top()) + "\n"));
				ret.push_back(moFact("literal", "loop" + to_string(loopcount.top()) + "End:\n"));
			}
			else if (scope.back().compare("if") == 0) {
				ifcount.pop();
				ret.push_back(moFact("literal", "if" + to_string(ifcount.top()) + "End:\n"));
			}
			else if (scope.back().compare("else") == 0) {
				ret.push_back(moFact("literal", "else" + to_string(ifcount.top()) + "End:\n"));
			}
			else {
				ret.push_back(moFact("closeScope", ""));
			}
			scope.pop_back();
			continue;
		}
		fatal("big bad error\nlexer could not resolve morpheme of type and value\n\t" + in[i].label + "\n\t" + in[i].value);
	}
	return ret;
}

vector<string> imports;
bool needsImport = false;

void importPls(string in, bool debug, ifstream &configFile, ofstream &newFile) {
	vector<string> ret;
	if (!configFile.is_open()) {
		fatal("config file does not exist (/etc/lamb.conf unless specified otherwise)");
	}
	string path;
	while (getline(configFile, path)) {
		if (path.substr(0, path.find(" ")).compare("lib") == 0) {
			path = path.substr(path.find(" ") + 1);
		}
	}
	if (path.empty()) {
		fatal("config file must have `\"homeDirec\": \"/path/to/lamb/lib\"`");
	}
	for (string t; (bool)(in.find(".") % 1); t = in.substr(0, in.find("."))) {
		in = in.substr(in.find(".") + 1);
		path += t + "/";
	}
	path += in;
	path = path.substr(0, path.length()) + ".asm";
	if (debug) {
		cout << "\t> importing from file " << path << endl;
	}
	ifstream importFile(path);
	if (!importFile.is_open()) {
		fatal("import file " + path + " does not exist");
	}
	while (getline(importFile, path))
		newFile << path << endl;
}

void commit(vector<morpheme> in, bool addLineIfNeeded, ofstream &newFile, bool debug) {
	in.shrink_to_fit();
	if (in.empty())
		return;
	bool amp = false;
	bool needsLine = false;
	for (uint i = 0; i < in.size(); i++) {
		if (amp)
			amp = false;
		if (in[i].label.compare("amp") == 0) {
			amp = true;
			i++;
		}
		if (in[i].label.compare("resolve") == 0) {
			if (amp)
				newFile << "lea ";
			else
				newFile << "mov ";
			newFile << "rax, ";
			needsLine = true;
			i++;
		}
		if (in[i].label.compare("funDef") == 0) {
			newFile << in[i].value << ":" << endl;
			newFile << "push rbp" << endl;
			newFile << "mov rbp, rsp" << endl;
			continue;
		}
		if (in[i].label.compare("intLit") == 0) {
			newFile << in[i].value;
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("charLit") == 0) {
			newFile << "\'" << in[i].value << "\'";
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("import") == 0) {
			imports.push_back(in[i].value);
			needsImport = true;
			if (debug)
				cout << "will import from: " << in[i].value << endl;
			continue;
		}
		if (in[i].label.compare("else") == 0) {
			newFile << "jmp else" << ifcount.top() << "End" << endl;
			continue;
		}
		if (in[i].label.compare("funCall") == 0) {
			newFile << "call " << in[i].value << endl;
			continue;
		}
		if (in[i].label.compare("ref") == 0) {
			newFile << "[rbp - " << retMap[in[i].value] << "]";
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("closeScope") == 0) {
			newFile << "pop rbp" << endl;
			newFile << "ret" << endl;
			continue;
		}
		if (in[i].label.compare("ret") == 0) {
			newFile << "ret" << endl;
			continue;
		}
		if (in[i].label.compare("ex") == 0) {
			if (in[i + 1].label.compare("funCall") == 0) {
				newFile << "call " << in[i + 1].value << endl;
				if (amp)
					newFile << "lea r8, rax" << endl;
				else
					newFile << "mov r8, rax" << endl;
			}
			else {
				if (amp)
					newFile << "lea r8, ";
				else
					newFile << "mov r8, ";
				needsLine = true;
			}
			continue;
		}
		if (in[i].label.compare("openScope") == 0) {
			newFile << "push rbp" << endl;
			newFile << "mov rbp, rsp" << endl;
			continue;
		}
		if (in[i].label.compare("go") == 0) {
			newFile << "jmp " << in[i].value << endl;
			continue;
		}
		if (in[i].label.compare("param") == 0) {
			newFile << in[i].value;
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("stringLit") == 0) {
			newFile << "s_" << in[i].value;
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("if") == 0) {
			if (in[i + 1].label.compare("startGiveParams") == 0) {
				vector<morpheme> temp;
				uint j;
				for (j = i + 1; in[j - 1].label.compare("endGiveParams") != 0; j++) {
					temp.push_back(in[j]);
				}
				commit(temp, true, newFile, debug);
				newFile << "call " << in[i + j].value << endl;
				newFile << "cmp rax, 1" << endl;
			}
			else {
				newFile << "cmp ";
				vector<morpheme> temp;
				for (uint j = i + 1; j < in.size(); j++)
					temp.push_back(in[j]);
				commit(temp, true, newFile, debug);
				newFile << ", 1";
			}
			newFile << "jne if" << in[i].value << "End" << endl;
			continue;
		}
		if (in[i].label.compare("startGiveParams") == 0) {
			uint j = i + 1;
			uint offset = 0;
			for (; in[j].label.compare("endGiveParams") != 0; j++) {
				bool funcCall = false;
				if (in[j].label.compare("funCall") == 0) {
					newFile << "call " << in[j].value << endl;
					funcCall = true;
				}
				else if (in[j].label.compare("startGiveParams") == 0) {
					funcCall = true;
					vector<morpheme> tempVec;
					tempVec.push_back(in[j]);
					uint o;
					uint giveParamsCount = 1;
					for (o = j + 1; giveParamsCount != 0 || in[o].label.compare("funCall") != 0; o++) {
						if (in[o].label.compare("startGiveParams") == 0) {
							giveParamsCount++;
						}
						if (in[o].label.compare("endGiveParams") == 0) {
							giveParamsCount--;
						}
						tempVec.push_back(in[o]);
					}
					commit(tempVec, true, newFile, debug);
					newFile << "call " << in[o].value << endl;
					j = o;
				}
				if (amp)
					newFile << "lea ";
				else
					newFile << "mov ";
				switch (offset) {
					case 0:
						newFile << "rdi, ";
						break;
					case 1:
						newFile << "rsi, ";
						break;
					case 2:
						newFile << "rdx, ";
						break;
					case 3:
						newFile << "rcx, ";
						break;
					case 4:
						newFile << "r8, ";
						break;
					case 5:
						newFile << "r9, ";
						break;
					default:
						fatal("sorry, no more than six params :/; found " + to_string(j));
						break;
				}
				if (funcCall) {
					newFile << "rax" << endl;
				}
				else {
					vector<morpheme> temp;
					temp.push_back(in[j]);
					commit(temp, true, newFile, debug);
				}
				offset++;
			}
			i += j;
			continue;
		}
		if (in[i].label.compare("set") == 0) {
			if (in[i + 2].label.compare("startGiveParams") == 0 || in[i + 2].label.compare("funCall") == 0) {
				vector<morpheme> temp;
				for (uint j = i + 2; j < in.size(); j++)
					temp.push_back(in[j]);
				commit(temp, false, newFile, debug);
				newFile << "mov ";
				vector<morpheme> tempier;
				tempier.push_back(in[i + 1]);
				commit(tempier, false, newFile, debug);
				newFile << ", rax" << endl;
			}
			else {
				vector<morpheme> temp;
				newFile << "mov ";
				if (in[i + 2].label.compare("param") == 0) {
					newFile << "r8, ";
					temp.push_back(in[i + 2]);
					commit(temp, true, newFile, debug);
					newFile << "mov ";
					temp[0] = in[i + 1];
					commit(temp, false, newFile, debug);
					newFile << ", r8" << endl;
				}
				else {
					temp.push_back(in[i + 1]);
					commit(temp, false, newFile, debug);
					newFile << ", " << in[i + 2].value << endl;
				}
			}
			break;
		}
		if (in[i].label.compare("literal") == 0) {
			newFile << in[i].value;
			continue;
		}
		string temp = "\n" + in[i].label + "\n" + in[i].value + "\n";
		fatal("BIG BAD ERROR:\n\tunprossesed morpheme in commital of label and value:" + temp);
	}
	if (addLineIfNeeded && needsLine) {
		newFile << endl;
	}
}

void sof(ofstream &newFile, bool debug) {
	newFile << "section .text" << endl;
	newFile << "global start" << endl;
	if (debug)
		cout << "\t> start of file" << endl;
}

void eof(string configFilePath, ofstream &newFile, bool debug) {	
	ifstream configFile(configFilePath);
	if (needsImport) {
		for (uint i = 0; i < imports.size(); i++) {
			if (debug)
				cout << "importing from " << imports[i] << endl;
			importPls(imports[i], debug, configFile, newFile);
		}
	}
	if (debug)
		cout << "end of file" << endl;
	newFile << "start:" << endl;
	newFile << "mov [origin], rbp" << endl;
 // remember origional stack frame
	newFile << "push rbp" << endl;
	newFile << "mov rbp, rsp" << endl;
 // set program stack frame
	newFile << "call _start" << endl;
 // start user program
	newFile << "exit:" << endl;
 // enters exit when returning from user program
 // will be exit and will make new _exit when parameters are introduced
	newFile << "mov rsp, rbp" << endl;
	newFile << "pop rbp" << endl;
 // unset stack frame
	newFile << "cmp [origin], rbp" << endl;
	newFile << "jne exit" << endl;
 // if stack base pointer does not match origin, jump to _exit and unset stack again
	newFile << "mov rdi, r8" << endl;
	newFile << "mov rax, 60" << endl;
 // exit with 
	newFile << "syscall" << endl;
	newFile << "section .bss" << endl;
	newFile << "origin resq 1" << endl;
 // space to remember origional stack base position

 // string literals
 	if (stringLitCount != 0) {
		newFile << "section .data" << endl;
		for (uint i = 0; i < stringLitCount; i++) {
			newFile << "s_" << i << " db ";
			newFile << "\"" << stringLitMap[i] << "\", 0" << endl;
		}
	}
}

struct options {
    bool debug = false;
	string oldFilePath = "";
    string newFilePath = "";
    string configFilePath = "";
};

void print_usage() {
	cout << "Usage: lamb [-f <lamb source>] [-o <assembly file>] [-c <config file>] [-s]" << endl;
	cout << "\t-f <lamb source>: specifies the file to retrieve the source code from" << endl;
	cout << "\t-o <assembly file>: specifies the assembly file to write the compiled code to" << endl;
	cout << "\t-c <config file>: specifies the config file to pull from, default is /etc/lamb.conf" << endl;
	cout << "\t-d: specifies whether or not to annotate the stages of compilation" << endl;
	cout << "\t-v: prints the version number and exits with code 0" << endl;
}

struct options getopts(int argc, char** argv) {
    struct options ret;
	int opt;
	// extern char *optarg;
	while ((opt = getopt(argc, argv, "c:f:o:dv")) != -1) {
		switch (opt) {
		case 'f':
			ret.oldFilePath = optarg;
			continue;
		case 'o':
			ret.newFilePath = optarg;
			continue;
		case 'c':
			ret.configFilePath = optarg;
			continue;
		case 'd':
			ret.debug = true;
			continue;
		case 'v':
			cout << "0.1.1" << endl;
			exit(0);
		case 'h':
		default:
			print_usage();
			continue;
        }
	}
    if (ret.oldFilePath == "" ) {
		print_usage();
		fatal("");
	}
    if (ret.configFilePath == "")
        ret.configFilePath = "/etc/lamb.conf";
    return ret;
}

int main(int argc, char** argv){
    struct options opts = getopts(argc, argv);
	ifstream oldFile(opts.oldFilePath);
	ofstream newFile(opts.newFilePath);
	sof(newFile, opts.debug);
	uint lineC = 0;
	string line;
	ifcount.push(0);
	loopcount.push(0);
	while(getline(oldFile, line)){
		lineC++;
		if (opts.debug)
			cout << "\t> sending line " << lineC << " to tokener " << endl;
		vector<morpheme> tok = tokener(line, lineC, opts.debug);
		if (opts.debug) {
			for (uint i = 0; i < tok.size(); i++) {
				cout << tok[i].label << endl;
				cout << tok[i].value << endl;
			}
			cout << "\t> line " << lineC << " tokenized" << endl;
			cout << "\t> sending line " << lineC << " to lexer" << endl;
		}
		vector<morpheme> lex = lexer(tok, opts.debug);
		if (opts.debug) {
			for (uint i = 0; i < lex.size(); i++) {
				cout << lex[i].label << endl;
				cout << lex[i].value << endl;
				if (lex[i].label.compare("funDef") == 0)
					cout << "\tres: " << retMap[lex[i].value] << endl;
			}
			cout << "\t> line " << lineC << " lexed" << endl;
			cout << "\t> sending line " << lineC << " to commital" << endl;
		}
		commit(lex, true, newFile, opts.debug);
	}
	eof(opts.configFilePath, newFile, opts.debug);
}
