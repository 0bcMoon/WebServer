
#!/bin/bash

# Launch LLDB, set a breakpoint, and run the program
lldb << EOF
target create webserv
b main.cpp:39
r
p http->globalPram
EOF
