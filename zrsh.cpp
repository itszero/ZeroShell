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
#include <pwd.h>
#include <signal.h>
using namespace std;

extern int errno;
extern char** environ;
bool interactive = false;
int waitingpid = 0;
bool color_output = false;
string msg_head = "[ZRSh] ";

#ifdef DEBUG
/**
 * Output debug message, only works if DEBUG flag is enabled during compiling.
 * @param msg debug message to show.
 */
void debug(string msg)
{
	cerr << msg_head << "(" << getpid() << ")" << msg << endl;
}
#else
void debug(string msg) {}
#endif

/**
 * This method gives you string trimmed the extra whitespaces in the begin or end.
 * @param str String to be trimmed
 */
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

/**
 * Expand the "~" or "~username" part in the given string to absolute path.
 * @param str the path to be expanded.
 * @return the expanded version of input string
 */
string expandHomeDirectory(string str)
{
	// If it's already an aboslute path, pass.
	if (str[0] == '/')
		return str;
		
	// Oh? User directory...
	if (str[0] == '~')
	{
		if (str.size() == 1)
		{
			string home = string(getenv("HOME"));
			return home;
		}
		else if (str[1] == '/')
		{
			string home = string(getenv("HOME"));
			return home + str.substr(1);
		}
		else
		{
			int pos = str.find("/");
			string username = str.substr(1, pos - 1);
			struct passwd *record = getpwnam(username.c_str());
			string path;
			if (record && record->pw_dir)
				path = string(record->pw_dir) + str.substr(pos);
			else
				path = str;
			return path;
		}
	}
	
	return str;
}

/**
 * Execute the command
 * @param r A ZRCommandParserResultPart object to be executed.
 */
void execute(ZRCommandParserResultPart r)
{
	char **arg_list = (char**)malloc(sizeof(char*) * (r.arguments.size() + 1));
	
	char *searchPath = getenv("MYPATH");
	// MYPATH is existed, but we need to change the delimiter to ordinary ':'
	if (searchPath)
	{
		int len = strlen(searchPath);
		for(int i=0;i<len;i++)
			if (searchPath[i] == '$')
				searchPath[i] = ':';
	}
	
	// MYPATH is not found, use PATH instead.
	if (!searchPath)
	{
		cerr << msg_head << "MYPATH is not found, fallback to PATH" << endl;
		searchPath = getenv("PATH");
	}
	
	// Nothing I can use :( Let's just find it in current directory.
	if (!searchPath)
	{
		searchPath = (char*)malloc(sizeof(char) * 1);
		strcpy(searchPath, "");
	}

	string path = r.getAbsolutePath(string(searchPath));
	if (!searchPath)
		free(searchPath);
	
	// Prepare arguments list
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
	arg_list[r.arguments.size() + 1] = NULL; // Don't forget the tailing NULL.
	
	// Handles redirection
	FILE* fp;
	int fd;
	
	if (r.redirIn != "")
	{
		fp = fopen(r.redirIn.c_str(), "r");
		fd = fileno(fp);
		if (dup2(fd, STDIN_FILENO) == -1)
			cerr << msg_head << "Unable to redirect STDIN from " << r.redirIn << endl;
	}

	if (r.redirOut != "")
	{
		fp = fopen(r.redirOut.c_str(), "w");
		fd = fileno(fp);
		if (dup2(fd, STDOUT_FILENO) == -1)
			cerr << msg_head << "Unable to redirect STDOUT to " << r.redirOut << endl;
	}

	if (r.redirAppend != "")
	{
		fp = fopen(r.redirAppend.c_str(), "a");
		fd = fileno(fp);
		if (dup2(fd, STDOUT_FILENO) == -1)
			cerr << msg_head << "Unable to redirect and append STDOUT to " << r.redirAppend << endl;
	}
	
	execv(arg_list[0], arg_list);

	// Code below only runs when exec is failed.
	cout << msg_head << "Unable to execute command. (code: " << errno << ")" << endl;
	exit(-1);

	for(i = 0;i < r.arguments.size();i++)
		free(arg_list[i]);
	free(arg_list);
	
	return;
}

