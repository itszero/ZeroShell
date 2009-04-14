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

/**
 * This struct contains the detail part of command
 * Each part is a command with extra information to determine whether
 * it should runs in the background or pipe the output to next program
 */
struct ZRCommandParserResultPart
{
	string command;
	vector<string> arguments;
	bool background;
	bool pipe_with_next;
	string redirIn;
	string redirOut;
	string redirAppend;
	
	ZRCommandParserResultPart()
	{
		background = false;
		pipe_with_next = false;
		redirIn = "";
		redirOut = "";
		redirAppend = "";
	}
	
	string getAbsolutePath(string searchPath);	
};

/**
 * The result of the command parser
 * It may contains many command parts.
 */
struct ZRCommandParserResult
{
	vector<ZRCommandParserResultPart> parts;
};

/**
 * Class providing the command parsing ability
 */
class ZRCommandParser
{
	public:
	static ZRCommandParserResult parse(string command);
};
#endif