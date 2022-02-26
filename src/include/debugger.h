#include <unistd.h>

#include <string>
class Debugger {
 public:
  explicit Debugger(pid_t pid) : pid_{pid} {}
  void StartRepl();
  void Continue();

 private:
  void Wait() const;
  void ProcessCommand(const std::string &cmd);
  static bool MatchCmd(const std::string &, const std::string &);
  pid_t pid_;
};
