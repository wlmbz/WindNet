#include <gtest/gtest.h>
#include <map>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "base/GUID.h"

using namespace std;

TEST(GuidTest, create)
{
    CGUID guid;
    EXPECT_EQ(guid.size(), 16);
    EXPECT_TRUE(guid.is_nil());
    guid = create_guid();
    EXPECT_FALSE(guid.is_nil());
}

TEST(GuidTest, from_string)
{
    const char* strGuid = "{CEBF9FA1-2A97-4980-83A1-3B4C39AC7AC8}";
    CGUID guid;
    EXPECT_TRUE(guid.from_string(strGuid));
    EXPECT_FALSE(guid.is_nil());
    CGUID guid1("{CEBF9FA1-2A97-4980-83A1-3B4C39AC7AC8}");
    EXPECT_EQ(guid, guid1);
    CGUID guid2(L"{CEBF9FA1-2A97-4980-83A1-3B4C39AC7AC8}");
    EXPECT_EQ(guid1, guid2);
}


template <typename T>
void test_map_hash()
{
    const size_t max_count = 1000;
    T   container;
    vector<CGUID>  guid_list;
    for (size_t i = 0; i < max_count; ++i)
    {
        CGUID guid = create_guid();
        EXPECT_FALSE(guid.is_nil());
        guid_list.push_back(guid);
        container[guid] = i * 10;
    }
    EXPECT_EQ(container.size(), guid_list.size());
    for (size_t i = 0; i < max_count; ++i)
    {
        const CGUID& guid = guid_list[i];
        EXPECT_TRUE(container.count(guid) > 0);
        EXPECT_EQ(container[guid], i*10);
        container.erase(guid);
        EXPECT_TRUE(container.count(guid) == 0);
        EXPECT_EQ(container.find(guid), container.end());
    }
}

template <typename T>
void test_set_hash()
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
    test_map_hash<map<CGUID, int> >();
    test_map_hash<unordered_map<CGUID, int> >();

    test_set_hash<set<CGUID> >();
    test_set_hash<unordered_set<CGUID> >();
}