/**
 * Spawn a new process, and run a single command
 * @param r A ZRCommandParserResultPart object to be executed.
 */
void spawn(ZRCommandParserResultPart r)
{
	int pid = fork();
	
	if (pid == 0)
		execute(r);
	else if (pid > 0)
	{
		if (!r.background)
		{
			waitingpid = pid;
			waitpid(pid, NULL, NULL);
		}
	}
	else
		cout << msg_head << "Sorry, fork failed!" << endl;
	
	return;
}


/**
 * Recursively spawn new processes to create specified pipe chain
 * @param begin the begin iterator for the ZRCommandParserResultParts
 * @param end the end iteartor for the ZRCommandParserResultParts
 */
int spawn_for_pipe(vector<ZRCommandParserResultPart>::iterator begin, vector<ZRCommandParserResultPart>::iterator end)
{
	int fds[2];
	if (pipe(fds) == -1)
		cout << msg_head << "Unable to create pipe (code: " << errno << ")" << endl;
	
	vector<ZRCommandParserResultPart>::iterator nxtItr = begin + 1;
	
	int pid = fork();
	if (pid == 0)
	{
		if (begin->pipe_with_next)
		{
			if (dup2(fds[1], STDOUT_FILENO) == -1)
				cerr << msg_head << "dup2 failed. (STDOUT)" << endl;
		}

		if (close(fds[0]) == -1)
			cerr << msg_head << "close failed. (PIPE_READ)" << endl;

		if (close(fds[1]) == -1)
			cerr << msg_head << "close failed. (PIPE_WRITE)" << endl;
			
		execute(*begin);
	}
	else if (pid > 0)
	{
		if (dup2(fds[0], STDIN_FILENO) == -1)
			cerr << msg_head << "dup2 failed. (STDIN)" << endl;

		if (close(fds[0]) == -1)
			cerr << msg_head << "close failed. (PIPE_READ)" << endl;

		if (close(fds[1]) == -1)
			cerr << msg_head << "close failed. (PIPE_WRITE)" << endl;
				
		if (nxtItr == end)
		{
			if (!begin->background)
				waitpid(pid, NULL, NULL);
				
			return pid;
		}
		else
			return spawn_for_pipe(nxtItr, end);
	}
	else
		cerr << msg_head << "Sorry, fork failed!" << endl;
	
	return -1;
}

/**
 * SIGINT(Ctrl + C) Handler, kill the waiting process if not in interactive mode. Otherwise, exit ZeroShell.
 * @params sig signal number (In our case, this will always be SIGINT.)
 */
void sigint_handler(int sig)
{
	if (!interactive)
	{
		cout << endl << msg_head << "Sending SIGINT to Kill the foreground process..." << endl;
		kill(waitingpid, SIGINT);
		cout << endl;
	}
	else
	{
		cout << endl << msg_head << "Received SIGINT, bye!" << endl;
		exit(0);
	}
}

