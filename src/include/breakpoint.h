#pragma once
#include <sys/types.h>

#include <cstdint>
#include <functional>

class Breakpoint {
 public:
  Breakpoint(pid_t pid, std::uintptr_t addr) : pid_{pid}, addr_{addr} {}
  void Enable();
  void Disable();
  bool IsEnabled() const;
  std::uintptr_t GetAddress() const;
  bool operator==(const Breakpoint &) const;

  struct HashFunction {
    size_t operator()(const Breakpoint &b) const {
      return std::hash<std::uintptr_t>()(b.addr_);
    }
  };

 private:
  bool enabled_ = false;
  std::uintptr_t addr_;
  pid_t pid_;
  uint8_t instruction_ = 0;
};
