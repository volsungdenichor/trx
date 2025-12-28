#include <gmock/gmock.h>

#include <trx/trx.hpp>
#include <sstream>

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
    const auto xform = trx::partition(is_even, trx::push_back(even), trx::push_back(odd));
    const auto result = trx::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });

    EXPECT_THAT(even, testing::ElementsAre(2, 4, 6, 8, 10));
    EXPECT_THAT(odd, testing::ElementsAre(1, 3, 5, 7, 9));
}

TEST(reducers, fork)
{
    std::vector<int> values;
    std::vector<std::string> str_values;

    const auto xform = trx::fork(
        trx::push_back(values),  //
        trx::transform(str) |= trx::push_back(str_values),
        trx::filter(is_even) |= trx::into(std::vector<int>{}),
        trx::filter(is_even) |= trx::count);
    const auto [a, b, c, d] = trx::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5 });

    EXPECT_THAT(values, testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(str_values, testing::ElementsAre("1", "2", "3", "4", "5"));

    EXPECT_THAT(a.get(), testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(b.get(), testing::ElementsAre("1", "2", "3", "4", "5"));
    EXPECT_THAT(c, testing::ElementsAre(2, 4));
    EXPECT_THAT(d, 2);
}
