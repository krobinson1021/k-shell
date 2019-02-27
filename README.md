# katieSHELL

I wrote this shell program in C++ that utilizes fork, exec, open, and wait system calls to emulate a Linux terminal. My shell's prompt provides the current directory in green in order to stand out from the other shell text and provide location context. I have included additional support for background processes, environment variables, and tab completion with the GNU Readline library.

# Commands supported:
- ls
- cd
- pipes ( | )
- IO redirection (< >)
- most shell built-ins (touch, rm, mkdir, echo, cat, head, tail, etc.)

# Instructions for Running

1) Download the Makefile and the three cpp files
2) Open a terminal and navigate to the containing folder
3) Type "make all" to compile, link, and run my shell (you will need GNU Make)

# Disclaimer

The splitOnString() and tokenize() functions were provided as part of my university assignment; the rest of the code is my own.

# Credits

- GNU Readline Library: https://tiswww.case.edu/php/chet/readline/rltop.html

