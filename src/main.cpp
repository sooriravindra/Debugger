#include <iostream>
using std::cerr;
using std::cin;
using std::cout;
using std::string;

class Repl {
 public:
  static bool ProcessCommand(std::string& s) {
    cerr << "Unknown command : " << s << std::endl;
    return false;
  }
};

int main() {
  cout << "Namaskara jagattu!" << std::endl;
  string s;
  while (true) {
    cout << "(db) ";
    cin >> s;
    Repl::ProcessCommand(s);
  }
}