int main()
{
	// Check if the STDOUT is a terminal and support color output
	if (isatty(STDOUT_FILENO) && strcmp(getenv("TERM"), "dumb"))
		color_output = true;
	
	if (!color_output)
	{
		cout << "Welcome to ZeroShell(ZRSh) 1.0" << endl;
		cout << "Copyright by Zero Cho 2009. Licensed under MIT license." << endl;
		cout << "Initializing in progress..." << endl;
	}
	else
	{
		cout << "\033[1;37mWelcome to \033[1;31mZeroShell\033[m(\033[1;31mZRSh\033[m) 1.0\033[m" << endl;
		cout << "Copyright by Zero Cho 2009. Licensed under MIT license." << endl;
		cout << "\033[1;33mInitializing in progress...\033[m" << endl;
		msg_head = "\033[1;31m[ZRSH]\033[m ";
	}
	// Initialize the readline library and generate prompt
	vector<string> history;
	string command;
	ZRCommandParser parser;
	char *input;
	using_history();
	char hostname[1024] = {0};
	gethostname(hostname, 1023);
	string home = string(getenv("HOME"));
	
	// Install SIGINT catcher
	signal(SIGINT, sigint_handler);
	
	cout << "\033[1;32mDone, dropping you to shell.\033[m" << endl << endl;
	
	// Command input routine
	while(true)
	{
		interactive = true;
		ostringstream oss;
		char *path_c = (char*)malloc(sizeof(char) * 1024);
		size_t size = 1024;
		getcwd(path_c, size);
		if (!path_c)
		{
			cout << msg_head << "ERROR: Unable to construct prompt. (code: " << errno << ")" << endl;
			exit(-1);
		}
		
		string path = string(path_c);
		free(path_c);
		// Construct the prompt, path section
		// If it contains home directory, substitute it with the well-known "~"
		int pos = path.find(home);
		if (pos != string::npos)
			path = "~" + path.substr(home.length(), path.length() - home.length());
		if (color_output)
			oss << "[\033[33m" << getenv("USER") << "\033[m@\033[32m" << hostname << "\033[m] " << path << "> ";
		else
			oss << getenv("USER") << "@" << hostname << " " << path << "> ";
		
		// Read input command
		input = readline(oss.str().c_str());
		command = string(input, strlen(input));
		trim(command);
		free(input);
		if (command == "") continue;
		add_history(command.c_str());
		interactive = false;
		
		// Parse the input command
		ZRCommandParserResult r = parser.parse(command);
		
		// Handle it here if it's shell built-in commands
		// or handle it to the spawn function
		if (r.parts.size() == 0)
			continue;
		
		if (r.parts[0].command == "quit" || r.parts[0].command == "exit")
			break;
		else if (r.parts[0].command == "history")
		{
			for(int i=0;i<history_length;i++)
			{
				HIST_ENTRY *entry = history_get(i);
				printf("%5d  %s\n", i, entry->line);
			}
		}
		else if (r.parts[0].command == "cd")
		{
			if (r.parts[0].arguments.size() == 0)
				chdir(home.c_str());
			else
			{
				chdir(expandHomeDirectory(r.parts[0].arguments[0]).c_str());
			}
		}
		else if (r.parts[0].command == "setenv")
		{
			if (r.parts[0].arguments.size() == 1)
				unsetenv(r.parts[0].arguments[0].c_str());
			else if (r.parts[0].arguments.size() == 2)
				setenv(r.parts[0].arguments[0].c_str(), r.parts[0].arguments[1].c_str(), true);
			else
			{
				cout << msg_head << "setenv requires 1 or 2 arguments, you give me " << r.parts[0].arguments.size() << " arguments." << endl;
				cout << msg_head << "Please check \"help\" command." << endl;
			}
		}
		else if (r.parts[0].command == "listenv")
		{
			char **ptr = environ;
			while(*ptr != NULL)
			{
				cout << *ptr << endl;
				ptr++;
			}
		}
		else if (r.parts[0].command == "help")
		{
			cout << "ZeroShell(ZRSh) 1.0 -Help-" << endl << endl;
			printf("%10s %s\n", "setenv", "(name) (value) Set environment variables, leave value empty to unset.");
			printf("%10s %s\n", "listenv", "List all environment variables");
			printf("%10s %s\n", "help", "Display this help");
			printf("%10s %s\n", "quit", "Leave this shell");
			cout << endl;
		}
		else
		{
			if (r.parts.size() == 1)
				spawn(r.parts[0]);
			else
			{
				int pid = fork();
				if (pid == 0)
				{
					int lastpid = spawn_for_pipe(r.parts.begin(), r.parts.end());
					waitpid(lastpid, NULL, NULL);
					exit(0);
				}
				else
				{
					waitingpid = pid;
					waitpid(pid, NULL, NULL);
				}
			}
		}
	}
}