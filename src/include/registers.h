#pragma once
#include <unordered_map>
namespace Register {
// order of enum dictated by sys/user.h
enum Reg {
  r15 = 0,
  r14,
  r13,
  r12,
  rbp,
  rbx,
  r11,
  r10,
  r9,
  r8,
  rax,
  rcx,
  rdx,
  rsi,
  rdi,
  orig_rax,
  rip,
  cs,
  eflags,
  rsp,
  ss,
  fs_base,
  gs_base,
  ds,
  es,
  fs,
  gs
};

}  // namespace Register
