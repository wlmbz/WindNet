#include <objbase.h>
#include <stdlib.h>
#include <time.h>
#include <gtest/gtest.h>
#include "base/logging.h"


_INITIALIZE_EASYLOGGINGPP;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

class MyTestEnvironment : public testing::Environment
{
public:
    void  SetUp()
    {
        ::CoInitialize(NULL);
        srand((unsigned)time(NULL));
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
