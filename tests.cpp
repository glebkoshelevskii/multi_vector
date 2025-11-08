#include <gtest/gtest.h>
#include <string>
#include "multi_vector.hpp"

// Tracked type to monitor construction/destruction
struct Tracked {
    static int ctor_count;
    static int dtor_count;
    static void reset_counts() {
        ctor_count = 0;
        dtor_count = 0;
    }

    int value;

    Tracked(int v = 0) : value(v) { ++ctor_count; }
    Tracked(const Tracked& other) : value(other.value) { ++ctor_count; }
    Tracked(Tracked&& other) noexcept : value(other.value) { ++ctor_count; }
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) noexcept = default;
    ~Tracked() { ++dtor_count; }

    bool operator==(int v) const { return value == v; }
};

int Tracked::ctor_count = 0;
int Tracked::dtor_count = 0;

using MV = multi_vector<int, double, std::string>;

TEST(MultiVector, DefaultConstructedHasNoStorage) {
    MV vec;
    EXPECT_EQ(vec.size<int>(), 0u);
    EXPECT_EQ(vec.size<double>(), 0u);
    EXPECT_EQ(vec.size<std::string>(), 0u);
    EXPECT_EQ(vec.data<int>(), nullptr);
    EXPECT_EQ(vec.data<double>(), nullptr);
    EXPECT_EQ(vec.data<std::string>(), nullptr);
    EXPECT_THROW(vec.push_back<int>(42), std::length_error);
}

TEST(MultiVector, BuildAndPushBackAndAccess) {
    MV vec = MV::builder()
        .capacity<int>(3)
        .capacity<double>(2)
        .capacity<std::string>(4)
        .build();

    EXPECT_EQ(vec.size<int>(), 0u);
    EXPECT_EQ(vec.size<double>(), 0u);
    EXPECT_EQ(vec.size<std::string>(), 0u);

    vec.push_back<int>(10);
    vec.push_back<int>(20);
    vec.push_back<double>(3.5);
    vec.push_back<std::string>("hello");
    vec.push_back<std::string>("world");

    ASSERT_EQ(vec.size<int>(), 2u);
    ASSERT_EQ(vec.size<double>(), 1u);
    ASSERT_EQ(vec.size<std::string>(), 2u);

    const int* ip = vec.data<int>();
    const double* dp = vec.data<double>();
    const std::string* sp = vec.data<std::string>();

    ASSERT_NE(ip, nullptr);
    ASSERT_NE(dp, nullptr);
    ASSERT_NE(sp, nullptr);

    EXPECT_EQ(ip[0], 10);
    EXPECT_EQ(ip[1], 20);
    EXPECT_DOUBLE_EQ(dp[0], 3.5);
    EXPECT_EQ(sp[0], "hello");
    EXPECT_EQ(sp[1], "world");
}

TEST(MultiVector, CapacityExceededThrows) {
    MV vec = MV::builder()
        .capacity<int>(1)
        .capacity<double>(0)
        .capacity<std::string>(1)
        .build();

    vec.push_back<int>(42);
    EXPECT_THROW(vec.push_back<int>(7), std::length_error); // int cap = 1

    vec.push_back<std::string>("x");
    EXPECT_THROW(vec.push_back<std::string>("y"), std::length_error); // string cap = 1

    // double capacity is 0 — any push should throw
    EXPECT_THROW(vec.push_back<double>(1.0), std::length_error);
}

TEST(MultiVector, MoveConstruct) {
    MV src = MV::builder()
        .capacity<int>(2)
        .capacity<double>(1)
        .capacity<std::string>(1)
        .build();

    src.push_back<int>(5);
    src.push_back<std::string>("m");

    MV moved(std::move(src));

    ASSERT_EQ(moved.size<int>(), 1u);
    ASSERT_EQ(moved.size<std::string>(), 1u);
    EXPECT_EQ(moved.data<int>()[0], 5);
    EXPECT_EQ(moved.data<std::string>()[0], "m");

    EXPECT_EQ(src.size<int>(), 0u);
    EXPECT_EQ(src.size<double>(), 0u);
    EXPECT_EQ(src.size<std::string>(), 0u);
}

