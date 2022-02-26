#include "debugger.h"
#include "linenoise.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <iostream>

void Debugger::StartRepl() {
    Wait();
    char * line_read = nullptr;
    while(line_read = linenoise("(db) > ")) {
        linenoiseHistoryAdd(line_read);
        ProcessCommand(line_read);
        linenoiseFree(line_read);
    }
}

void Debugger::Continue() {
    ptrace(PTRACE_CONT, pid_,nullptr, nullptr);
    Wait();
}

void Debugger::Wait() {
    int status;
    waitpid(pid_, &status, 0);
}

void Debugger::ProcessCommand(const std::string & s){
  if(s.empty()) {
      return;
  }
  if(MatchCmd(s,"continue")) {
      Continue();
  } else {
      std::cerr << "Unknown command :(" << std::endl;
  }
}

bool Debugger::MatchCmd(const std::string & input, const std::string & cmd) {
    // If input is longer, there are garbage character at the end
    if (input.size() > cmd.size()) return false; 
    // Now match all of input to cmd till the length of input
    return std::equal(input.begin(), input.end(), cmd.begin());
}
