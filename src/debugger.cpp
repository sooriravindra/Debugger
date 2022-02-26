#include "debugger.h"

#include <sys/ptrace.h>
#include <sys/wait.h>

#include <iostream>
#include <sstream>

#include "breakpoint.h"
#include "linenoise.h"

const auto kHexBase = 16;

void Debugger::StartRepl() {
  char* line_read = nullptr;
  while ((line_read = linenoise("(db) > ")) != nullptr) {
    linenoiseHistoryAdd(line_read);
    ProcessCommand(line_read);
    linenoiseFree(line_read);
  }
}

int Debugger::Wait(int* status) const { return waitpid(pid_, status, 0); }

void Debugger::Continue() {
  ptrace(PTRACE_CONT, pid_, nullptr, nullptr);
  Wait();
}

void Debugger::SetBreakpointAtAddress(std::uintptr_t addr) {
  Breakpoint b(pid_, addr);
  b.Enable();
  breakpoints_.insert(b);
}

bool Debugger::MatchCmd(const std::string& input, const std::string& cmd) {
  // If input is longer, there are garbage character at the end
  if (input.size() > cmd.size()) {
    return false;
  }
  // Now match all of input to cmd till the length of input
  return std::equal(input.begin(), input.end(), cmd.begin());
}

std::vector<std::string> Debugger::SplitCommand(const std::string& cmd) {
  std::stringstream ss(cmd);
  std::string word;
  std::vector<std::string> command_words;
  while (ss >> word) {
    command_words.push_back(word);
  }
  return command_words;
}

void Debugger::ProcessCommand(const std::string& cmd_line) {
  auto cmd_argv = SplitCommand(cmd_line);

  if (cmd_argv.empty()) {
    return;
  }

  if (MatchCmd(cmd_argv[0], "continue")) {
    Continue();
  } else if (MatchCmd(cmd_argv[0], "breakpoint")) {
    if (cmd_argv.size() < 2) {
      std::cerr << "Need argument" << std::endl;
    }
    std::string addr(cmd_argv[1], 2);
    SetBreakpointAtAddress(std::stol(addr, 0, kHexBase));
  } else {
    std::cerr << "Unknown command :(" << std::endl;
  }
}
