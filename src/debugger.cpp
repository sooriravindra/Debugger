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
  std::cout << "Breakpoint set at address : 0x" << std::hex << addr
            << std::endl;
}

bool Debugger::MatchCmd(std::vector<std::string>& input, const std::string& cmd,
                        int num_args) {
  // If input is longer, there are garbage character at the end
  if (input[0].size() > cmd.size()) {
    return false;
  }
  // Now match all of input to cmd till the length of input
  if (!std::equal(input[0].begin(), input[0].end(), cmd.begin())) {
    return false;
  }

  // Check there are right number of arguments
  if (input.size() != num_args + 1) {
    std::cerr << cmd << " takes " << num_args << " arguments." << std::endl;
    return false;
  }

  return true;
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

  if (MatchCmd(cmd_argv, "continue", 0)) {
    Continue();
  } else if (MatchCmd(cmd_argv, "breakpoint", 1)) {
    auto cmd_arg = cmd_argv[1];
    auto start_str = cmd_arg.find("0x") == 0 ? 2 : 0;
    std::string addr(cmd_arg, start_str);
    SetBreakpointAtAddress(std::stol(addr, 0, kHexBase));
  } else {
    std::cerr << "Please check the command" << std::endl;
  }
}
