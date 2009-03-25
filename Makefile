CC = g++
INCLUDES = -include /usr/include/errno.h

all:
	$(CC) $(INCLUDES) -o zrsh -lreadline ZRCommandParser.cpp zrsh.cpp