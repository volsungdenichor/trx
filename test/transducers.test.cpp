#include <gmock/gmock.h>

#include <trx/trx.hpp>

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
        = trx::reduce(trx::reducer_proxy_t{ 0, trx::to_reducer(std::plus<>{}) }, std::vector<int>{ 1, 2, 3, 4, 5 });

    EXPECT_THAT(result, 15);
    EXPECT_THAT((std::vector<int>{ 5, 10, 15 } |= trx::reducer_proxy_t{ 0, trx::to_reducer(std::plus<>{}) }), 30);
}

TEST(transducers, any_of)
{
    const auto xform = trx::any_of(is_even);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), false);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 3, 5, 7, 8 }), true);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 3, 5, 7, 9 }), false);

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, false);
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 3, 5, 7, 8 }) |= xform, true);
    EXPECT_THAT(trx::from(std::vector<int>{ 3, 5, 7, 9 }) |= xform, false);
}

TEST(transducers, all_of)
{
    const auto xform = trx::all_of(is_even);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), true);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 4, 6, 8 }), true);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 4, 5, 8 }), false);

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, true);
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 4, 6, 8 }) |= xform, true);
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 4, 5, 8 }) |= xform, false);
}

TEST(transducers, none_of)
{
    const auto xform = trx::none_of(is_even);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), true);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 3, 5, 7, 9 }), true);
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 3, 4, 7, 9 }), false);

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, true);
    EXPECT_THAT(trx::from(std::vector<int>{ 3, 5, 7, 9 }) |= xform, true);
    EXPECT_THAT(trx::from(std::vector<int>{ 3, 4, 7, 9 }) |= xform, false);
}

TEST(transducers, transform)
{
    const auto xform = trx::transform(uppercase) |= trx::into(std::vector<std::string>{});

    EXPECT_THAT(
        trx::reduce(xform, std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas" }),
        testing::ElementsAre("ALABAMA", "ALASKA", "ARIZONA", "ARKANSAS"));
    EXPECT_THAT(
        trx::from(std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas" }) |= xform,
        testing::ElementsAre("ALABAMA", "ALASKA", "ARIZONA", "ARKANSAS"));
}

TEST(transducers, transform_indexed)
{
    const auto xform = trx::transform_indexed(
        [](std::ptrdiff_t index, const std::string& str) -> std::string
        { return index % 2 == 0 ? uppercase(str) : lowercase(str); })
        |= trx::into(std::vector<std::string>{});

    EXPECT_THAT(
        trx::reduce(xform, std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas" }),
        testing::ElementsAre("ALABAMA", "alaska", "ARIZONA", "arkansas"));
    EXPECT_THAT(
        trx::from(std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas" }) |= xform,
        testing::ElementsAre("ALABAMA", "alaska", "ARIZONA", "arkansas"));
}

TEST(transducers, transform_maybe)
{
    const auto xform = trx::transform_maybe(
        [](const std::string& str) -> std::optional<std::string>
        {
            if (str[0] == 'A')
            {
                return uppercase(str);
            }
            return std::nullopt;
        })
        |= trx::into(std::vector<std::string>{});

    EXPECT_THAT(
        trx::reduce(xform, std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas", "California", "Colorado" }),
        testing::ElementsAre("ALABAMA", "ALASKA", "ARIZONA", "ARKANSAS"));
    EXPECT_THAT(
        trx::from(std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas", "California", "Colorado" }) |= xform,
        testing::ElementsAre("ALABAMA", "ALASKA", "ARIZONA", "ARKANSAS"));
}

TEST(transducers, transform_maybe_indexed)
{
    const auto xform = trx::transform_maybe_indexed(
        [](std::ptrdiff_t index, const std::string& str) -> std::optional<std::string>
        {
            if (index % 2 == 0)
            {
                return uppercase(str);
            }
            return std::nullopt;
        })
        |= trx::into(std::vector<std::string>{});

    EXPECT_THAT(
        trx::reduce(xform, std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas", "California", "Colorado" }),
        testing::ElementsAre("ALABAMA", "ARIZONA", "CALIFORNIA"));
    EXPECT_THAT(
        trx::from(std::vector<std::string>{ "Alabama", "Alaska", "Arizona", "Arkansas", "California", "Colorado" }) |= xform,
        testing::ElementsAre("ALABAMA", "ARIZONA", "CALIFORNIA"));
}

TEST(transducers, filter)
{
    const auto xform = trx::filter(is_even) |= trx::into(std::vector<int>{});

    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), testing::ElementsAre(2, 4, 6, 8, 10));
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }) |= xform, testing::ElementsAre(2, 4, 6, 8, 10));
}

TEST(transducers, filter_indexed)
{
    const auto xform = trx::filter_indexed([](std::ptrdiff_t index, int x) { return index % 3 == 0 || x % 2 == 0; })
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(
        trx::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), testing::ElementsAre(1, 2, 4, 6, 7, 8, 10));
    EXPECT_THAT(
        trx::from(std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }) |= xform, testing::ElementsAre(1, 2, 4, 6, 7, 8, 10));
}

