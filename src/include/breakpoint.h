#pragma once
#include <sys/types.h>

#include <cstdint>
#include <functional>

class Breakpoint {
 public:
  Breakpoint() = default;
  Breakpoint(pid_t pid, std::uintptr_t addr) : pid_{pid}, addr_{addr} {}
  void Enable();
  void Disable();
  bool IsEnabled() const;
  std::uintptr_t GetAddress() const;

 private:
  bool enabled_ = false;
  std::uintptr_t addr_;
  pid_t pid_;
  uint8_t instruction_ = 0;
};
