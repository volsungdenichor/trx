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
    const auto result = trx::from(
        std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 },
        trx::partition(is_even, trx::push_back(even), trx::push_back(odd)));

    EXPECT_THAT(even, testing::ElementsAre(2, 4, 6, 8, 10));
    EXPECT_THAT(odd, testing::ElementsAre(1, 3, 5, 7, 9));
}

TEST(reducers, fork)
{
    std::vector<int> values;
    std::vector<std::string> str_values;

    const auto [a, b, c, d] = trx::from(
        std::vector<int>{ 1, 2, 3, 4, 5 },
        trx::fork(
            trx::push_back(values),  //
            trx::transform(str) |= trx::push_back(str_values),
            trx::filter(is_even) |= trx::into(std::vector<int>{}),
            trx::filter(is_even) |= trx::count));

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