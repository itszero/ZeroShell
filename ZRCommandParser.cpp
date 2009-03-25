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

string ZRCommandParserResult::getAbsolutePath(string searchPath)
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


ZRCommandParserResult ZRCommandParser::parse(string command)
{
	istringstream iss(command);
	string input;
	ZRCommandParserResult r;
	int i = 0;
	while(iss >> input)
	{
		if (i++ == 0)
		{
			r.command = input;
		}
		else if (input == "&")
		{
			r.background = true;
			break;
		}
		else
		{
			r.arguments.push_back(input);
		}
	}
	
	return r;
}