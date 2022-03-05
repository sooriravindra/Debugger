#include "debugger.h"

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "breakpoint.h"
#include "linenoise.h"
#include "registers.h"

const auto kHexBase = 16;
const auto kRegisterCount = 27;

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

void Debugger::HandleSigtrap(siginfo_t siginfo) const {
  switch (siginfo.si_code) {
    case SI_KERNEL:
    case TRAP_BRKPT: {
      auto pc = GetRegister(Register::rip);
      pc--;  // rewind PC to the trap instruction
      SetRegister(Register::rip, pc);
      std::cout << "**Hit breakpoint at address 0x" << std::hex << pc << "**"
                << std::endl;
      auto offset_pc = SubtractLoadAddress(pc);
      auto line_entry = GetLineEntryFromPC(offset_pc);
      PrintSource(line_entry->file->path, line_entry->line);
      return;
    }
    case TRAP_TRACE:
      // Single stepping
      return;
    default:
      std::cout << "Unknown SIGTRAP code " << siginfo.si_code << std::endl;
  }
}

int Debugger::Wait(int* status) const {
  auto ret = waitpid(pid_, status, 0);
  auto siginfo = GetSigInfo();
  switch (siginfo.si_signo) {
    case SIGTRAP:
      HandleSigtrap(siginfo);
      break;
    case SIGSEGV: {
      std::array<std::string, 4> reason{"SEGV_MAPERR", "SEGV_ACCERR",
                                        "SEGV_BNDERR", "SEGV_PKUERR"};
      if (siginfo.si_code > 0 && siginfo.si_code < reason.size()) {
        std::cout << "Segmentation fault. Reason : "
                  << reason[siginfo.si_code] << std::endl;
      } else {
        std::cout << "Segmentation fault. Couldn't decipher reason! si_code : "
                  << siginfo.si_code << std::endl;
      }
      break;
    }
    default:
      std::cout << "Got signal " << strsignal(siginfo.si_signo) << std::endl;
  }
  return ret;
}

void Debugger::StepOverBreakpoint() {
  // Subtract 1 from PC because the interrupt instruction was 1 byte
  // which is what the PC would have incremented by when it executes the
  // interrupt
  auto possible_breakpoint_address = GetRegister(Register::rip);

  if (breakpoints_.count(possible_breakpoint_address) != 0) {
    auto& bp = breakpoints_[possible_breakpoint_address];
    if (!bp.IsEnabled()) {
      return;
    }
    // Set PC to the breakpoint address
    SetRegister(Register::rip, possible_breakpoint_address);
    // Undo the trap at the address
    bp.Disable();
    // Take one step and re-enable breakpoint
    ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr);
    Wait();
    bp.Enable();
  }
}

void Debugger::SingleStepInstruction() {
  ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr) ;
  Wait();
}

void Debugger::SingleStepInstructionWithBreakpointCheck() {
  if(breakpoints_.count(GetRegister(Register::rip))) {
    StepOverBreakpoint();
  }
  else {
    SingleStepInstruction();
  }
}

void Debugger::RemoveBreakpoint(std::uintptr_t addr) {
  if (breakpoints_.at(addr).IsEnabled()) {
    breakpoints_.at(addr).Disable();
  }
  breakpoints_.erase(addr);
}

void Debugger::StepOut() {
  auto frame_pointer = GetRegister(Register::rbp);
  auto return_address = GetMemory(frame_pointer + 8);

  bool should_remove_breakpoint = false;
  if (!breakpoints_.count(return_address)) {
    SetBreakpointAtAddress(return_address);
    should_remove_breakpoint = true;
  }

  Continue();

  if (should_remove_breakpoint) {
    RemoveBreakpoint(return_address);
  }
}

