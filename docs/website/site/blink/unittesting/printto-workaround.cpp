#include <iostream>
#include <gtest/gtest.h>

using std::cout;
using testing::PrintToString;

#define OVERRIDE override

// MyClass.h
// ---------------------------------------------------------------

class MyClassA {

// As WebKit is compiled without RTTI, the following idiom is used to
// emulate RTTI type information.
protected:
   enum MyClassType {
     BType,
     CType
   };

   virtual MyClassType type() const = 0;

public:
    bool isB() const { return type() == BType; }
    bool isC() const { return type() == CType; }
};

class MyClassB : public MyClassA {
    virtual MyClassType type() const OVERRIDE { return BType; }
};
class MyClassC : public MyClassB {
    virtual MyClassType type() const OVERRIDE { return CType; }
};

// MyClassTestHelper.h
// ---------------------------------------------------------------

void PrintTo(const MyClassB& b, ::std::ostream* os)
{
    *os << "B@" << &b;
}

// Make C use B's PrintTo
void PrintTo(const MyClassC& c, ::std::ostream* os)
{
    PrintTo(*static_cast<const MyClassB*>(&c), os);
}

// Call the more specific subclass PrintTo method
// *WARNING*: The base class PrintTo must be defined *after* the other PrintTo
// methods otherwise it'll just call itself.
void PrintTo(const MyClassA& a, ::std::ostream* os)
{
    if (a.isB()) {
        PrintTo(*static_cast<const MyClassB*>(&a), os);
    } else if (a.isC()) {
        PrintTo(*static_cast<const MyClassC*>(&a), os);
    } else {
        //ASSERT_NOT_REACHED();
    }
}

int main() {
    MyClassB b;
    MyClassC c;

    MyClassB* b_ptr = &b;
    MyClassA* a_ptr1 = &b;
    MyClassC* c_ptr = &c;
    MyClassA* a_ptr2 = &c;

    cout << PrintToString(b) << "\n";       // B@0xYYYYYYYY
    cout << PrintToString(*b_ptr) << "\n";  // B@0xYYYYYYYY
    cout << PrintToString(*a_ptr1) << "\n"; // B@0xYYYYYYYY

    cout << PrintToString(c) << "\n";       // B@0xAAAAAAAA
    cout << PrintToString(*c_ptr) << "\n";  // B@0xAAAAAAAA
    cout << PrintToString(*a_ptr2) << "\n"; // B@0xAAAAAAAA
}
