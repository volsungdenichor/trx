#include <gmock/gmock.h>

#include <flux/flux.hpp>

namespace
{

constexpr auto is_even = [](int value) { return value % 2 == 0; };

constexpr inline struct uppercase_fn
{
    char operator()(char ch) const
    {
        return std::toupper(ch);
    }

    std::string operator()(std::string str) const
    {
        std::transform(str.begin(), str.end(), str.begin(), std::cref(*this));
        return str;
    }
} uppercase{};

constexpr inline struct lowercase_fn
{
    char operator()(char ch) const
    {
        return std::tolower(ch);
    }

    std::string operator()(std::string str) const
    {
        std::transform(str.begin(), str.end(), str.begin(), std::cref(*this));
        return str;
    }
} lowercase{};

}  // namespace

TEST(transducers, basic_usage)
{
    const auto result
        = flux::reduce(flux::reducer_proxy_t{ 0, flux::to_reducer(std::plus<>{}) }, std::vector<int>{ 1, 2, 3, 4, 5 });

    EXPECT_THAT(result, 15);
}

TEST(transducers, any_of)
{
    const auto xform = flux::any_of(is_even);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{}), false);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 1, 3, 5, 7, 8 }), true);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 3, 5, 7, 9 }), false);
}

TEST(transducers, all_of)
{
    const auto xform = flux::all_of(is_even);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{}), true);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 2, 4, 6, 8 }), true);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 2, 4, 5, 8 }), false);
}

TEST(transducers, none_of)
{
    const auto xform = flux::none_of(is_even);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{}), true);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 3, 5, 7, 9 }), true);
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 3, 4, 7, 9 }), false);
}

TEST(transducers, transform)
{
    const auto xform = flux::transform(uppercase) |= flux::into(std::vector<std::string>{});
    const auto result = flux::reduce(xform, std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas" });

    EXPECT_THAT(result, testing::ElementsAre("ALABAMA", "ALASKA", "ARIZONA", "ARKANSAS"));
}

TEST(transducers, filter)
{
    const auto xform = flux::filter(is_even) |= flux::into(std::vector<int>{});
    const auto result = flux::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });

    EXPECT_THAT(result, testing::ElementsAre(2, 4, 6, 8, 10));
}

TEST(transducers, take)
{
    const auto xform = flux::take(3) |= flux::into(std::vector<int>{});

    EXPECT_THAT(flux::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 1, 2 }), testing::ElementsAre(1, 2));
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), testing::ElementsAre(1, 2, 3));
}

TEST(transducers, drop)
{
    const auto xform = flux::drop(3) |= flux::into(std::vector<int>{});

    EXPECT_THAT(flux::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 1, 2 }), testing::IsEmpty());
    EXPECT_THAT(
        flux::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), testing::ElementsAre(4, 5, 6, 7, 8, 9, 10));
}

TEST(transducers, stride)
{
    const auto xform = flux::stride(3) |= flux::into(std::vector<int>{});

    EXPECT_THAT(flux::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 1, 2 }), testing::ElementsAre(1));
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), testing::ElementsAre(1, 4, 7, 10));
}

TEST(transducers, take_while)
{
    const auto xform = flux::take_while(is_even) |= flux::into(std::vector<int>{});

    EXPECT_THAT(flux::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 2, 3, 4 }), testing::ElementsAre(2));
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 1, 2, 3 }), testing::IsEmpty());
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 2, 4, 6, 8 }), testing::ElementsAre(2, 4, 6, 8));
}

TEST(transducers, drop_while)
{
    const auto xform = flux::drop_while(is_even) |= flux::into(std::vector<int>{});

    EXPECT_THAT(flux::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 2, 3, 4 }), testing::ElementsAre(3, 4));
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 1, 2, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(flux::reduce(xform, std::vector<int>{ 2, 4, 6, 8 }), testing::IsEmpty());
}

TEST(transducers, join)
{
    const auto xform = flux::join |= flux::into(std::vector<int>{});
    const auto result = flux::reduce(
        xform,
        std::vector<std::vector<int>>{
            { 1, 2, 3 },
            { 4, 5 },
            {},
            { 6, 7, 8, 9 },
        });

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST(transducers, intersperse)
{
    const auto xform = flux::intersperse(',') |= flux::into(std::string{});

    EXPECT_THAT(flux::reduce(xform, std::string{ "hello" }), testing::Eq("h,e,l,l,o"));
    EXPECT_THAT(flux::reduce(xform, std::string{ "" }), testing::Eq(""));
}
