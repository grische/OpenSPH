#include "objects/containers/List.h"
#include "catch.hpp"
#include "utils/RecordType.h"
#include "utils/Utils.h"

using namespace Sph;

TEST_CASE("Empty list", "[list]") {
    List<RecordType> list;
    REQUIRE(list.empty());
    REQUIRE(list.size() == 0);
    REQUIRE(list.begin() == list.end());

    REQUIRE_SPH_ASSERT(list.front());
    REQUIRE_SPH_ASSERT(list.back());
}

TEST_CASE("List push one", "[list]") {
    {
        RecordType::resetStats();
        List<RecordType> list;
        list.pushBack(1);
        REQUIRE_FALSE(list.empty());
        REQUIRE(list.size() == 1);
        REQUIRE(list.front().value == 1);
        REQUIRE(&list.front() == &list.back());

        REQUIRE(RecordType::constructedNum == 1);
        REQUIRE(RecordType::destructedNum == 0);
    }
    // make sure the element gets deleted
    REQUIRE(RecordType::constructedNum == 1);
    REQUIRE(RecordType::destructedNum == 1);
}

TEST_CASE("List push elements", "[list]") {
    {
        RecordType::resetStats();
        List<RecordType> list;
        list.pushBack(2);
        list.pushBack(3);
        list.pushFront(1);
        list.pushBack(4);
        list.pushFront(0);

        REQUIRE_FALSE(list.empty());
        REQUIRE(list.size() == 5);
        REQUIRE(list.front().value == 0);
        REQUIRE(list.back().value == 4);
        REQUIRE(RecordType::constructedNum == 5);
        REQUIRE(RecordType::destructedNum == 0);
    }
    REQUIRE(RecordType::constructedNum == 5);
    REQUIRE(RecordType::destructedNum == 5);
}

TEST_CASE("List construct initializer_list", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    REQUIRE(list.size() == 4);
    REQUIRE(list.front() == 1);
    REQUIRE(list.back() == 4);
}

TEST_CASE("List move construct", "[list]") {
    {
        List<RecordType> list1({ 3, 4, 5 });
        RecordType::resetStats();
        List<RecordType> list2(std::move(list1));
        REQUIRE(RecordType::constructedNum == 0); // shouldn't create more elements
        REQUIRE(list2.size() == 3);
        REQUIRE(list2.front().value == 3);
        REQUIRE(list2.back().value == 5);
        REQUIRE(list1.empty());
        REQUIRE(list1.size() == 0);
    }
    REQUIRE(RecordType::destructedNum == 3);
}

TEST_CASE("List move operator", "[list]") {
    List<RecordType> list1({ 8, 9, 10, 11 });
    {
        List<RecordType> list2({ 3, 4, 5 });
        RecordType::resetStats();
        list1 = std::move(list2);
        REQUIRE(RecordType::constructedNum == 0); // shouldn't create more elements
        REQUIRE(RecordType::destructedNum == 0);  // shouldn't delete elements either
    }
    REQUIRE(RecordType::destructedNum == 4); // previous content of list1 should be deleted by now
    REQUIRE(list1.size() == 3);
    REQUIRE(list1.front().value == 3);
    REQUIRE(list1.back().value == 5);
}

TEST_CASE("List forward iteration", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    ListIterator<RecordType> iter = list.begin();
    REQUIRE(iter->value == 1);
    ++iter;
    REQUIRE(iter->value == 2);
    ++iter;
    REQUIRE(iter->value == 3);
    ++iter;
    REQUIRE(iter->value == 4);
    ++iter;
    REQUIRE(!iter);
    REQUIRE_FALSE(iter);
    REQUIRE_SPH_ASSERT(*iter);
}

TEST_CASE("List backward iteration", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    // move to last; we cant use list.end, that points to one-past-last
    ListIterator<RecordType> iter = ++++++list.begin();
    REQUIRE(iter->value == 4);
    --iter;
    REQUIRE(iter->value == 3);
    --iter;
    REQUIRE(iter->value == 2);
    --iter;
    REQUIRE(iter->value == 1);
    --iter;
    REQUIRE(!iter);
    REQUIRE_FALSE(iter);
    REQUIRE_SPH_ASSERT(*iter);
}