TEST(MultiVector, IndexBasedAccess) {
    // Test type_at functionality and index-based methods
    MV vec = MV::builder()
        .capacity<0>(3)  // int
        .capacity<1>(2)  // double
        .capacity<2>(4)  // std::string
        .build();

    // Verify capacities
    EXPECT_EQ(vec.capacity<0>(), 3u);
    EXPECT_EQ(vec.capacity<1>(), 2u);
    EXPECT_EQ(vec.capacity<2>(), 4u);

    // Add elements using index-based push_back
    vec.push_back<0>(10);
    vec.push_back<0>(20);
    vec.push_back<1>(3.14);
    vec.push_back<2>("test");

    // Verify sizes
    EXPECT_EQ(vec.size<0>(), 2u);
    EXPECT_EQ(vec.size<1>(), 1u);
    EXPECT_EQ(vec.size<2>(), 1u);

    // Verify data access
    const int* ip = vec.data<0>();
    const double* dp = vec.data<1>();
    const std::string* sp = vec.data<2>();

    ASSERT_NE(ip, nullptr);
    ASSERT_NE(dp, nullptr);
    ASSERT_NE(sp, nullptr);

    EXPECT_EQ(ip[0], 10);
    EXPECT_EQ(ip[1], 20);
    EXPECT_DOUBLE_EQ(dp[0], 3.14);
    EXPECT_EQ(sp[0], "test");

    // Verify capacity enforcement
    vec.push_back<0>(30);
    EXPECT_THROW(vec.push_back<0>(40), std::length_error);
}

TEST(MultiVector, DefaultValue) {
    // Test default_value functionality with type-based access
    MV vec = MV::builder()
        .capacity<int>(3)
        .capacity<double>(2)
        .capacity<std::string>(4)
        .default_value<int>(42)
        .default_value<std::string>("default")
        .build();

    // Verify that default values were applied and sizes set
    EXPECT_EQ(vec.size<int>(), 3u);
    EXPECT_EQ(vec.size<double>(), 0u);  // No default for double
    EXPECT_EQ(vec.size<std::string>(), 4u);

    // Verify all int elements are initialized
    const int* ip = vec.data<int>();
    ASSERT_NE(ip, nullptr);
    EXPECT_EQ(ip[0], 42);
    EXPECT_EQ(ip[1], 42);
    EXPECT_EQ(ip[2], 42);

    // Verify all string elements are initialized
    const std::string* sp = vec.data<std::string>();
    ASSERT_NE(sp, nullptr);
    EXPECT_EQ(sp[0], "default");
    EXPECT_EQ(sp[1], "default");
    EXPECT_EQ(sp[2], "default");
    EXPECT_EQ(sp[3], "default");

    // Can't push more elements - capacity is full
    EXPECT_THROW(vec.push_back<int>(100), std::length_error);
    EXPECT_THROW(vec.push_back<std::string>("new"), std::length_error);

    // Can still add doubles since they weren't default-initialized
    vec.push_back<double>(1.5);
    EXPECT_EQ(vec.size<double>(), 1u);
    EXPECT_DOUBLE_EQ(vec.data<double>()[0], 1.5);
}

TEST(MultiVector, DefaultValueIndexBased) {
    // Test default_value functionality with index-based access
    MV vec = MV::builder()
        .capacity<0>(2)  // int
        .capacity<1>(3)  // double
        .capacity<2>(1)  // std::string
        .default_value<0>(999)
        .default_value<1>(2.71828)
        .build();

    // Verify sizes
    EXPECT_EQ(vec.size<0>(), 2u);
    EXPECT_EQ(vec.size<1>(), 3u);
    EXPECT_EQ(vec.size<2>(), 0u);  // No default for string

    // Verify int values
    const int* ip = vec.data<0>();
    EXPECT_EQ(ip[0], 999);
    EXPECT_EQ(ip[1], 999);

    // Verify double values
    const double* dp = vec.data<1>();
    EXPECT_DOUBLE_EQ(dp[0], 2.71828);
    EXPECT_DOUBLE_EQ(dp[1], 2.71828);
    EXPECT_DOUBLE_EQ(dp[2], 2.71828);

    // Can add string since it wasn't default-initialized
    vec.push_back<2>("test");
    EXPECT_EQ(vec.size<2>(), 1u);
    EXPECT_EQ(vec.data<2>()[0], "test");
}

