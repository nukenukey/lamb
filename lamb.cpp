#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <stack>

using namespace std;

      ////////////////////////
     // nukenukey @ github //
    // Jan 24 25          //
   ////////////////////////

 // lamb -> nasm for linux for 64 bit

string specChars[] = { ":", ";", "(", ")", " ", "\t", "\\", "!", "#" };
int lim = (int)(sizeof(specChars) / sizeof(string));
bool show = false;
ofstream newFile;
uint timestopop = 0;

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
	newFile.close();
	cerr << "\t> fatal error: " << in << endl;
	exit(1);
}

bool special(string in) {
	for (int i = 0; i < lim; i++) {
		if (in.compare(specChars[i]) == 0) {
			return true;
		}
	}
	return false;
}

vector<morpheme> tokener (string in, uint lineC) {
	vector<morpheme> ret;
	if (show) {
		cout << "tokenizing line " << lineC << endl;
		cout << "peeks and buffs:" << endl;
	}
	string buff = "";
	string peek = "";
	in += "\\";
	for (uint i = 0; i < in.length(); i++) {
		peek = in.substr(i, 1);
		if (special(peek)) {
			if (buff.length() > 0 || (peek.compare("\\") == 0 && buff.length() > 0)) {
				if (isalpha(buff.at(0)) || buff.at(0) == '#') {
					ret.push_back(moFact("id", buff));
				}
				else {
					ret.push_back(moFact("intLit", buff));
				}
				if (peek.compare("\\") == 0)
					continue;
			}
			if (peek.compare(")") == 0) {
				ret.push_back(moFact("cParen", peek));
			}
			else if (peek.compare("(") == 0) {
				ret.push_back(moFact("oParen", peek));
			}
			else if (peek.compare(":") == 0) {
				ret.push_back(moFact("colon", peek));
			}
			else if (peek.compare(";") == 0) {
				ret.push_back(moFact("semi", peek));
			}
			else if (peek.compare("!") == 0) {
				ret.push_back(moFact("ex", peek));
			}
			buff = "";
		}
		else if (isalpha(peek.at(0)) || isdigit(peek.at(0)) || peek.at(0) == '#'){
			buff += peek;
		}
	}
	return ret;
}

unordered_map<string, uint> resMap;
vector<string> resNames;
 // i dont really know how to use this yet but i still think i need it.
stack<string> scope;
 // tracks current scope
stack<uint> ifcount;
stack<uint> stackcount;
stack<uint> loopcount;
 // number to differentiate ifs and loops and stuff
uint ifVars;
unordered_map<string, bool> inlineMap;
unordered_map<string, uint> inlinePoint;

