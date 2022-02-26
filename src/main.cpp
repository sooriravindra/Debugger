#include <sys/ptrace.h>
#include <unistd.h>

#include <iostream>

#include "debugger.h"
using std::cerr;
using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Please provide program to debug" << endl;
    return 1;
  }

  cout << "***** DB v0.01 *****" << endl;

  auto pid = fork();
  // and then there were two...

  if (pid == 0) {
    // Let the child be examinable and controllable by the parent
    auto ret = ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
    if (ret != 0) {
      cerr << "Ptrace failed :(" << endl;
      return 2;
    }
    execv(argv[1], &argv[1]);
    cerr << "Failed to exec program" << endl;
    return 3;
  }

  // This be parent.
  // Instantiate debugger and observe & control child
  Debugger my_debugger(pid);
  my_debugger.StartRepl();

}
