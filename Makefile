CC = g++
INCLUDES = -include /usr/include/errno.h
FLAGS = -DDEBUG

all:
	$(CC) $(FLAGS) $(INCLUDES) -o zrsh -lreadline ZRCommandParser.cpp zrsh.cpp