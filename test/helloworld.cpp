#include <iostream>

void h() {
  int a = 1;
  int b = 2;
  std::cout << "Hey World," << std::endl;
  a++;
  b++;
  a++;
  b++;
}

void g() {
  int a = 1;
  int b = 2;
  a++;
  b++;
  a++;
  b++;
  h();
}

void f() {
  int a = 1;
  int b = 2;
  a += 10;
  b += 5;
  a += 3;
  b += 2;
  g();
}

int main() {
  long a = 0xdead;
  long b = 0xf00d;
  a++;
  b++;
  a++;
  b++;
  f();
  std::cout << "Hello!" << std::endl;
}
