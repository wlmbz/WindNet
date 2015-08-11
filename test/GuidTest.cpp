#include <map>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <gtest/gtest.h>
#include "common/GUID.h"


TEST(GuidTest, create)
{
    CGUID guid;
    EXPECT_EQ(guid.size(), 16);
    EXPECT_TRUE(guid.isNil()); 
    guid = createGuid();
    EXPECT_FALSE(guid.isNil());
    auto s = guid.toString();
    EXPECT_EQ(s.size(), 36);
}

TEST(GuidTest, fromString)
{
    const char* strGuid = "cebf9fa1-2a97-4980-83A1-3b4c39ac7ac8";
    CGUID guid;
    EXPECT_TRUE(guid.fromString(strGuid));
    EXPECT_FALSE(guid.isNil());
    const char* invalid_guid_list[] =
    {
        "{CEBF9FA1-2A97-4980-83A1-3B4C39AC7AC8}",
        "cebf9fa12a97498083A13b4c39ac7ac8----",
        "CEBF9-FA12A97-498083-A13B4-C39AC7AC8",
        "pEBF9-FA1-2A97498083A13B4-C39-AC7AC9",
        "CEBF9-FA12A-97498083A13B4--C39AC7ACy",
        "cebf9fa1-2a97-4980-83A1-3b4c39ac7acw",
    };
    for (auto str : invalid_guid_list)
    {
        EXPECT_FALSE(guid.fromString(str));
    }
}


template <typename C>
void test_hash_map(C& map, int max_count)
{
    std::vector<CGUID>  keys;
    for (int i = 0; i < max_count; ++i)
    {
        CGUID guid = createGuid();
        EXPECT_FALSE(guid.isNil());
        keys.push_back(guid);
        map[guid] = i * 10;
    }
    EXPECT_EQ(map.size(), keys.size());
    for (int i = 0; i < max_count; ++i)
    {
        const CGUID& guid = keys[i];
        EXPECT_TRUE(map.count(guid) == 1);
        EXPECT_EQ(map[guid], i * 10);
        map.erase(guid);
        EXPECT_TRUE(map.count(guid) == 0);
        EXPECT_EQ(map.find(guid), map.end());
    }
}

template <typename C>
void test_hash_set(C& set, int max_count)
{
    std::vector<CGUID>  keys;
    for (size_t i = 0; i < max_count; ++i)
    {
        CGUID guid = createGuid();
        EXPECT_FALSE(guid.isNil());
        keys.push_back(guid);
        auto r = set.insert(guid);
        EXPECT_EQ(r.second, true);
    }
    EXPECT_EQ(set.size(), keys.size());
    for (size_t i = 0; i < max_count; ++i)
    {
        const CGUID& guid = keys[i];
        EXPECT_TRUE(set.count(guid) == 1);
        set.erase(guid);
        EXPECT_TRUE(set.count(guid) == 0);
        EXPECT_EQ(set.find(guid), set.end());
    }
}


TEST(GuidTest, hash)
{
    std::map<CGUID, int> map1;
    std::unordered_map<CGUID, int> map2;
    test_hash_map(map1, 1000);
    test_hash_map(map2, 1000);
    
    std::set<CGUID> set1;
    std::unordered_set<CGUID> set2;
    test_hash_set(set1, 1000);
    test_hash_set(set2, 1000);
}
