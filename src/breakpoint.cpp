#include "breakpoint.h"

#include <sys/ptrace.h>

const auto kByteMask = 0xff;
const auto kInt3 = 0xcc;

void Breakpoint::Enable() {
  if (enabled_) {
    return;
  };
  auto word = ptrace(PTRACE_PEEKTEXT, pid_, addr_, nullptr);
  instruction_ = word & kByteMask;
  // Set lowest byte to 0xcc.
  uint64_t new_instruction = (instruction_ & ~kByteMask) | kInt3;
  ptrace(PTRACE_POKEDATA, pid_, addr_, new_instruction);
  enabled_ = true;
}

void Breakpoint::Disable() {
  if (!enabled_) {
    return;
  };
  auto word = ptrace(PTRACE_PEEKTEXT, pid_, addr_, nullptr);
  // Set lowest byte to instruction_
  uint64_t new_word = (word & ~kByteMask) | instruction_;
  ptrace(PTRACE_POKEDATA, pid_, addr_, new_word);
  enabled_ = false;
}

bool Breakpoint::operator==(const Breakpoint& b) const {
  return addr_ == b.GetAddress();
}

bool Breakpoint::IsEnabled() const { return enabled_; }

std::uintptr_t Breakpoint::GetAddress() const { return addr_; }