void Debugger::StepIn() {
  auto line = GetLineEntryFromPC(SubtractLoadAddress(GetRegister(Register::rip)))->line;
  while(GetLineEntryFromPC(SubtractLoadAddress(GetRegister(Register::rip)))->line == line) {
    SingleStepInstructionWithBreakpointCheck();
  }
  auto line_entry = GetLineEntryFromPC(SubtractLoadAddress(GetRegister(Register::rip)));
  PrintSource(line_entry->file->path, line_entry->line);
}

void Debugger::StepOver() {
  auto func = GetFunctionFromPC(SubtractLoadAddress(GetRegister(Register::rip)));
  auto func_entry = dwarf::at_low_pc(func);
  auto func_end = dwarf::at_high_pc(func);

  auto line = GetLineEntryFromPC(func_entry);
  auto start_line = GetLineEntryFromPC(SubtractLoadAddress(GetRegister(Register::rip)));

  std::vector<std::uintptr_t> to_delete{};

  while(line->address < func_end) {
    auto load_address = load_address_ + line->address;
    if (line->address != start_line->address && !breakpoints_.count(load_address)) {
      SetBreakpointAtAddress(load_address);
      to_delete.push_back(load_address);
    }
    ++line;
  }

  auto frame_pointer = GetRegister(Register::rbp);
  auto return_address = GetMemory(frame_pointer + 8);
  if (!breakpoints_.count(return_address)) {
    SetBreakpointAtAddress(return_address);
    to_delete.push_back(return_address);
  }

  Continue();

  for (auto addr : to_delete) {
    RemoveBreakpoint(addr);
  }
}

void Debugger::Continue() {
  StepOverBreakpoint();
  ptrace(PTRACE_CONT, pid_, nullptr, nullptr);
  Wait();
}

void Debugger::SetBreakpointAtAddress(std::uintptr_t addr) {
  Breakpoint b(pid_, addr);
  b.Enable();
  breakpoints_[addr] = b;
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
  } else if (MatchCmd(cmd_argv, "registers-dump", 0)) {
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
  } else if (MatchCmd(cmd_argv, "write-register", 2)) {
    SetRegister(cmd_argv[1], std::stol(cmd_argv[2], 0, kHexBase));
  } else if (MatchCmd(cmd_argv, "read-memory", 1)) {
    auto cmd_arg = cmd_argv[1];
    auto start_str = cmd_arg.find("0x") == 0 ? 2 : 0;
    std::string addr(cmd_arg, start_str);
    std::cout << std::hex << "0x" << GetMemory(std::stol(addr, 0, kHexBase))
              << std::endl;
  } else if (MatchCmd(cmd_argv, "write-memory", 2)) {
    auto cmd_arg = cmd_argv[1];
    auto start_str = cmd_arg.find("0x") == 0 ? 2 : 0;
    std::string addr(cmd_arg, start_str);

    cmd_arg = cmd_argv[2];
    start_str = cmd_arg.find("0x") == 0 ? 2 : 0;
    std::string value(cmd_arg, start_str);
    SetMemory(std::stol(addr, 0, kHexBase), std::stol(value, 0, kHexBase));
  } else if (MatchCmd(cmd_argv, "step", 0)) {
    StepIn();
  }else if(MatchCmd(cmd_argv, "stepi" , 0)) {
    SingleStepInstructionWithBreakpointCheck();
    auto offset_pc = SubtractLoadAddress(GetRegister(Register::rip));
    auto line_entry = GetLineEntryFromPC(offset_pc);
    PrintSource(line_entry->file->path, line_entry->line);
  } else if (MatchCmd(cmd_argv, "next", 0)) {
    StepOver();
  } else if (MatchCmd(cmd_argv, "finish", 0)) {
    StepOut();
  } else{
    std::cerr << "Please check the command" << std::endl;
  }
}

uint64_t Debugger::GetMemory(uintptr_t addr) const {
  return ptrace(PTRACE_PEEKDATA, pid_, addr, nullptr);
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
  user_regs_struct regs;
  ptrace(PTRACE_GETREGS, pid_, nullptr, &regs);
  return *(reinterpret_cast<uint64_t*>(&regs) + static_cast<size_t>(r));
}