TEST_CASE("List range-based for empty", "[list]") {
    List<RecordType> list;
    Size counter = 0;
    for (RecordType& e : list) {
        MARK_USED(e);
        ++counter;
    }
    for (const RecordType& e : asConst(list)) {
        MARK_USED(e);
        ++counter;
    }
    REQUIRE(counter == 0);
}

TEST_CASE("List range-based for elements", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    int idx = 1;
    for (RecordType& e : list) {
        REQUIRE(e.value == idx);
        ++idx;
    }
    REQUIRE(idx == 5);
    // const overload
    for (const RecordType& e : asConst(list)) {
        REQUIRE(e.value == idx - 4);
        ++idx;
    }
    REQUIRE(idx == 9);
}

TEST_CASE("List insert", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    ListIterator<RecordType> iter = list.begin();
    list.insert(iter, 9);
    REQUIRE(list.size() == 5);
    REQUIRE(iter->value == 1);
    ++iter;
    REQUIRE(iter->value == 9);
    ++iter;
    REQUIRE(iter->value == 2);
    list.insert(iter, 11);
    REQUIRE(iter->value == 2);
    ++iter;
    REQUIRE(iter->value == 11);
    ++iter;
    REQUIRE(iter->value == 3);
    ++iter;
    list.insert(iter, 99);
    REQUIRE(list.back().value == 99);

    REQUIRE_SPH_ASSERT(list.insert(list.end(), 16));
}

TEST_CASE("List erase", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    RecordType::resetStats();
    ListIterator<RecordType> iter = ++list.begin();
    list.erase(list.begin());
    REQUIRE(RecordType::destructedNum == 1);
    REQUIRE(list.size() == 3);
    REQUIRE(list.front().value == 2);
    REQUIRE(iter->value == 2); // not invalidated
    REQUIRE_FALSE(iter--);

    ++iter;
    list.erase(iter);
    REQUIRE(list.size() == 2);
    REQUIRE(list.front().value == 2);
    REQUIRE(list.back().value == 4);

    list.erase(list.begin());
    list.erase(list.begin());
    REQUIRE(list.empty());
    REQUIRE(list.size() == 0);

    // add after the whole list has been erased, to make sure it is in consistent state
    list.pushBack(5);
    REQUIRE(list.size() == 1);
    REQUIRE(list.front().value == 5);
}

TEST_CASE("List erase and advance", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    ListIterator<RecordType> iter = list.begin();
    iter = list.erase(iter);
    REQUIRE(list.size() == 3);
    REQUIRE(list.front().value == 2);
    REQUIRE(iter->value == 2);

    iter = list.erase(iter);
    iter = list.erase(iter);
    REQUIRE(list.size() == 1);
    REQUIRE(iter->value == 4);

    iter = list.erase(iter);
    REQUIRE(list.empty());
    REQUIRE_FALSE(iter);
}

TEST_CASE("List erase loop", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    Size counter = 0;
    for (auto iter = list.begin(); iter != list.end();) {
        ++counter;
        iter = list.erase(iter);
    }
    REQUIRE(list.empty());
    REQUIRE(counter == 4);
}

TEST_CASE("List clone", "[list]") {
    List<RecordType> list1({ 1, 2, 3, 4 });
    List<RecordType> list2 = list1.clone();
    REQUIRE(list2.size() == 4);
    REQUIRE(list2.front().value == 1);
    REQUIRE(list2.back().value == 4);

    list2.back().value = 8;
    REQUIRE(list1.back().value == 4); // deep copy, lists are not referencing same elements
}

TEST_CASE("List clear", "[list]") {
    List<RecordType> list({ 1, 2, 3, 4 });
    list.clear();
    REQUIRE(list.size() == 0);
    REQUIRE(list.empty());
    REQUIRE(list.begin() == list.end());
    REQUIRE_SPH_ASSERT(list.front());
    REQUIRE_SPH_ASSERT(list.back());

    list.pushBack(5);
    REQUIRE(list.size() == 1);
    REQUIRE(list.front().value == 5);
}

TEST_CASE("List of references", "[list]") {
    List<RecordType&> list;
    RecordType r1(3), r2(5);
    list.pushBack(r1);
    list.pushBack(r2);
    r1.value = 1;
    r2.value = 2;
    REQUIRE(list.front().value == 1);
    REQUIRE(list.back().value == 2);
}
