#pragma once
#include <unistd.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "breakpoint.h"
#include "registers.h"

class Debugger {
 public:
  explicit Debugger(pid_t pid) : pid_{pid} {}
  void StartRepl();
  void Continue();
  void SetBreakpointAtAddress(std::uintptr_t addr);

 private:
  int Wait(int *status = nullptr) const;
  void ProcessCommand(const std::string &cmd);
  static bool MatchCmd(std::vector<std::string> &input, const std::string &cmd,
                       int num_args);
  uint64_t GetRegister(Register::Reg r) const;
  uint64_t GetRegister(std::string s) const;
  void SetRegister(Register::Reg r, uint64_t value) const;
  void SetRegister(std::string s, uint64_t value) const;
  void StepOverBreakpoint();
  static std::vector<std::string> SplitCommand(const std::string &cmd);
  pid_t pid_;
  std::unordered_map<std::uintptr_t, Breakpoint> breakpoints_;
};