TEST(transducers, take)
{
    const auto xform = trx::take(3) |= trx::into(std::vector<int>{});

    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2 }), testing::ElementsAre(1, 2));
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), testing::ElementsAre(1, 2, 3));

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2 }) |= xform, testing::ElementsAre(1, 2));
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }) |= xform, testing::ElementsAre(1, 2, 3));
}

TEST(transducers, drop)
{
    const auto xform = trx::drop(3) |= trx::into(std::vector<int>{});

    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2 }), testing::IsEmpty());
    EXPECT_THAT(
        trx::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), testing::ElementsAre(4, 5, 6, 7, 8, 9, 10));

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2 }) |= xform, testing::IsEmpty());
    EXPECT_THAT(
        trx::from(std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }) |= xform, testing::ElementsAre(4, 5, 6, 7, 8, 9, 10));
}

TEST(transducers, stride)
{
    const auto xform = trx::stride(3) |= trx::into(std::vector<int>{});

    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2 }), testing::ElementsAre(1));
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }), testing::ElementsAre(1, 4, 7, 10));

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2 }) |= xform, testing::ElementsAre(1));
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }) |= xform, testing::ElementsAre(1, 4, 7, 10));
}

TEST(transducers, take_while)
{
    const auto xform = trx::take_while(is_even) |= trx::into(std::vector<int>{});

    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 3, 4 }), testing::ElementsAre(2));
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2, 3 }), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 4, 6, 8 }), testing::ElementsAre(2, 4, 6, 8));

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 3, 4 }) |= xform, testing::ElementsAre(2));
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2, 3 }) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 4, 6, 8 }) |= xform, testing::ElementsAre(2, 4, 6, 8));
}

TEST(transducers, take_while_indexed)
{
    const auto xform = trx::take_while_indexed([](std::ptrdiff_t index, int value) { return index < 2 && is_even(value); })
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 3, 4 }), testing::ElementsAre(2));
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2, 3 }), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 4, 6, 8 }), testing::ElementsAre(2, 4));

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 3, 4 }) |= xform, testing::ElementsAre(2));
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2, 3 }) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 4, 6, 8 }) |= xform, testing::ElementsAre(2, 4));
}

TEST(transducers, drop_while)
{
    const auto xform = trx::drop_while(is_even) |= trx::into(std::vector<int>{});

    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 3, 4 }), testing::ElementsAre(3, 4));
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 4, 6, 8 }), testing::IsEmpty());

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 3, 4 }) |= xform, testing::ElementsAre(3, 4));
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2, 3 }) |= xform, testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 4, 6, 8 }) |= xform, testing::IsEmpty());
}

