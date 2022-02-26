#include "debugger.h"

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <iostream>
#include <sstream>

#include "breakpoint.h"
#include "linenoise.h"
#include "registers.h"

const auto kHexBase = 16;

namespace Register {
const std::unordered_map<Reg, std::string> register_lookup = {
    {r15, "r15"},
    {r14, "r14"},
    {r13, "r13"},
    {r12, "r12"},
    {rbp, "rbp"},
    {rbx, "rbx"},
    {r11, "r11"},
    {r10, "r10"},
    {r9, "r9"},
    {r8, "r8"},
    {rax, "rax"},
    {rcx, "rcx"},
    {rdx, "rdx"},
    {rsi, "rsi"},
    {rdi, "rdi"},
    {orig_rax, "orig_rax"},
    {rip, "rip"},
    {cs, "cs"},
    {eflags, "eflags"},
    {rsp, "rsp"},
    {ss, "ss"},
    {fs_base, "fs_base"},
    {gs_base, "gs_base"},
    {ds, "ds"},
    {es, "es"},
    {fs, "fs"},
    {gs, "gs"}};
}  // namespace Register

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
  } else if (MatchCmd(cmd_argv, "registers", 0)) {
    for (const auto& [k, v] : Register::register_lookup) {
      std::cout << std::hex << v << "\t:\t0x" << GetRegister(k) << std::endl;
    }
  } else if (MatchCmd(cmd_argv, "read-register", 1)) {
    try {
      auto value = GetRegister(cmd_argv[1]);
      std::cout << std::hex << "0x" << value << std::endl;
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
  } else if (MatchCmd(cmd_argv, "register-write", 2)) {
  } else {
    std::cerr << "Please check the command" << std::endl;
  }
}

uint64_t Debugger::GetRegister(std::string s) const {
  for (const auto& [k, v] : Register::register_lookup) {
    if (v == s) {
      return GetRegister(k);
    }
  }
  throw std::runtime_error("Incorrect register name");
}

uint64_t Debugger::GetRegister(Register::Reg r) const {
  user_regs_struct reg_struct;
  ptrace(PTRACE_GETREGS, pid_, nullptr, &reg_struct);
  return *(reinterpret_cast<uint64_t *>(&reg_struct) + static_cast<size_t>(r));
}
