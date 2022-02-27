#pragma once
#include <fcntl.h>
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
    m_dwarf_ = dwarf::dwarf{dwarf::elf::create_loader(elf_)};
  }
  void StartRepl();
  void Continue();
  void SetBreakpointAtAddress(std::uintptr_t addr);

 private:
  int Wait(int* status = nullptr) const;
  void ProcessCommand(const std::string& cmd);
  static bool MatchCmd(std::vector<std::string>& input, const std::string& cmd,
                       int num_args);
  uint64_t GetRegister(Register::Reg r) const;
  uint64_t GetRegister(std::string s) const;
  uint64_t GetMemory(uintptr_t addr) const;
  void SetRegister(Register::Reg r, uint64_t value) const;
  void SetRegister(std::string s, uint64_t value) const;
  void SetMemory(uintptr_t addr, uint64_t value) const;
  void StepOverBreakpoint();
  static std::vector<std::string> SplitCommand(const std::string& cmd);
  pid_t pid_;
  const char* binary_name_;
  elf::elf elf_;
  dwarf::dwarf m_dwarf_;
  std::unordered_map<std::uintptr_t, Breakpoint> breakpoints_;
};
