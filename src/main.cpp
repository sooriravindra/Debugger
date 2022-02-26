#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

#include "debugger.h"
using std::cerr;
using std::cout;
using std::endl;

void fail(const std::string& msg) {
  cerr << "Fail: " << msg << "!!" << endl;
  exit(1);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Please provide program to debug" << endl;
    return 1;
  }

  cout << "***** DB v0.01 *****" << endl;

  auto pid = fork();
  // and then there were two...

  if (pid == 0) {
    // Disable ASLR in child so we can give addresses
    personality(ADDR_NO_RANDOMIZE);

    // Let the child be examinable and controllable by the parent
    auto ret = ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
    if (ret != 0) {
      fail("ptrace");
    }

    // Execute!!
    execv(argv[1], &argv[1]);
    fail("exec");
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      fail("Debugee terminated");
    }
    // Instantiate debugger and observe & control child
    Debugger my_debugger(pid);
    my_debugger.StartRepl();
  }
}
