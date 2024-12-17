#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unordered_map>
#include <sys/stat.h>

using namespace std;

   ////////////////////////
  // nukenukey @ github //
 // Dec 16th 24        //
////////////////////////


// lamb syntax -> nasm for linux for x64 computers

// TODO
// a bunch of stack bs with functions reterning and function parameters
// i like the paradigm tho
// see lexer    | easy i think / just parameters i think
// also
//  math        | easy
//  user i/o    | easy
//  file i/o    | harder
//  comparasons | harder
//  loops       | harder

string specChars[] = { ":", ";", "(", ")", " ", "\t", "-", "@", "!", "\\" };
// @ is a placeholder for pulling from ebx
// idk if I like it or not

int lim = (int)(sizeof(specChars) / sizeof(string));

bool show = false;

enum tokenType {
	_int_lit,
	_oParen,
	_cParen,
	_colon,
	_semi,
	_id,
	_at,
	_ex
};

struct token {
	string type;
	string value;
};

struct lexeme {
	string label;
	string value;
};

struct token tokenFact(string t, string v) {
	struct token tok;
	tok.type = t;
	tok.value = v;
	return tok;
}

struct lexeme lexFact(string lab, string val) {
	struct lexeme lex;
	lex.label = lab;
	lex.value = val;
	return lex;
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

void fatal(uint line, string in) {
	cerr << "\t> fatal error on line " << line << ": " << in << endl;
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

vector<token> tokener (string in, uint lineC) {
	vector<token> ret;
	if (show) {
		cout << "tokenizing line " << lineC << endl;
		cout << "peeks and buffs:" << endl;
	}
	string buff = "";
	string peek = "";
	for (uint i = 0; i < in.length(); i++) {
		if (in.at(i) == '\\') {
			fatal(lineC, "illegal char of value: " + in.at(i));
		}
	}
	in += "\\";
	for (uint i = 0; i < in.length(); i++) {
		peek = in.substr(i, 1);
		if (special(peek)) {
			if (buff.length() > 0 || (peek.compare("\\") == 0 && buff.length() > 0)) {
				if (isalpha(buff.at(0))) {
					ret.push_back(tokenFact("id", buff));
				}
				else {
					ret.push_back(tokenFact("intLit", buff));
				}
				if (peek.compare("\\") == 0)
					continue;
			}
			if (peek.compare(")") == 0) {
				ret.push_back(tokenFact("cParen", peek));
			}
			else if (peek.compare("(") == 0) {
				ret.push_back(tokenFact("oParen", peek));
			}
			else if (peek.compare(":") == 0) {
				ret.push_back(tokenFact("colon", peek));
			}
			else if (peek.compare(";") == 0) {
				ret.push_back(tokenFact("semi", peek));
			}
			buff = "";
		}
		else if (isalpha(peek.at(0)) || isdigit(peek.at(0))){
			buff += peek;
		}
	}
	return ret;
}

uint totalRes = 0;
unordered_map<string, uint> resMap;

vector<lexeme> lexer(vector<token> in) {
	uint offset = 0;
	vector<lexeme> ret;
	bool resolve = false;
	for (uint i = 0; i < in.size() - 1; i++) {
		if (resolve) {
			ret.push_back(lexFact("resolve", ""));
			resolve = false;
		}
		if (in[i].type.compare("colon") == 0) {
			resolve = true;
			if (in[i + 1].type.compare("intLit") == 0) {
				ret.push_back(lexFact("intDef", in[i + 1].value));
			}
		}
		else if (in[i].type.compare("id") == 0) {
			if (in[i + 1].type.compare("oParen") == 0) {
				if (in[i + 3].type.compare("colon") == 0) {
					ret.push_back(lexFact("funDef", in[i].value));
					if (in[i + 4].value.compare("null") != 0) {
						offset += (uint)stoi(in[i + 4].value);
						resMap[in[i].value] = offset;
						totalRes += offset;
					}
					else {
						resMap[in[i].value] = 0;
					}
				}
				else {
					ret.push_back(lexFact("funCall", in[i].value));
				}
			}
			else {
				if (in[i].value.compare("exit") == 0) {
					ret.push_back(lexFact("exit", in[i + 1].value));
				}
				ret.push_back(lexFact("funRef", in[i].value));
			}
			i = in.size();
			continue;
		}
		else if (in[i].type.compare("semi") == 0) {
// scope has no effect right now
			ret.push_back(lexFact("closeScope", ""));
			i = in.size();
			continue;
		}
	}
	if (show) {
		for (uint i = 0; i < ret.size(); i++) {
			cout << "> " << ret[i].label << endl << "\t> " << ret[i].value << endl;
		}
	}
	return ret;
}

ofstream newFile;

vector<string> importPls(string in) {
	vector<string> ret;
	struct stat sb;
	if (stat("/etc/lamb.conf", &sb) != 0) {
		cerr << "\t> error: file \'/etc/lamb.conf\' was not found" << endl;
		exit(1);
	}
	ifstream configFile("/etc/lamb.config");
	string path;
	while (getline(configFile, path)) {
		if (path.substr(0, path.find(":")).compare("\"homeDirec\"") == 0) {
			path = path.substr(path.find(":"));
			path = path.substr(path.find("\""), path.length() - 1) + "/";
			break;
		}
	}
	for (string t; (int)(in.find(":")) != -1; t = in.substr(0, in.find(":"))) {
		in = in.substr(in.find(":"));
		path += t + "/";
	}
	path = path.substr(0, path.length() - 1) + ".asm";
	if (stat(path.c_str(), &sb) != 0) {
		cerr << "\t> error: import file " << path << " was not found" << endl;
		exit(1);
	}
	else if (show) {
		cout << "\t> importing from file " << path << endl;
	}
	ifstream importFile(path);
	while (getline(importFile, path)) {
		ret.push_back(path);
	}
	return ret;
}

void compile(vector<lexeme> in) {
	for (uint i = 0; i < in.size(); i++) {
		if (in[i].label.compare("funDef") == 0) {
			newFile << in[i].value << ":" << endl;
			if (show)
				cout << "funDef: " << in[i].value << endl;
			continue;
		}
		if (in[i].label.compare("intDef") == 0) {
			newFile << "mov rdi, " << in[i].value << endl;
			if (show)
				cout << "intDef: " << in[i].value << endl;
			continue;
		}
	}
}

void sof() {
	newFile << "section .text\n";
	newFile << "global _start\n";
	newFile << "_start:\n";
	newFile << "push rbp\n";
	newFile << "mov rbp, rsp\n";
	newFile << "sub rsp, " << totalRes << "\n";
	newFile << "jmp start\n";
	if (show)
		cout << "start of file" << endl;
}

void eof() {	
	newFile << "mov rbx, rdi\n";
	newFile << "mov rsp, rbp\n";
	newFile << "mov rax, 1\n";
	newFile << "int 0x80\n";
	if (show)
		cout << "end of file" << endl;
	newFile.close();
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
		vector<token> tok = tokener(line, lineC);
		if (show) {
			for (uint i = 0; i < tok.size(); i++) {
				cout << tok[i].type << endl;
				cout << tok[i].value << endl;
			}
			cout << "\t> line " << lineC << " tokenized" << endl;
			cout << "\t> sending line " << lineC << " to lexer" << endl;
		}
		vector<lexeme> lex = lexer(tok);
		if (show) {
			for (uint i = 0; i < lex.size(); i++) {
				cout << lex[i].label << endl;
				cout << lex[i].value << endl;
			}
			cout << "\t> line " << lineC << " lexed" << endl;
			cout << "\t> sending line " << lineC << " to compliation" << endl;
		}
		compile(lex);
	}
	eof();
}
