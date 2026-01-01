#include <gmock/gmock.h>

#include <sstream>
#include <trx/trx.hpp>

namespace
{

constexpr auto is_even = [](int value) { return value % 2 == 0; };

constexpr inline struct str_fn
{
    template <class... Args>
    std::string operator()(Args&&... args) const
    {
        std::stringstream ss;
        (ss << ... << std::forward<Args>(args));
        return ss.str();
    }
} str{};

}  // namespace

TEST(reducers, partition)
{
    std::vector<int> even;
    std::vector<int> odd;
    const auto result = trx::from(std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 })
        |= trx::partition(is_even, trx::push_back(even), trx::push_back(odd));

    EXPECT_THAT(even, testing::ElementsAre(2, 4, 6, 8, 10));
    EXPECT_THAT(odd, testing::ElementsAre(1, 3, 5, 7, 9));
}

TEST(reducers, fork)
{
    std::vector<int> values;
    std::vector<std::string> str_values;

    const auto [a, b, c, d] = trx::from(std::vector<int>{ 1, 2, 3, 4, 5 }) |= trx::fork(
        trx::push_back(values),  //
        trx::transform(str) |= trx::push_back(str_values),
        trx::filter(is_even) |= trx::into(std::vector<int>{}),
        trx::filter(is_even) |= trx::count);

    EXPECT_THAT(values, testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(str_values, testing::ElementsAre("1", "2", "3", "4", "5"));

    EXPECT_THAT(a.get(), testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(b.get(), testing::ElementsAre("1", "2", "3", "4", "5"));
    EXPECT_THAT(c, testing::ElementsAre(2, 4));
    EXPECT_THAT(d, 2);
}

TEST(reducers, output_iterator)
{
    const std::vector<int> input = { 1, 2, 3, 4, 5 };
    std::vector<int> result(2);
    auto res = std::copy(
        input.begin(),
        input.end(),
        trx::out(
            trx::filter([](int x) { return x % 2 == 0; }) |= trx::transform([](int v) { return v * 10; })
            |= trx::copy_to(result.begin())));

    EXPECT_THAT(result, testing::ElementsAre(20, 40));
    EXPECT_THAT(std::distance(result.begin(), res.get()), 2);
}

TEST(reducers, output_iterator_with_std_generate_n)
{
    EXPECT_THAT(
        std::generate_n(
            trx::out(trx::transform(str) |= trx::into(std::vector<std::string>{})),
            7,
            [state = std::make_pair(1, 1)]() mutable -> int
            {
                const auto value = state.first;
                state = std::make_pair(state.second, state.first + state.second);
                return value;
            })
            .get(),
        testing::ElementsAre("1", "1", "2", "3", "5", "8", "13"));
}

TEST(reducers, output_iterator_set_operations)
{
    const std::vector<int> a = { 1, 2, 3, 4, 5 };
    const std::vector<int> b = { 2, 4, 6, 8, 10 };

    EXPECT_THAT(
        std::set_union(
            a.begin(), a.end(), b.begin(), b.end(), trx::out(trx::transform(str) |= trx::into(std::vector<std::string>{})))
            .get(),
        testing::ElementsAre("1", "2", "3", "4", "5", "6", "8", "10"));

    EXPECT_THAT(
        std::set_difference(
            a.begin(), a.end(), b.begin(), b.end(), trx::out(trx::transform(str) |= trx::into(std::vector<std::string>{})))
            .get(),
        testing::ElementsAre("1", "3", "5"));

    EXPECT_THAT(
        std::set_symmetric_difference(
            a.begin(), a.end(), b.begin(), b.end(), trx::out(trx::transform(str) |= trx::into(std::vector<std::string>{})))
            .get(),
        testing::ElementsAre("1", "3", "5", "6", "8", "10"));

    EXPECT_THAT(
        std::set_intersection(
            a.begin(), a.end(), b.begin(), b.end(), trx::out(trx::transform(str) |= trx::into(std::vector<std::string>{})))
            .get(),
        testing::ElementsAre("2", "4"));
}

TEST(reducers, sum)
{
    EXPECT_THAT(trx::from(std::vector<int>{ 1, 2, 3, 4, 5 }) |= trx::sum(0), 15);
}

TEST(reducers, generator)
{
    const auto result = trx::generator_t<int>(
        [](trx::generator_t<int>::yield_fn yield)
        {
            auto state = std::make_pair(1, 1);
            while (true)
            {
                if (!yield(state.first))
                {
                    break;
                }
                state = std::make_pair(state.second, state.first + state.second);
            }
        })
        |= trx::transform(str)  //
        |= trx::take(7)         //
        |= trx::into(std::vector<std::string>{});
    EXPECT_THAT(result, testing::ElementsAre("1", "1", "2", "3", "5", "8", "13"));
}

TEST(reducers, ternary_generator)
{
    const auto result = trx::generator_t<int, int, int>(
        [](trx::generator_t<int, int, int>::yield_fn yield)
        {
            for (int i = 0; i <= 1000; ++i)
            {
                ASSERT_THAT(i, testing::Le(11));
                if (!yield(i, i * i, i * i * i))
                {
                    break;
                }
            }
        })
        |= trx::transform([](int value, int square, int cube) { return str(value, "/", square, "/", cube); })  //
        |= trx::take(10)                                                                                       //
        |= trx::intersperse(std::string{ ", " })                                                               //
        |= trx::join                                                                                           //
        |= trx::into(std::string{});
    EXPECT_THAT(result, "0/0/0, 1/1/1, 2/4/8, 3/9/27, 4/16/64, 5/25/125, 6/36/216, 7/49/343, 8/64/512, 9/81/729");
}

TEST(reducers, pythagorean_triples)
{
    const auto result = trx::generator_t<int, int, int>(
        [](trx::generator_t<int, int, int>::yield_fn yield)
        {
            for (int a = 1; a <= 20; ++a)
            {
                for (int b = a; b <= 20; ++b)
                {
                    for (int c = b; c <= 20; ++c)
                    {
                        if (a * a + b * b == c * c)
                        {
                            if (!yield(a, b, c))
                            {
                                return;
                            }
                        }
                    }
                }
            }
        })
        |= trx::transform([](int a, int b, int c) { return str("(", a, ", ", b, ", ", c, ")"); })  //
        |= trx::intersperse(std::string{ ", " })                                                   //
        |= trx::join                                                                               //
        |= trx::into(std::string{});
    EXPECT_THAT(result, "(3, 4, 5), (5, 12, 13), (6, 8, 10), (8, 15, 17), (9, 12, 15), (12, 16, 20)");
}

TEST(reducers, chain)
{
    std::vector<int> input_a = { 1, 2, 3 };
    std::vector<int> input_b = { 10, 20, 30 };
    const auto result = trx::chain(input_a, input_b) |= trx::into(std::vector<int>{});
    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 10, 20, 30));
}