vector<morpheme> lexer(vector<morpheme> in) {
	in.shrink_to_fit();
	vector<morpheme> ret;
	
	if (in.front().label.compare("colon") == 0) {
		ret.push_back(moFact("resolve", ""));
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
		else if (in.back().label.compare("colon") == 0) {
			ret.push_back(moFact("openScope", "" + stackcount.top()));
 // arbitrary stack set, like { bool thing = false }. will be more important when variables are introduced
			stackcount.top()++;
			break;
		}
		else if (in[i].label.compare("id") == 0) {
			if ((i <= in.size() - 2) && (in[i + 1].label.compare("oParen") == 0)) {
				if (in[i].value.compare("if") == 0) {
					ret.push_back(moFact("if", scope.top() + "_if_" + to_string(ifcount.top())));
					i += 2;
					scope.push("if");
					ifcount.top()++;
					continue;
				}
				if (in[i].value.compare("import") == 0) {
					string toImport = in[i + 2].value;
					for (uint j = i + 3; in[j].label.compare("cParen") != 0; j++) {
						toImport += in[j].value;
					}
					ret.push_back(moFact("import", toImport));
					if (show)
						cout << "  import:" << endl << toImport << endl;
					break;
				}
				if (in.size() > 3 && in[in.size() - 2].label.compare("colon") == 0) {
					ret.push_back(moFact("funDef", in[i].value));
					resNames.push_back(in[i].value);
					scope.push(in[i].value);
					if (in.back().value.compare("null") != 0) {
						resMap[in[i].value] = (uint)stoi(in[i + 4].value);
						scope.push(in[i].value);
					}
					else {
						resMap[in[i].value] = 0;
					}
					break;
				}
				else {
					ret.push_back(moFact("funCall", in[i].value));
					break;
				}
			}
			else {
 // keywords
				if (in[i].value.compare("ret") == 0) {
					ret.push_back(moFact("ret", ""));
				}
				else if (in[i].value.compare("else") == 0) {
					ret.push_back(moFact("else", to_string(ifcount.top())));
				}
				else if (in[i].value.compare("go") == 0) {
					ret.push_back(moFact("goto", in[i + 1].value));
					i++;
				}
				else {
					{
					bool temp = false;
					for (uint j = 0; j < resNames.size(); j++) {
						if (resNames[j].compare(in[i].value) == 0) {
							temp = true;
							break;
						}
					}
					if (temp)
						ret.push_back(moFact("ref", in[i].value));
					else
						fatal("unrecognized reference of value: " + in[i].value);
					}
				}
				continue;
			}
		}
		else if (in[i].label.compare("semi") == 0) {
			ret.push_back(moFact("closeScope", ""));
			if (scope.top().compare("if") != 0) {
				ifcount.pop();
				loopcount.pop();
				stackcount.pop();
			}
			scope.pop();
			continue;
		}
		fatal("big bad error\nlexer could not resolve morpheme of label and type\n\t" + in[i].label + "\n\t" + in[i].value);
	}
	return ret;
}

vector<string> imports;
bool needsImport = false;

void importPls(string in) {
	vector<string> ret;
	ifstream configFile("/etc/lamb.config");
	if (!configFile.is_open()) {
		fatal("/etc/lamb.config does not exist");
	}
	string path;
	while (getline(configFile, path)) {
		if (path.substr(0, path.find(":")).compare("\"homeDirec\"") == 0) {
			path = path.substr(path.find(":"));
			path = path.substr(path.find("\"") + 1, path.length());
			if (path.back() != '/')
				path += "/";
			break;
		}
	}
	if (path.empty()) {
		fatal("/etc/lamb.config must have a line like `\"homeDirec\": \"/usr/lib/lamb\"`");
	}
	for (string t; (bool)(in.find(":") % 1); t = in.substr(0, in.find(":"))) {
		in = in.substr(in.find(":"));
		path += t + "/";
	}
	path = path.substr(0, path.length() - 1) + ".asm";
	if (show) {
		cout << "\t> importing from file " << path << endl;
	}
	ifstream importFile(path);
	if (!importFile.is_open()) {
		fatal("import file " + path + " does not exist");
	}
	while (getline(importFile, path))
		newFile << path << endl;
}

void commit(vector<morpheme> in) {
	in.shrink_to_fit();
	bool needsLine = false;
	for (uint i = 0; i < in.size(); i++) {
		if (in[i].label.compare("resolve") == 0) {
			newFile << "mov ";
			switch(resMap[in[i].value]) {
				case 2:
					newFile << "WORD";
					break;
				case 3:
				case 4:
					newFile << "DWORD";
					break;
				case 5:
				case 6:
				case 7:
				case 8:
					newFile << "QWORD";
					break;
				default:
					newFile << "BYTE";
					break;
			}
			newFile << " [r_" << scope.top() << "], ";
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("funDef") == 0) {
			newFile << "_" << in[i].value << ":" << endl;
			newFile << "push rbp" << endl;
			newFile << "mov rbp, rsp" << endl;
			if (show)
				cout << "funDef: _" << in[i].value << endl;
			continue;
		}
		if (in[i].label.compare("intLit") == 0) {
			newFile << in[i].value;
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("import") == 0) {
			imports.push_back(in[i].value);
			needsImport = true;
			if (show)
				cout << "will import from: " << in[i].value << endl;
			continue;
		}
		if (in[i].label.compare("else") == 0) {
			newFile << in[i].value << endl;
			continue;
		}
		if (in[i].label.compare("funCall") == 0) {
			newFile << "call _" << in[i].value << endl;
			continue;
		}
		if (in[i].label.compare("ref") == 0) {
			newFile << "[r_" << in[i].value << "]";
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("closeScope") == 0) {
			newFile << "mov rsp, rbp" << endl;
			newFile << "pop rbp" << endl;
			newFile << "ret" << endl;
			continue;
		}
		if (in[i].label.compare("ret") == 0) {
			newFile << "ret" << endl;
			continue;
		}
		if (in[i].label.compare("ex") == 0) {
			newFile << "mov r8, ";
			needsLine = true;
			continue;
		}
		if (in[i].label.compare("openScope") == 0) {
			newFile << "push rbp" << endl;
			newFile << "mov rbp, rsp" << endl;
			continue;
		}
		if (in[i].label.compare("go") == 0) {
			newFile << "jmp _" << in[i].value << endl;
			continue;
		}
		cerr << "BIG BAD ERROR:" << endl << "\tunprossesed morpheme in commital of label and value: " << endl << in[i].label << endl << in[i].value << endl;
	}
	if (needsLine) {
		newFile << endl;
	}
}

