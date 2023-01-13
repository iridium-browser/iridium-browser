#include <iostream>
#include <gtest/gtest.h>

using std::cout;
using testing::PrintToString;

class A {};
class B : public A {};
class C : public B {};


void PrintTo(const A& a, ::std::ostream* os)
{
    *os << "A@" << &a;
}

void PrintTo(const B& b, ::std::ostream* os)
{
    *os << "B@" << &b;
}

int main() {
    A a;
    B b;
    C c;

    A* a_ptr1 = &a;
    B* b_ptr = &b;
    A* a_ptr2 = &b;
    C* c_ptr = &c;
    A* a_ptr3 = &c;

    cout << PrintToString(a) << "\n";       // A@0xXXXXXXXX
    cout << PrintToString(b) << "\n";       // B@0xYYYYYYYY
    cout << PrintToString(c) << "\n";       // 1-byte object <60>
    cout << PrintToString(*a_ptr1) << "\n"; // A@0xXXXXXXXX
    cout << PrintToString(*b_ptr) << "\n";  // B@0xYYYYYYYY
    cout << PrintToString(*a_ptr2) << "\n"; // A@0xYYYYYYYY
}
