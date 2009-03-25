/**
* ZeroShell(ZRSh)
* Copyright by Zero Cho 2009.
* 
* ZRCommandParser.h - Header file of ZRCommandParser, ZRCommandParserResult (command parser)
*
* License Type: MIT License (please read MIT-LICENSE file.)
*/
#ifndef ZRCOMMNADPARSER_H
#define ZRCOMMNADPARSER_H
#include <string>
#include <vector>
using namespace std;

struct ZRCommandParserResult
{
	string command;
	vector<string> arguments;
	bool background;
	
	string getAbsolutePath(string searchPath);
};

class ZRCommandParser
{
	public:
	static ZRCommandParserResult parse(string command);
};
#endif