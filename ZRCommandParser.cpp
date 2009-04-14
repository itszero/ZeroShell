/**
* ZeroShell(ZRSh)
* Copyright by Zero Cho 2009.
* 
* ZRCommandParser.cpp - Implementation of command parser
*
* License Type: MIT License (please read MIT-LICENSE file.)
*/
#include "ZRCommandParser.h"
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

/**
 * Check if file is exists.
 * @param path Path to file to be checked.
 * @return Yes, if file is existed. Otherwise, No.
 */
bool zr_checkFileExists(string path)
{
	FILE *fp = NULL;
	fp = fopen(path.c_str(), "r");
	
	if (fp != NULL)
	{
		fclose(fp);
		return true;
	}
	
	return false;
}

/**
 * Find command executable file in given path.
 * @param searchPath Search file in the given path with delimiter ":"
 * @return the absolute path of the executable file
 */
string ZRCommandParserResultPart::getAbsolutePath(string searchPath)
{
	for(int i=0;i<searchPath.length();i++)
	{
		if (searchPath[i] == ':')
		{
			searchPath[i] = ' ';
		}
	}

	if (zr_checkFileExists(command))
		return command;
		
	if (zr_checkFileExists("./" + command))
		return "./" + command;
	
	istringstream iss(searchPath);
	string path;
	while (iss >> path)
	{
		if (zr_checkFileExists(path + "/" + command))
			return path + "/" + command;
	}
	
	return command;
}

/**
 * Parse the given command
 * @param command command to parse.
 * @return A ZRCommandParserResult contains the parsed information.
 */
ZRCommandParserResult ZRCommandParser::parse(string command)
{
	istringstream iss(command);
	string input;
	ZRCommandParserResult r;
	ZRCommandParserResultPart rp;
	
	int i = 0;
	bool nextIsRedirIn = false;
	bool nextIsRedirOut = false;
	bool nextIsRedirAppend = false;
	while(iss >> input)
	{
		bool createNextPart = false;
		
		if (i++ == 0)
		{
			rp.command = input;
		}
		else if (input == "&")
		{
			rp.background = true;
			createNextPart = true;
		}
		else if (input == "|")
		{
			rp.pipe_with_next = true;
			rp.background = true;
			createNextPart = true;
		}
		else if (input == "<")
		{
			nextIsRedirIn = true;
		}
		else if (input == ">")
		{
			nextIsRedirOut = true;
		}
		else if (input == ">>")
		{
			nextIsRedirAppend = true;
		}
		else
		{
			if (nextIsRedirIn)
			{
				rp.redirIn = input;
				nextIsRedirIn = false;
			}
			else if (nextIsRedirOut)
			{
				rp.redirOut = input;
				nextIsRedirOut = false;				
			}
			else if (nextIsRedirAppend)
			{
				rp.redirAppend = input;
				nextIsRedirAppend = false;				
			}
			else
				rp.arguments.push_back(input);
		}
		
		if (createNextPart)
		{
			r.parts.push_back(rp);
			rp = ZRCommandParserResultPart();
			i = 0;
			continue;
		}
	}
	
	if (rp.command != "")
		r.parts.push_back(rp);
	
	return r;
}