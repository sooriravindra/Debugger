#include <unistd.h>
#include <string>
class Debugger {
 public:
  Debugger(pid_t pid) : pid_{pid} {}
  void StartRepl();
  void Continue();

 private:
  void Wait();
  void ProcessCommand(std::string cmd);
  pid_t pid_;
};