TEST(MultiVector, ProperDestruction) {
    // Test that non-trivial objects are properly destroyed
    Tracked::reset_counts();

    {
        // Create vector with Tracked objects using push_back
        auto vec = multi_vector<Tracked, int>::builder()
            .capacity<Tracked>(5)
            .capacity<int>(3)
            .build();

        EXPECT_EQ(Tracked::ctor_count, 0);
        EXPECT_EQ(Tracked::dtor_count, 0);

        // Add some Tracked objects
        vec.push_back<Tracked>(Tracked(10));  // 2 ctors (temp + copy), 1 dtor (temp)
        vec.push_back<Tracked>(Tracked(20));  // 2 ctors (temp + copy), 1 dtor (temp)
        vec.push_back<Tracked>(Tracked(30));  // 2 ctors (temp + copy), 1 dtor (temp)

        EXPECT_EQ(vec.size<Tracked>(), 3u);
        // 6 constructions (3 temps + 3 in-place), 3 destructions (3 temps)
        EXPECT_EQ(Tracked::ctor_count, 6);
        EXPECT_EQ(Tracked::dtor_count, 3);

        // Verify values
        EXPECT_EQ(vec.data<Tracked>()[0], 10);
        EXPECT_EQ(vec.data<Tracked>()[1], 20);
        EXPECT_EQ(vec.data<Tracked>()[2], 30);
    } // vec goes out of scope here

    // After destruction, all objects should be destroyed
    EXPECT_EQ(Tracked::ctor_count, 6);
    EXPECT_EQ(Tracked::dtor_count, 6);  // 3 in-place destroyed
}

TEST(MultiVector, ProperDestructionWithDefaults) {
    // Test that default-initialized objects are properly destroyed
    Tracked::reset_counts();

    {
        // Create vector with default values
        auto vec = multi_vector<Tracked, int>::builder()
            .capacity<Tracked>(4)
            .capacity<int>(2)
            .default_value<Tracked>(Tracked(99))  // Creates temp + copy to optional + 4 copies in vector
            .default_value<int>(42)
            .build();

        // default_value creates: 1 temp Tracked(99) + 1 copy in optional + 4 copies in vector
        EXPECT_EQ(vec.size<Tracked>(), 4u);
        EXPECT_EQ(vec.size<int>(), 2u);

        // Should have 6 constructions (1 temp + 1 in optional + 4 in vector)
        EXPECT_EQ(Tracked::ctor_count, 6);
        // 2 destructions (the temp + the optional's copy at end of build())
        EXPECT_EQ(Tracked::dtor_count, 2);

        // Verify all values are initialized
        for (std::size_t i = 0; i < 4; ++i) {
            EXPECT_EQ(vec.data<Tracked>()[i], 99);
        }
    } // vec goes out of scope

    // After destruction, all objects should be destroyed: 4 in vector + 1 temp + 1 in optional
    EXPECT_EQ(Tracked::ctor_count, 6);
    EXPECT_EQ(Tracked::dtor_count, 6);
}

TEST(MultiVector, ProperDestructionOnMove) {
    // Test that moved-from vectors don't double-destroy
    Tracked::reset_counts();

    {
        auto vec1 = multi_vector<Tracked>::builder()
            .capacity<Tracked>(3)
            .build();

        vec1.push_back<Tracked>(Tracked(100));
        vec1.push_back<Tracked>(Tracked(200));

        // 4 constructions (2 temps + 2 in vec1), 2 destructions (2 temps)
        EXPECT_EQ(Tracked::ctor_count, 4);
        EXPECT_EQ(Tracked::dtor_count, 2);

        {
            // Move construct
            auto vec2 = std::move(vec1);

            // No new constructions/destructions from move
            EXPECT_EQ(Tracked::ctor_count, 4);
            EXPECT_EQ(Tracked::dtor_count, 2);

            EXPECT_EQ(vec2.size<Tracked>(), 2u);
            EXPECT_EQ(vec1.size<Tracked>(), 0u);  // moved-from
        } // vec2 destroyed, vec1 should not destroy anything

        // Only vec2's elements destroyed
        EXPECT_EQ(Tracked::ctor_count, 4);
        EXPECT_EQ(Tracked::dtor_count, 4);
    } // vec1 destroyed (but has no elements)

    // No additional destructions
    EXPECT_EQ(Tracked::ctor_count, 4);
    EXPECT_EQ(Tracked::dtor_count, 4);
}