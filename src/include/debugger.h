#pragma once
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "breakpoint.h"
#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"
#include "registers.h"

class Debugger {
 public:
  explicit Debugger(const char* binary_name, pid_t pid)
      : binary_name_{binary_name}, pid_{pid} {
    auto fd = open(binary_name_, O_RDONLY);
    elf_ = elf::elf(elf::create_mmap_loader(fd));
    dwarf_ = dwarf::dwarf{dwarf::elf::create_loader(elf_)};
    load_address_ = GetLoadAddress();
  }
  void StartRepl();
  void Continue();
  void SetBreakpointAtAddress(std::uintptr_t addr);

 private:
  void HandleSigtrap(siginfo_t siginfo) const;
  int Wait(int* status = nullptr) const;
  siginfo_t GetSigInfo() const;
  void ProcessCommand(const std::string& cmd);
  static bool MatchCmd(std::vector<std::string>& input, const std::string& cmd,
                       int num_args);
  static void PrintSource(const std::string& file_name, unsigned line,
                          unsigned n_lines_context = 2 << 2);
  uint64_t GetRegister(Register::Reg r) const;
  uint64_t GetRegister(std::string s) const;
  uint64_t GetMemory(uintptr_t addr) const;
  void SetRegister(Register::Reg r, uint64_t value) const;
  void SetRegister(std::string s, uint64_t value) const;
  void SetMemory(uintptr_t addr, uint64_t value) const;
  void StepOverBreakpoint();
  void SingleStepInstruction();
  void SingleStepInstructionWithBreakpointCheck();
  void StepOut();
  void StepOver();
  void StepIn();
  void RemoveBreakpoint(std::uintptr_t addr);
  uint64_t GetLoadAddress();
  dwarf::die GetFunctionFromPC(uint64_t pc);
  dwarf::line_table::iterator GetLineEntryFromPC(uint64_t pc) const;
  static std::vector<std::string> SplitCommand(const std::string& cmd);
  uint64_t SubtractLoadAddress(uint64_t addr) const;
  pid_t pid_;
  const char* binary_name_;
  uint64_t load_address_;
  elf::elf elf_;
  dwarf::dwarf dwarf_;
  std::unordered_map<std::uintptr_t, Breakpoint> breakpoints_;
};
