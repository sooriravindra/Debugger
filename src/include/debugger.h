#include <unistd.h>
#include <string>
class Debugger {
 public:
  Debugger(pid_t pid) : pid_{pid} {}
  void StartRepl();
  void Continue();

 private:
  void Wait();
  void ProcessCommand(const std::string & cmd);
  bool MatchCmd(const std::string &, const std::string&);
  pid_t pid_;
};
