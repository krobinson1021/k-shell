#pragma once

#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

 /* 
  * NOTE: The splitOnSymbol() and tokenize() functions were NOT coded by me; they were provided as part of the school assignment.
  The purpose of the assignment was to develop an understanding of processes and related system calls as opposed to string
  splicing.
  The rest of this is my personal code.
  */


/*
 Searches for a symbol c in a string at position i in a vector, and splits the string by the symbol.
 If symbol is at the end, does not modify string.
 If symbol is at the beginning, the symbol is separated as its own string of size 1 at position i
 and everything after the symbol in the string is added to the vector as a new string at position i + 1.
 Returns true if symbol is present in vector.
 */
bool splitOnSymbol(std::vector<std::string>& words, int i, char c);

/*
 Splits a string into tokens (words); parses by spaces and &, <, >, |.
 Returns vector of words/tokens.
 */
std::vector<std::string> tokenize(const std::string& s);

/*
Struct representing a Command object.
*/
struct Command {
	std::string exec; // Name of the executable (argv[0])
	std::vector<const char*> argv; // argv should end with a nullptr!
	int fdStdin, fdStdout;
	bool background;
	bool environment;
};

/*
 Overloads << operator to allow
 command object to be printed for debugging purposes.
 */
std::ostream& operator<<(std::ostream& outs, const Command& c);

/*
 Converts each token (word) in input vector<string> to a command. Populates member variables appropriately.
 Returns empty vector and closes file descriptors on error.
 */
std::vector<Command> getCommands(const std::vector<std::string>& tokens);

/*
Helper function: close Command's file descriptors that are not in use.
*/
void closeFileDescriptors(Command c);
