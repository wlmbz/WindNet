#include <objbase.h>
#include <gtest/gtest.h>



class MyTestEnvironment : public testing::Environment
{
public:
    void  SetUp()
    {
        ::CoInitialize(NULL);
    }

    void  TearDown()
    {
        ::CoUninitialize();
    }
};


int main(int argc, char* argv[])
{
    testing::AddGlobalTestEnvironment(new MyTestEnvironment);
    testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