void sof() {
	newFile << "section .text" << endl;
	newFile << "global start" << endl;
	if (show)
		cout << "\t> start of file" << endl;
}

void eof() {	
	if (needsImport) {
		for (uint i = 0; i < imports.size(); i++) {
			if (show)
				cout << "importing from " << imports[i] << endl;
			importPls(imports[i]);
		}
	}
	if (show)
		cout << "end of file" << endl;
	newFile << "start:" << endl;
	newFile << "mov [origin], rbp" << endl;
 // remember origional stack frame
	newFile << "push rbp" << endl;
	newFile << "mov rbp, rsp" << endl;
 // set program stack frame
	newFile << "call _start" << endl;
 // start user program
	newFile << "_exit:" << endl;
 // enters exit when returning from user program
 // will be exit and will make new _exit when parameters are introduced
	newFile << "mov rsp, rbp" << endl;
	newFile << "pop rbp" << endl;
 // unset stack frame
	newFile << "cmp [origin], rbp" << endl;
	newFile << "jne _exit" << endl;
 // if stack base pointer does not match origin, jump to _exit and unset stack again
	newFile << "mov rdi, r8" << endl;
	newFile << "mov rax, 60" << endl;
 // exit with 
	newFile << "syscall" << endl;
	newFile << "section .bss" << endl;
	newFile << "origin resq 1" << endl;
 // space to remember origional stack base position
	for (uint i = 0; i < resNames.size(); i++) {
		if (resMap[resNames[i]] != 0) {
			newFile << "r_" << resNames[i] << " resb " << resMap[resNames[i]] << endl;
 		}
	}
}

int main(int argc, char** argv){
	if (argc < 3){
		cerr << "\t> error: bad usage. use like: \"lamb <assembly file> <lmb file>\"" << endl;
		exit(1);
	}
	if(argc >= 4){
		if (((string)(argv[3])).compare("show") == 0) {
			cout << "\t> show set to true" << endl;
			show = true;
		}
	}
	string path = argv[1];
	newFile.open(path);
	sof();
	ifstream oldFile(argv[2]);
	uint lineC = 0;
	string line;
	while(getline(oldFile, line)){
		lineC++;
		if (show)
			cout << "\t> sending line " << lineC << " to tokener " << endl;
		vector<morpheme> tok = tokener(line, lineC);
		if (show) {
			for (uint i = 0; i < tok.size(); i++) {
				cout << tok[i].label << endl;
				cout << tok[i].value << endl;
			}
			cout << "\t> line " << lineC << " tokenized" << endl;
			cout << "\t> sending line " << lineC << " to lexer" << endl;
		}
		vector<morpheme> lex = lexer(tok);
		if (show) {
			for (uint i = 0; i < lex.size(); i++) {
				cout << lex[i].label << endl;
				cout << lex[i].value << endl;
			}
			cout << "\t> line " << lineC << " lexed" << endl;
			cout << "\t> sending line " << lineC << " to commital" << endl;
		}
		commit(lex);
	}
	oldFile.close();
	eof();
	newFile.close();
}
