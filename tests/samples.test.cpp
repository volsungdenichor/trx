#include <gmock/gmock.h>

#include <optional>
#include <string>
#include <trx/trx.hpp>
#include <vector>

TEST(samples, transform)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::vector<int> result = input                     //
        |= trx::transform([](int x) { return x * 2; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(2, 4, 6, 8, 10));
}

TEST(samples, transform_indexed)
{
    std::vector<std::string> input = { "a", "b", "c" };
    std::vector<std::string> result = input
        |= trx::transform_indexed([](std::ptrdiff_t idx, const std::string& s) { return std::to_string(idx) + ":" + s; })
        |= trx::into(std::vector<std::string>{});

    EXPECT_THAT(result, testing::ElementsAre("0:a", "1:b", "2:c"));
}

TEST(samples, filter)
{
    std::vector<int> input = { 1, 2, 3, 4, 5, 6 };
    std::vector<int> result = input                       //
        |= trx::filter([](int x) { return x % 2 == 0; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(2, 4, 6));
}

TEST(samples, filter_indexed)
{
    std::vector<int> input = { 10, 20, 30, 40, 50 };
    std::vector<int> result = input                                                     //
        |= trx::filter_indexed([](std::ptrdiff_t idx, int x) { return idx % 2 == 0; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(10, 30, 50));
}

TEST(samples, inspect)
{
    std::vector<int> input = { 1, 2, 3 };
    std::stringstream ss;
    std::vector<int> result = input                        //
        |= trx::inspect([&ss](int x) { ss << x << " "; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(ss.str(), "1 2 3 ");
}

TEST(samples, inspect_indexed)
{
    std::vector<int> input = { 1, 2, 3 };
    std::stringstream ss;
    std::vector<int> result = input                                                                          //
        |= trx::inspect_indexed([&ss](std::ptrdiff_t idx, int x) { ss << "[" << idx << "]=" << x << " "; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(ss.str(), "[0]=1 [1]=2 [2]=3 ");
}

TEST(samples, transform_maybe)
{
    std::vector<std::string> input = { "1", "2", "abc", "4" };
    std::vector<int> result = input  //
        |= trx::transform_maybe(
            [](const std::string& s) -> std::optional<int>
            {
                try
                {
                    return std::stoi(s);
                }
                catch (...)
                {
                    return std::nullopt;
                }
            })
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 4));
}

TEST(samples, transform_maybe_indexed)
{
    std::vector<std::string> input = { "1", "2", "abc", "4" };
    std::vector<int> result = input  //
        |= trx::transform_maybe_indexed(
            [](std::ptrdiff_t idx, const std::string& s) -> std::optional<int>
            {
                try
                {
                    return std::stoi(s) * idx;
                }
                catch (...)
                {
                    return std::nullopt;
                }
            })
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(0, 2, 12));
}

TEST(samples, take_while)
{
    std::vector<int> input = { 1, 2, 3, 4, 5, 3, 2, 1 };
    std::vector<int> result = input                      //
        |= trx::take_while([](int x) { return x < 4; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
}

TEST(samples, take_while_indexed)
{
    std::vector<int> input = { 1, 2, 3, 4, 5, 3, 2, 1 };
    std::vector<int> result = input                                                    //
        |= trx::take_while_indexed([](std::ptrdiff_t idx, int x) { return idx < 3; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
}

TEST(samples, drop_while)
{
    std::vector<int> input = { 1, 2, 3, 4, 5, 3, 2, 1 };
    std::vector<int> result = input                      //
        |= trx::drop_while([](int x) { return x < 4; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(4, 5, 3, 2, 1));
}

TEST(samples, drop_while_indexed)
{
    std::vector<int> input = { 1, 2, 3, 4, 5, 3, 2, 1 };
    std::vector<int> result = input                                                    //
        |= trx::drop_while_indexed([](std::ptrdiff_t idx, int x) { return idx < 3; })  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(4, 5, 3, 2, 1));
}

TEST(samples, take)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::vector<int> result = input  //
        |= trx::take(3)              //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
}

TEST(samples, drop)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::vector<int> result = input  //
        |= trx::drop(2)              //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(3, 4, 5));
}

TEST(samples, stride)
{
    std::vector<int> input = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<int> result = input  //
        |= trx::stride(2)            //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(0, 2, 4, 6, 8));
}

TEST(samples, join)
{
    std::vector<std::vector<int>> input = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
    std::vector<int> result = input  //
        |= trx::join                 //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(samples, intersperse)
{
    std::string input = "ABCD";
    std::string result = input    //
        |= trx::intersperse(',')  //
        |= trx::into(std::string{});

    EXPECT_THAT(result, testing::Eq("A,B,C,D"));
}

TEST(samples, all_of)
{
    std::vector<int> input = { 2, 4, 6, 8 };
    bool result = input |= trx::all_of([](int x) { return x % 2 == 0; });

    EXPECT_TRUE(result);
}

TEST(samples, any_of)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    bool result = input |= trx::any_of([](int x) { return x % 2 == 0; });

    EXPECT_TRUE(result);
}

TEST(samples, none_of)
{
    std::vector<int> input = { 1, 3, 5, 7 };
    bool result = input |= trx::none_of([](int x) { return x % 2 == 0; });

    EXPECT_TRUE(result);
}

TEST(samples, dev_null)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::stringstream ss;
    input |= trx::inspect([&ss](int x) { ss << x << " "; }) |= trx::dev_null;

    EXPECT_THAT(ss.str(), testing::Eq("1 2 3 4 5 "));
}

TEST(samples, partition)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    auto [first, second] = input  //
        |= trx::partition(        //
            [](int x) { return x % 2 == 0; },
            trx::into(std::vector<int>{}),
            trx::into(std::vector<int>{}));

    EXPECT_THAT(first, testing::ElementsAre(2, 4));
    EXPECT_THAT(second, testing::ElementsAre(1, 3, 5));
}

TEST(samples, fork)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    auto [first, second] = input  //
        |= trx::fork(             //
            trx::into(std::vector<int>{}),
            trx::count);

    EXPECT_THAT(first, testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_EQ(second, 5u);
}

TEST(samples, copy_to)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::vector<int> dest(5);
    input |= trx::copy_to(dest.begin());

    EXPECT_THAT(dest, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(samples, push_back)
{
    std::vector<int> input = { 1, 2, 3 };
    std::vector<int> result;
    input |= trx::push_back(result);

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
}

TEST(samples, into)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::vector<int> result = input |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(samples, count)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::size_t result = input |= trx::count;

    EXPECT_EQ(result, 5u);
}

TEST(samples, from)
{
    std::vector<int> input_a = { 1, 2, 3, 4 };
    std::vector<std::string> input_b = { "one", "two", "three", "four" };
    std::vector<std::string> result = trx::from(input_a, input_b)                                                  //
        |= trx::filter([](int a, const std::string& b) { return a % 2 == 0; })                                     //
        |= trx::transform([](int a, const std::string& b) -> std::string { return b + ":" + std::to_string(a); })  //
        |= trx::into(std::vector<std::string>{});

    EXPECT_THAT(result, testing::ElementsAre("two:2", "four:4"));
}

TEST(samples, chain)
{
    std::vector<int> input_a = { 1, 2, 3 };
    std::vector<int> input_b = { 10, 20, 30 };
    std::vector<int> result = trx::chain(input_a, input_b)  //
        |= trx::into(std::vector<int>{});

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 10, 20, 30));
}

TEST(samples, custom_generators)
{
    std::vector<std::string> result = trx::generator_t<int, int>(
        [](auto yield)
        {
            for (int i = 0; i < 10; ++i)
            {
                if (!yield(i, i * i))
                {
                    break;
                }
            }
        })
        |= trx::transform([](int v, int square) -> std::string { return std::to_string(v) + " " + std::to_string(square); })
        |= trx::take(4) |= trx::into(std::vector<std::string>{});

    EXPECT_THAT(result, testing::ElementsAre("0 0", "1 1", "2 4", "3 9"));
}

TEST(samples, out)
{
    std::vector<int> input = { 1, 2, 3 };
    std::vector<std::string> result
        = std::copy(
              input.begin(),
              input.end(),
              trx::out(trx::transform([](int x) { return std::to_string(x); }) |= trx::into(std::vector<std::string>{})))
              .get();

    EXPECT_THAT(result, testing::ElementsAre("1", "2", "3"));
}

TEST(samples, to_reducer)
{
    std::vector<int> input = { 5, 10, 15 };
    int result = input |= trx::reductor_t{ 0, trx::to_reducer(std::plus<>{}) };

    EXPECT_THAT(result, testing::Eq(30));
}

TEST(samples, for_each)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::vector<int> result;
    input |= trx::for_each([&result](int x) { result.push_back(x * 2); });
    EXPECT_THAT(result, testing::ElementsAre(2, 4, 6, 8, 10));
}

