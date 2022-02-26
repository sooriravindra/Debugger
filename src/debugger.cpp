#include "debugger.h"

#include <sys/ptrace.h>
#include <sys/wait.h>

#include <iostream>

#include "linenoise.h"

void Debugger::StartRepl() {
  Wait();
  char* line_read = nullptr;
  while ((line_read = linenoise("(db) > ")) != nullptr) {
    linenoiseHistoryAdd(line_read);
    ProcessCommand(line_read);
    linenoiseFree(line_read);
  }
}

void Debugger::Wait() const {
  int status;
  waitpid(pid_, &status, 0);
}

void Debugger::Continue() {
  ptrace(PTRACE_CONT, pid_, nullptr, nullptr);
  Wait();
}

bool Debugger::MatchCmd(const std::string& input, const std::string& cmd) {
  // If input is longer, there are garbage character at the end
  if (input.size() > cmd.size()) {
    return false;
  }
  // Now match all of input to cmd till the length of input
  return std::equal(input.begin(), input.end(), cmd.begin());
}

void Debugger::ProcessCommand(const std::string& cmd_line) {
  if (cmd_line.empty()) {
    return;
  }

  // Get the first word
  std::string s = cmd_line.substr(0, cmd_line.find(" "));

  if (MatchCmd(s, "continue")) {
    Continue();
  } else {
    std::cerr << "Unknown command :(" << std::endl;
  }
}
