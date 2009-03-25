/**
* ZeroShell(ZRSh)
* Copyright by Zero Cho 2009.
* 
* zrsh.cpp - main entry point of ZRSh
*
* License Type: MIT License (please read MIT-LICENSE file.)
*/
#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include "ZRCommandParser.h"
using namespace std;

extern int errno;
extern char** environ;

void trim(string& str)
{
  string::size_type pos = str.find_last_not_of(' ');
  if(pos != string::npos) {
    str.erase(pos + 1);
    pos = str.find_first_not_of(' ');
    if(pos != string::npos) str.erase(0, pos);
  }
  else str.erase(str.begin(), str.end());
}

int spawn(ZRCommandParserResult r)
{
	// Prepare argument list
	char **arg_list = (char**)malloc(sizeof(char*) * (r.arguments.size() + 1));
	char *searchPath = getenv("MYPATH");
	if (!searchPath)
		searchPath = getenv("PATH");
		
	if (!searchPath)
	{
		searchPath = (char*)malloc(sizeof(char) * 1);
		strcpy(searchPath, "");
	}
		
	string path = r.getAbsolutePath(string(searchPath));
	arg_list[0] = (char*)malloc(path.length() + 1);
	strcpy(arg_list[0], path.c_str());
	arg_list[0][path.length()] = NULL;
	int i;
	for(i = 0;i < r.arguments.size();i++)
	{
		arg_list[i+1] = (char*)malloc(r.arguments[i].length() + 1);
		strcpy(arg_list[i+1], r.arguments[i].c_str());
		arg_list[i+1][r.arguments[i].length()] = NULL;
	}
	arg_list[r.arguments.size() + 1] = NULL;
	
	int pid = fork();
	if (pid == 0)
	{
		execv(arg_list[0], arg_list);
		
		// Code below only runs when exec is failed.
		cout << "[ZRSh] Unable to execute command. (code: " << errno << ")" << endl;
		exit(-1);
	}
	else if (pid > 0)
	{
		if (!r.background)
			waitpid(pid, NULL, NULL);
	}
	else
	{
		cout << "[ZRSh] Sorry, fork failed." << endl;
	}
	
	for(i = 0;i < r.arguments.size();i++)
	{
		free(arg_list[i]);
	}
	free(arg_list);
}

int main()
{
	cout << "Welcome to ZeroShell(ZRSh) 1.0" << endl;
	cout << "Copyright by Zero Cho 2009. Licensed under MIT license." << endl;
	cout << "Initializing in progress..." << endl;
	vector<string> history;
	string command;
	ZRCommandParser parser;
	char *input;
	using_history();
	char hostname[1024] = {0};
	gethostname(hostname, 1023);
	string home = string(getenv("HOME"));
	cout << "Done, dropping you to shell." << endl << endl;
	while(true)
	{
		ostringstream oss;
		char *path_c = (char*)malloc(sizeof(char) * 1024);
		size_t size = 1024;
		getcwd(path_c, size);
		if (!path_c)
		{
			cout << "[ZRSh] ERROR: Unable to construct prompt. (code: " << errno << ")" << endl;
			exit(-1);
		}
		
		string path = string(path_c);
		free(path_c);
		int pos = path.find(home);
		if (pos != string::npos)
			path = "~" + path.substr(home.length(), path.length() - home.length());
		oss << getenv("USER") << "@" << hostname << " " << path << "> ";
		input = readline(oss.str().c_str());
		command = string(input, strlen(input));
		trim(command);
		free(input);
		if (command == "") continue;
		add_history(command.c_str());
		ZRCommandParserResult r = parser.parse(command);
		
		if (r.command == "quit")
			break;
		else if (r.command == "history")
		{
			for(int i=0;i<history_length;i++)
			{
				HIST_ENTRY *entry = history_get(i);
				printf("%5d  %s\n", i, entry->line);
			}
		}
		else if (r.command == "cd")
		{
			chdir(r.arguments[0].c_str());
		}
		else if (r.command == "setenv")
		{
			if (r.arguments.size() == 1)
				unsetenv(r.arguments[0].c_str());
			else if (r.arguments.size() == 2)
				setenv(r.arguments[0].c_str(), r.arguments[1].c_str(), true);
			else
			{
				cout << "setenv requires 1 or 2 arguments, you give me " << r.arguments.size() << " arguments." << endl;
				cout << "Please check \"help\" command." << endl;
			}
		}
		else if (r.command == "listenv")
		{
			char **ptr = environ;
			while(*ptr != NULL)
			{
				cout << *ptr << endl;
				ptr++;
			}
		}
		else if (r.command == "help")
		{
			cout << "ZeroShell(ZRSh) 1.0 -Help-" << endl << endl;
			printf("%10s %s\n", "setenv", "(name) (value) Set environment variables, leave value empty to unset.");
			printf("%10s %s\n", "listenv", "List all environment variables");
			printf("%10s %s\n", "help", "Display this help");
			printf("%10s %s\n", "quit", "Leave this shell");
		}
		else
		{
			spawn(r);
		}
	}
}