TEST(samples, for_each_indexed)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::vector<int> result;
    input |= trx::for_each_indexed([&result](std::ptrdiff_t idx, int x) { result.push_back(x * 2 + 100 * idx); });
    EXPECT_THAT(result, testing::ElementsAre(2, 104, 206, 308, 410));
}

TEST(samples, accumulate)
{
    std::vector<int> input = { 1, 2, 3, 4, 5 };
    int result = input |= trx::accumulate(0, [](int state, int x) { return state + x * x; });
    EXPECT_THAT(result, testing::Eq(55));
}

TEST(samples, unpack)
{
    std::vector<std::tuple<int, int, char>> input = { { 1, 2, 'a' }, { 2, 3, 'b' } };
    std::vector<std::string> result = input |= trx::unpack
        |= trx::transform([](int a, int b, char c) { return std::to_string(10 * a + b) + c; })
        |= trx::into(std::vector<std::string>{});

    EXPECT_THAT(result, testing::ElementsAre("12a", "23b"));
}

TEST(samples, project)
{
    struct S
    {
        int a;
        std::string b;
        char c;
    };
    std::vector<S> input = { { 10, "A", 'z' }, { 20, "BB", 'y' }, { 35, "CCC", 'x' } };
    std::vector<std::tuple<char, char, int>> result = input |= trx::project(&S::c, &S::b, [](const S& s) { return s.a; })
        |= trx::transform(
            [](char c, const std::string& b, int a) {
                return std::tuple{ c, b[0], a };
            })
        |= trx::into(std::vector<std::tuple<char, char, int>>{});

    EXPECT_THAT(
        result,
        testing::ElementsAre(std::make_tuple('z', 'A', 10), std::make_tuple('y', 'B', 20), std::make_tuple('x', 'C', 35)));
}