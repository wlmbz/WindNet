#include <gtest/gtest.h>
#include <map>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "base/GUID.h"

using namespace std;


template <typename T>
void test_container_hash()
{
    const size_t max_count = 1000;
    T   container;
    vector<CGUID>  guid_list;
    for (size_t i = 0; i < max_count; ++i)
    {
        CGUID guid = create_guid();
        EXPECT_FALSE(guid.is_nil());
        guid_list.push_back(guid);
    }
    EXPECT_EQ(container.size(), guid_list.size());
    for (size_t i = 0; i < max_count; ++i)
    {
        const CGUID& guid = guid_list[i];
        EXPECT_TRUE(container.count(guid) > 0);
        container.erase(guid);
        EXPECT_TRUE(container.count(guid) == 0);
        EXPECT_EQ(container.find(guid), container.end());
    }
}

TEST(GuidTest, hash)
{
    test_container_hash<map<CGUID, int> >();
    //test_container_hash<multimap<CGUID, int> >();
    test_container_hash<unordered_map<CGUID, int> >();
}
