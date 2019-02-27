
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "shelpers.hpp"

/*
 Searches for a symbol c in a string at position i in a vector, and splits the string by the symbol.
 If symbol is at the end, does not modify string.
 If symbol is at the beginning, the symbol is separated as its own string of size 1 at position i
 and everything after the symbol in the string is added to the vector as a new string at position i + 1.
 Returns true if symbol is present in vector.
 */
bool splitOnSymbol(std::vector<std::string>& words, int i, char c) {
    if (words[i].size() < 2) { // If string size is 1 or 2, can't split
        return false;
    }
    int pos;
    if ((pos = (int) words[i].find(c)) != std::string::npos) { // If string contains c
        if (pos == 0) { // If c is at the beginning of the string
            words.insert(words.begin() + i + 1, words[i].substr(1, words[i].size() - 1)); // Take everything after c as second vector entry
            words[i] = words[i].substr(0, 1); // Truncate first entry to only contain c
        } else { // Then c is in the middle or at the end
            words.insert(words.begin() + i + 1, std::string{c}); // Add c to appropriate position of vector
            std::string after = words[i].substr(pos + 1, words[i].size() - pos - 1); // Creating second half string
            if (!after.empty()) { //if symbol not at end
                words.insert(words.begin() + i + 2, after); // Add second half string (if not empty) to vector
            }
            words[i] = words[i].substr(0, pos); // Truncating first half string up until c
        }
        return true;
    } else {
        return false;
    }
}

/*
 Splits a string into tokens (words); parses by spaces and &, <, >, |.
 Returns vector of words/tokens.
 */
std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> ret;
    int pos = 0;
    int space;
    while ((space = (int) s.find(' ', pos)) != std::string::npos) { // While there is another space in the string
        std::string word = s.substr(pos, space - pos); // Split on spaces
        if (!word.empty()) {
            ret.push_back(word); // Add word to vector
        }
        pos = space + 1; // Move forward a position
    }
    std::string lastWord = s.substr(pos, s.size() - pos); // Take care of last word
    if (!lastWord.empty()) {
        ret.push_back(lastWord); // Add it to the vector
    }
    for (int i = 0; i < ret.size(); ++i) { // For every string in vector
        for (auto c : {'&', '<', '>', '|', '=', '$'}) { // For each special symbol, split
            if (splitOnSymbol(ret, i, c)) { // If string contained a special symbol and was split once
                --i; // Then also split the lower string to isolate symbol
                break;
            }
        }
    }
    return ret;
}

/*
 Overloads << operator to allow
 command object to be printed for debugging purposes.
 */
std::ostream& operator<<(std::ostream& outs, const Command& c) {
    outs << c.exec << " argv: "; // rint name of executable
    for (const auto& arg : c.argv) {
        if (arg) {
            outs << arg << ' '; // Print arguments
        }
    }
    outs << "fds: " << c.fdStdin << ' ' << c.fdStdout << ' ' << (c.background ? "background" : ""); // Print other member variables
    return outs;
}

/*
 Converts each token (word) in input vector<string> to a command. Populates member variables appropriately.
 Returns empty vector and closes file descriptors on error.
 */
std::vector<Command> getCommands(const std::vector<std::string>& tokens) {
    std::vector<Command> ret(std::count(tokens.begin(), tokens.end(), "|") + 1); // Create vector with right number of commands
    int first = 0;
    int last = (int) (std::find(tokens.begin(), tokens.end(), "|") - tokens.begin());
    bool error = false;

    for (int i = 0; i < ret.size(); ++i) { // Loop through each command in vector

        // Error handling
        if ((tokens[first] == "&") || (tokens[first] == "<") || (tokens[first] == ">") || (tokens[first] == "|")) {
            error = true; // No commands should start with these symbols
            break;
        }

        // Initialize Command object
        ret[i].exec = tokens[first]; // The first word is the executable's name
        ret[i].argv.push_back(tokens[first].c_str()); // argv[0] is also the name
        ret[i].fdStdin = 0; // default
        ret[i].fdStdout = 1; // default
        ret[i].background = false;

        for (int j = first + 1; j < last; ++j) { // Loop through all other arguments in the command (after name)

            // Set up file descriptors for I/O redirection
            if (tokens[j] == ">" || tokens[j] == "<" ) {
                std::string filePath = tokens[j + 1];
                int fd;
                if (tokens[j] == ">") {
                    fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
                    ret[i].fdStdout = fd;
                    ret[i].argv.push_back(nullptr); // adding nullptr to indicate end (don't want filename)
                }
                if (tokens[j] == "<") {
                    fd = open(filePath.c_str(), O_RDONLY, 0666);
                    ret[i].fdStdin = fd;
                }
                if (fd < 0) {
                    perror("open() failed");
                    continue; // Redisplay prompt in case of typo
                }

                // Background command
            } else if (tokens[j] == "&") {
                ret[i].background = true;

                // Environment variable declaration/assignment
            } else if (tokens[j] == "=") {
                ret[i].environment = true;
                int s = setenv(tokens[j - 1].c_str(), tokens[j + 1].c_str(), 1); // 1 indicates add OR change existing
                if (s < 0) {
                    perror("setenv()");
                    continue; // Redisplay prompt and try again
                }
            } else if (tokens[j] == "$") {
                char* envVal = getenv(tokens[j + 1].c_str());
                if (envVal == NULL) {
                    perror("getenv()");
                    continue; // Redisplay prompt and try again
                }
                ret[i].argv.push_back(envVal);
                ret[i].argv.push_back(nullptr); // adding nullptr to indicate end

                // Add argument to vector
            } else {
                ret[i].argv.push_back(tokens[j].c_str());
            }
        }

        // Handle shell built-in 'cd'
        if (ret[i].exec == "cd") {
            std::string rootDirectory = "/Users/katierobinson";
            if (ret[i].argv.size() > 1) {
                rootDirectory = ret[i].argv[1];
            }
            int r = chdir(rootDirectory.c_str());
            if (r < 0) {
                perror("chdir()");
                continue; // Redisplay prompt in case of typo
            }
        }

        // Allow environment variables to be unset
        if (ret[i].exec == "unset") {
            ret[i].environment = true;
            int u = unsetenv(ret[i].argv[1]);
            if (u < 0) {
                perror("unsetenv()");
                continue; // Redisplay prompt and try again
            }
        }

        // Create a pipe to enable multiple commands
        if (i >= 1) {
            int fds[2];
            int p = pipe(fds);
            if (p < 0) {
                perror("pipe()");
                error = true;
                exit(EXIT_FAILURE);
            }
            // Connect the ends (the output of the first to the input of the second)
            ret[i].fdStdin = fds[0];
            ret[i - 1].fdStdout = fds[1];
        }

        // Add required nullptr to end of argv
        ret[i].argv.push_back(nullptr);


        // Find the next pipe character
        first = last + 1;
        if (first < tokens.size()) {
            last = (int) (std::find(tokens.begin() + first, tokens.end(), "|") - tokens.begin());
        }
    }

    // On error, close all file decriptors
    if (error) {
        for (Command c : ret) {
            closeFileDescriptors(c);
        }
        std::cout << "Error in getCommands().\n";
        exit(EXIT_FAILURE);
    }
    return ret;
}

/*
Helper function: close Command's file descriptors.
*/
void closeFileDescriptors(Command c) {

    if (c.fdStdin != 0) {
        close(c.fdStdin);
    }
    if (c.fdStdout != 1) {
        close(c.fdStdout);
    }
}