TEST(transducers, drop_while_indexed)
{
    const auto xform = trx::drop_while_indexed([](std::ptrdiff_t index, int value) { return index < 2 && is_even(value); })
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(trx::reduce(xform, std::vector<int>{}), testing::IsEmpty());
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 3, 4 }), testing::ElementsAre(3, 4));
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 1, 2, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(trx::reduce(xform, std::vector<int>{ 2, 4, 6, 8 }), testing::ElementsAre(6, 8));

    EXPECT_THAT(trx::from(std::vector<int>{}) |= xform, testing::IsEmpty());
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 3, 4 }) |= xform, testing::ElementsAre(3, 4));
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2, 3 }) |= xform, testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(trx::from(std::vector<int>{ 2, 4, 6, 8 }) |= xform, testing::ElementsAre(6, 8));
}

TEST(transducers, join)
{
    const auto xform = trx::join |= trx::into(std::vector<int>{});
    const auto result = trx::reduce(
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
    const auto xform = trx::intersperse(',') |= trx::into(std::string{});

    EXPECT_THAT(trx::reduce(xform, std::string{ "hello" }), testing::Eq("h,e,l,l,o"));
    EXPECT_THAT(trx::reduce(xform, std::string{ "" }), testing::Eq(""));

    EXPECT_THAT(trx::from(std::string{ "hello" }) |= xform, testing::Eq("h,e,l,l,o"));
    EXPECT_THAT(trx::from(std::string{ "" }) |= xform, testing::Eq(""));
}

TEST(transducers, join_take_with_early_termination)
{
    EXPECT_THAT(
        trx::from(std::vector<std::string>{ "Alpha", "Beta", "Gamma", "Delta" }) |= trx::join |= trx::take(10)
        |= trx::into(std::string{}),
        testing::Eq("AlphaBetaG"));
}

auto sample() -> trx::generator_t<int>
{
    return trx::generator_t<int>{ [](auto yield)
                                  {
                                      yield(10);
                                      yield(12);
                                      yield(14);
                                  } };
}

TEST(transducers, working_with_visitor)
{
    EXPECT_THAT(sample() |= trx::into(std::vector<int>{}), testing::ElementsAre(10, 12, 14));
    EXPECT_THAT(sample() |= trx::all_of([](int x) { return x % 2 == 0; }), true);
    EXPECT_THAT(
        sample() |= trx::filter([](int x) { return x != 10; })  //
        |= trx::transform([](int x) { return x * 2; })          //
        |= trx::into(std::vector<int>{}),
        testing::ElementsAre(24, 28));
}

TEST(transducers, range)
{
    EXPECT_THAT(trx::range(5, 10) |= trx::into(std::vector<int>{}), testing::ElementsAre(5, 6, 7, 8, 9));
    EXPECT_THAT(trx::range(3) |= trx::into(std::vector<int>{}), testing::ElementsAre(0, 1, 2));
}

TEST(transducers, iota)
{
    EXPECT_THAT(trx::iota() |= trx::take(5) |= trx::into(std::vector<int>{}), testing::ElementsAre(0, 1, 2, 3, 4));
    EXPECT_THAT(trx::iota(5) |= trx::take(5) |= trx::into(std::vector<int>{}), testing::ElementsAre(5, 6, 7, 8, 9));
}

TEST(transducers, read_lines)
{
    std::istringstream is{ "First line\nSecond line\r\nThird line\nFourth line" };

    EXPECT_THAT(
        trx::read_lines(is) |= trx::into(std::vector<std::string>{}),
        testing::ElementsAre("First line", "Second line", "Third line", "Fourth line"));
}

TEST(transducers, unpack)
{
    const auto xform = trx::unpack |= trx::transform([](int x, int y) { return x + y; }) |= trx::into(std::vector<int>{});

    EXPECT_THAT(
        trx::reduce(
            xform,
            std::vector<std::pair<int, int>>{
                { 1, 2 },
                { 3, 4 },
                { 5, 6 },
            }),
        testing::ElementsAre(3, 7, 11));
}
