#ifdef _MSC_VER
#include "gtest/gtest.h"

#include <ext/core/COM.h>

DECLARE_COM_INTERFACE(ITest, "A740AEFD-B160-447C-8771-43E29D8310DD")
{
    virtual int getVal() = 0;
};

DECLARE_COM_INTERFACE(ITestSecond, "1F42CF13-9B8A-47DE-B75F-43CB0D215E7A")
{
};

DECLARE_COM_INTERFACE_(ITestWithBase, ITest, "16400C3E-C5DA-4C69-BA84-6F68F871ACF2")
{
};

// Inherits and implements ITest, the interface is written in COM_MAP
DECLARE_COM_BASE_CLASS(TestBaseClass)
    , public ITest
{
public:
    TestBaseClass() = default;
    TestBaseClass(int a) { val = a; }

    BEGIN_COM_MAP(TestBaseClass)
        COM_INTERFACE_ENTRY(ITest)
    END_COM_MAP()

    static const int kValue = -10;

// ITest
    virtual int getVal() override { return val; }

    int val = kValue;
};
// Inherits only ITest, the interface is registered in COM_MAP
DECLARE_COM_INTERNAL_CLASS(TestClass)
    , public ITest
{
public:
    TestClass() = default;
    TestClass(int a) { val = a; }

    BEGIN_COM_MAP(TestClass)
        COM_INTERFACE_ENTRY(ITest)
    END_COM_MAP()

    static const int kValue = 101;

// ITest
    int getVal() override { return val; }

    int val = kValue;
};
// Inherits ITestSecond and TestBaseClass, passes COM_MAP to the base class
DECLARE_COM_INTERNAL_CLASS_(TestClassSecond, TestBaseClass)
    , public ITestSecond
{
public:
    TestClassSecond() { val = kValue; }
    TestClassSecond(int a) : TestBaseClass(a) { val = a; }

    BEGIN_COM_MAP(TestClassSecond)
        COM_INTERFACE_ENTRY(ITestSecond)
        COM_INTERFACE_ENTRY_CHAIN(TestBaseClass)
    END_COM_MAP()

    static const int kValue = 201;

// ITest
    int getVal() override { return val; }
};
// Inherits TestBaseClass, but does not pass COM_MAP to it
DECLARE_COM_INTERNAL_CLASS_(TestClassSecondNoChain, TestBaseClass)
    , public ITestSecond
{
public:
    TestClassSecondNoChain() = default;

    BEGIN_COM_MAP(TestClassSecondNoChain)
        COM_INTERFACE_ENTRY(ITestSecond)
    END_COM_MAP()

    static const int kValue = 70;

// ITest
    int getVal() override { return kValue; }
};
// Inherits ITestWithBase, but ITest is not registered in COM_MAP
DECLARE_COM_INTERNAL_CLASS(TestClassInterfaceWithBase)
    , public ITestWithBase
{
public:
    TestClassInterfaceWithBase() = default;

    BEGIN_COM_MAP(TestClassInterfaceWithBase)
        COM_INTERFACE_ENTRY(ITestWithBase)
    END_COM_MAP()

    static const int kValue = 7;

// ITest
    int getVal() override { return kValue; }
};


