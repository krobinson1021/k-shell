
#include <cstring>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "shelpers.hpp"

using namespace std;


int main(int argc, const char *argv[]) {

	// Configure readline to auto-complete filepaths on tab
	rl_bind_key('\t', rl_complete);
	char* entry, shellPrompt[100];

	// This vector will hold any background processes being waited on
	vector<pid_t> backgroundProcesses;

	while (true) {

		// Display custom shell prompt with current directory (color green: \033[0;32m)
		snprintf(shellPrompt, sizeof(shellPrompt), "\033[0;32m%s@SHELL:~%s$ \033[0;37m", getenv("USER"), (getcwd(NULL, 1024) + 20));

		// Read in string and autocomplete file paths; utilizes GNU readline library
		entry = readline(shellPrompt);

		// Inform user if any background processes have completed
		for (int i = 0; i < backgroundProcesses.size(); i++) {
			if (waitpid(backgroundProcesses[i], NULL, WNOHANG) > 0) {
				cout << "Done\n";
				backgroundProcesses.erase(backgroundProcesses.begin() + i);
			}
		}

		// Exit shell gracefully & handle blank entries
		if ((strcmp(entry, "q") == 0) || (strcmp(entry, "quit") == 0)) {
			break;
		}
		if ((strcmp(entry, "") == 0)) { // If user hits enter with no text, redisplay prompt
			continue;
		}

		// Add entry to tab autocomplete history
		add_history(entry);

		// Parse entry and create vector of Command objects
		vector<Command> commands = getCommands(tokenize(entry));
		vector<pid_t> pids; // Vector of process IDs for waiting

		for (int i = 0; i < commands.size(); i++) {

			// If command is 'cd' or an enviroment variable assignment/unset, don't fork or execute
			if (commands[i].exec == "cd" || commands[i].environment) {
				continue;
			}

			// Fork to create a child process
			pid_t cpid = fork();
			if (!commands[i].background) {
				pids.push_back(cpid); // Push non-background processes into vector to wait()
			} else {
				backgroundProcesses.push_back(cpid); // Push background processes into special vector
			}
			if (cpid < 0) {
				perror("fork() failed");
				exit(EXIT_FAILURE); // Fatal error


			} else if (cpid == 0) { // CHILD

				// Manipulate file descriptors for any redirection or pipes
				int d1 = dup2(commands[i].fdStdin, 0);
				int d2 = dup2(commands[i].fdStdout, 1);
				if (d1 < 0 || d2 < 0) {
					perror("dup2() failed");
					exit(EXIT_FAILURE); // Fatal error

				}

				// Loop through all commands and close stdin and stdout, except for the current command
				for (Command c : commands) {
					closeFileDescriptors(c);
				}

				// Execute command
				string commandName = commands[i].exec;
				int e = execvp(commandName.c_str(), const_cast<char *const *>(commands[i].argv.data()));
				if (e < 0) {
					perror("exec() failed");
					exit(EXIT_FAILURE);
				}
			}
		}

		for (Command c : commands) {
			closeFileDescriptors(c);
		}

		// Loop through and wait for all child processes to end
		for (pid_t pid : pids) {
			int status;
			waitpid(pid, &status, 0);
			if (status < 0) {
				perror("wait() failed");
				exit(EXIT_FAILURE);
			}
		}
	}
	return 0;
}