void Debugger::SetMemory(uintptr_t addr, uint64_t value) const {
  ptrace(PTRACE_POKEDATA, pid_, addr, value);
}

void Debugger::SetRegister(std::string s, uint64_t value) const {
  for (const auto& [k, v] : Register::register_lookup) {
    if (v == s) {
      return SetRegister(k, value);
    }
  }
  throw std::runtime_error("Incorrect register name");
}

void Debugger::SetRegister(Register::Reg r, uint64_t value) const {
  if (r < 0 || r > kRegisterCount) {
    throw std::runtime_error("Attempted to set a bad register");
  }
  user_regs_struct regs;
  ptrace(PTRACE_GETREGS, pid_, nullptr, &regs);
  *(reinterpret_cast<uint64_t*>(&regs) + static_cast<size_t>(r)) = value;
  ptrace(PTRACE_SETREGS, pid_, nullptr, &regs);
}

dwarf::die Debugger::GetFunctionFromPC(uint64_t pc) {
  for (const auto& compilation_unit : dwarf_.compilation_units()) {
    if (dwarf::die_pc_range(compilation_unit.root()).contains(pc)) {
      for (const auto& die : compilation_unit.root()) {
        if (die.tag == dwarf::DW_TAG::subprogram) {
          if (dwarf::die_pc_range(die).contains(pc)) {
            return die;
          }
        }
      }
    }
  }
  throw std::out_of_range{"Cannot find function"};
}

dwarf::line_table::iterator Debugger::GetLineEntryFromPC(uint64_t pc) const {
  for (const auto& compilation_unit : dwarf_.compilation_units()) {
    if (dwarf::die_pc_range(compilation_unit.root()).contains(pc)) {
      const auto& line_table = compilation_unit.get_line_table();
      auto it = line_table.find_address(pc);
      if (it == line_table.end()) {
        throw std::out_of_range{"Cannot find line entry!"};
      }
      return it;
    }
  }
  throw std::out_of_range{"Cannot find line entry"};
}

uint64_t Debugger::GetLoadAddress() {
  // If this is a dynamic library (e.g. PIE)
  if (elf_.get_hdr().type == elf::et::dyn) {
    // The load address is found in /proc/pid/maps
    std::ifstream maps("/proc/" + std::to_string(pid_) + "/maps");
    std::string addr;
    std::getline(maps, addr, '-');
    return std::stol(addr, 0, kHexBase);
  }
  return 0;
}

siginfo_t Debugger::GetSigInfo() const {
  siginfo_t info;
  ptrace(PTRACE_GETSIGINFO, pid_, nullptr, &info);
  return info;
}

void Debugger::PrintSource(const std::string& file_name, unsigned line,
                           unsigned n_lines_context) {
  std::ifstream file{file_name};

  // Work out a window around the desired line
  auto start_line = line <= n_lines_context ? 1 : line - n_lines_context;
  auto end_line = line + n_lines_context +
                  (line < n_lines_context ? n_lines_context - line : 0) + 1;

  char c{};
  auto current_line = 1U;
  // Skip lines up until start_line
  while (current_line != start_line && file.get(c)) {
    if (c == '\n') {
      ++current_line;
    }
  }

  // Output cursor if we're at the current line
  std::cout << (current_line == line ? "> " : "  ");

  // Write lines up until end_line
  while (current_line <= end_line && file.get(c)) {
    std::cout << c;
    if (c == '\n') {
      ++current_line;
      // Output cursor if we're at the current line
      std::cout << (current_line == line ? "> " : "  ");
    }
  }

  // Write newline and make sure that the stream is flushed properly
  std::cout << std::endl;
}

uint64_t Debugger::SubtractLoadAddress(uint64_t addr) const {
  return addr - load_address_;
}