// Checking the possibility of creating COM classes
TEST(TestCOM, TestClassCreation)
{
    // class inherited from ITest
    TestClass::Ptr pTestClass;
    // class that inherits from TestBaseClass
    TestClassSecond::Ptr pTestSecondClass;
    // class that inherits from TestBaseClass but does not pass COM_INTERFACE_ENTRY_CHAIN to it
    TestClassSecondNoChain::Ptr pTestClassSecondNoChain;
    // class that inherits from an interface with inheritance
    TestClassInterfaceWithBase::Ptr pTestClassInterfaceWithBase;

    pTestClass = TestClass::create();
    EXPECT_FALSE(!pTestClass || pTestClass->getVal() != TestClass::kValue) << "Failed to create class via ::create()";
    pTestClass = TestClass::make(10);
    EXPECT_FALSE(!pTestClass || pTestClass->getVal() != 10) << "Failed to create class via ::make()";

    pTestSecondClass = TestClassSecond::create();
    EXPECT_FALSE(!pTestSecondClass || pTestSecondClass->getVal() != TestClassSecond::kValue) << "Failed to create class with inheritance";

    pTestSecondClass = TestClassSecond::make(110);
    EXPECT_FALSE(!pTestSecondClass || pTestSecondClass->getVal() != 110) << "Failed to create class with inheritance";

    pTestClassSecondNoChain = TestClassSecondNoChain::create();
    EXPECT_FALSE(!pTestClassSecondNoChain ||
                 pTestClassSecondNoChain->getVal() != TestClassSecondNoChain::kValue) << "Failed to create class with inheritance without COM_INTERFACE_ENTRY_CHAIN";

    pTestClassInterfaceWithBase = TestClassInterfaceWithBase::create();
    EXPECT_FALSE(!pTestClassInterfaceWithBase ||
                 pTestClassInterfaceWithBase->getVal() != TestClassInterfaceWithBase::kValue) << "Failed to create class from interface with inheritance";
}

// Checking the possibility of casting classes to (un)inherited interfaces
TEST(TestCOM, TestClassToInterfaceQuery)
{
    ITestPtr pITest;
    ITestSecondPtr pITestSecond;
    ITestWithBasePtr pITestWithBase;

    pITest = TestClass::create();
    EXPECT_FALSE(!pITest || pITest->getVal() != TestClass::kValue) << "Failed to cast class to interface with inheritance";
    pITest = TestClassSecond::create();
    EXPECT_FALSE(!pITest || pITest->getVal() != TestClassSecond::kValue) << "Failed to cast class to interface with inheritance";
    pITest = TestClassSecondNoChain::create();
    EXPECT_FALSE(pITest) << "Managed to bring the class to an interface that is not in COM_MAP";
    pITest = TestClassInterfaceWithBase::create();
    EXPECT_FALSE(pITest) << "Managed to bring the class to an interface that is not in COM_MAP";

    pITestSecond = TestClass::create();
    EXPECT_FALSE(pITestSecond) << "Managed to cast the class to a non-inherited interface";
    pITestSecond = TestClassSecond::create();
    EXPECT_TRUE(pITestSecond) << "Failed to cast class to interface with inheritance";
    pITestSecond = TestClassSecondNoChain::create();
    EXPECT_TRUE(pITestSecond) << "Failed to cast class to interface with inheritance";
    pITestSecond = TestClassInterfaceWithBase::create();
    EXPECT_FALSE(pITestSecond) << "Managed to cast the class to a non-inherited interface";

    pITestWithBase = TestClass::create();
    EXPECT_FALSE(pITestWithBase) << "Managed to cast the class to a non-inherited interface";
    pITestWithBase = TestClassInterfaceWithBase::create();
    EXPECT_TRUE(pITestWithBase) << "Failed to cast class to interface with inheritance";
}

