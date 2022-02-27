#include "breakpoint.h"

#include <sys/ptrace.h>

#include <iostream>

const auto kByteMask = 0xff;
const auto kInt3 = 0xcc;

void Breakpoint::Enable() {
  if (enabled_) {
    std::cout << "Already enabled" << std::endl;
    return;
  };
  auto word = ptrace(PTRACE_PEEKTEXT, pid_, addr_, nullptr);
  instruction_ = word & kByteMask;
  // Set lowest byte to 0xcc.
  uint64_t new_instruction = (word & ~kByteMask) | kInt3;
  ptrace(PTRACE_POKETEXT, pid_, addr_, new_instruction);
  enabled_ = true;
}

void Breakpoint::Disable() {
  if (!enabled_) {
    std::cout << "Not enabled" << std::endl;
    return;
  };
  auto word = ptrace(PTRACE_PEEKTEXT, pid_, addr_, nullptr);
  // Set lowest byte to instruction_
  uint64_t original_word = (word & ~kByteMask) | instruction_;
  ptrace(PTRACE_POKETEXT, pid_, addr_, original_word);
  enabled_ = false;
}

bool Breakpoint::IsEnabled() const { return enabled_; }

std::uintptr_t Breakpoint::GetAddress() const { return addr_; }