// Checking the casting and equalization of interfaces
TEST(TestCOM, TestInterfaceQuerying)
{
    ITestPtr pITest;

    CComPtr<ITest> pCComITest;
    CComQIPtr<ITest> pCComQIITest;

    {
        pITest = TestClass::create();

        // operators = to COM objects from one interface
        pCComITest = pITest;
        EXPECT_TRUE(pCComITest) << "Failed to equate interface to CComPtr";
        pCComQIITest = pITest;
        EXPECT_TRUE(pCComITest) << "Failed to equate interface to CComQIPtr";
        pITest = pCComITest;
        EXPECT_TRUE(pITest) << "Failed to equate CComPtr to interface";
        pITest = pCComQIITest;
        EXPECT_TRUE(pITest) << "Failed to equate CComQIPtr to interface";
    }

    {
        pITest = TestClass::create();

        // constructors
        CComPtr<ITest> pCComITest(pITest);
        EXPECT_TRUE(pCComITest) << "Failed to pass interface to CComPtr constructor";
        CComQIPtr<ITest> pCComQIITest(pITest);
        EXPECT_TRUE(pCComQIITest) << "Failed to pass interface to CComQIPtr constructor";
        ITestPtr pITest1(pCComITest);
        EXPECT_TRUE(pITest1) << "Failed to pass CComPtr to interface constructor";
        ITestPtr pITest2(pCComQIITest);
        EXPECT_TRUE(pITest2) << "Failed to pass CComQIPtr to interface constructor";
    }

    {
        // operators = to COM objects from different interfaces

        ITestSecondPtr pITestSecond = TestClassSecond::create();

        pITest = pITestSecond;
        EXPECT_TRUE(pITest) << "Failed to equate ITestSecondPtr to ITestPtr";
        pCComITest = pITestSecond;
        EXPECT_TRUE(pCComITest) << "Failed to equate ITestSecondPtr to CComPtr<ITest>";
        pCComQIITest = pITestSecond;
        EXPECT_TRUE(pCComQIITest) << "Failed to equate ITestSecondPtr to CComQIPtr<ITest>";
        pITestSecond = pCComITest;
        EXPECT_TRUE(pITestSecond) << "Failed to equate CComPtr <ITest> to ITestSecondPtr";
        pITestSecond = pCComQIITest;
        EXPECT_TRUE(pITestSecond) << "Failed to equate CComQIPtr<ITest> to ITestSecondPtr";
    }

    {
        // constructors for COM objects from different interfaces

        ITestSecondPtr pITestSecond = TestClassSecond::create();

        ITestPtr pITest(pITestSecond);
        EXPECT_TRUE(pITest) << "Failed to pass ITestSecondPtr to ITestPtr constructor";
        CComPtr<ITest> pCComITest(pITestSecond);
        EXPECT_TRUE(pCComITest) << "Failed to pass ITestSecondPtr to CComPtr<ITest> constructor";
        CComQIPtr<ITest> pCComQIITest(pITestSecond);
        EXPECT_TRUE(pCComQIITest) << "Failed to pass ITestSecondPtr to CComQIPtr<ITest> constructor";
        ITestSecondPtr pITestSecond1(pCComITest);
        EXPECT_TRUE(pITestSecond1) << "Failed to pass CComPtr<ITest> to ITestSecondPtr constructor";
        ITestSecondPtr pITestSecond2(pCComQIITest);
        EXPECT_TRUE(pITestSecond2) << "Failed to pass CComQIPtr<ITest> to ITestSecondPtr constructor";
    }

    {
        // type casting, just checking for assembly

        CComPtr<ITestSecond> pCComITestSecond = TestClassSecond::create();
        EXPECT_TRUE(pCComITestSecond) << "Failed to get CComPtr from ComInternal via constructor";
        // CComPtr has problems with the operator = to other interfaces, we use casting
        pCComITestSecond = (decltype(pCComITestSecond))TestClassSecond::create();
        EXPECT_TRUE(pCComITestSecond) << "Failed to get CComPtr from ComInternal via =";

        // CComQIPtr does not have a constructor for COM objects with a different interface, we use casting
        CComQIPtr<ITestSecond> pCComQIITestSecond =
            (decltype(pCComQIITestSecond))TestClassSecond::create();
        EXPECT_TRUE(pCComITestSecond) << "Failed to get CComQIPtr from ComInternal via constructor";
        pCComQIITestSecond = TestClassSecond::create();
        EXPECT_TRUE(pCComITestSecond) << "Failed to get CComQIPtr from ComInternal via =";
    }

    {
        // checking the lifetime of an object

        ITestSecondPtr pITestSecond;
        EXPECT_FALSE(pITestSecond) << "After the interface constructor - the object is not empty";

        {
            ITestPtr pITestInternal = TestClassSecond::create();
            pITestSecond = pITestInternal;
        }

        EXPECT_TRUE(pITestSecond) << "After leaving the scope of the interface - the object is destroyed";
    }
}

#endif // _MSC_VER
