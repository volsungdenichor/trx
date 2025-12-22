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

TEST(reducers, partition)
{
    std::vector<int> even;
    std::vector<int> odd;
    const auto xform = flux::partition(is_even, flux::push_back(even), flux::push_back(odd));
    const auto result = flux::reduce(xform, std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });

    EXPECT_THAT(even, testing::ElementsAre(2, 4, 6, 8, 10));
    EXPECT_THAT(odd, testing::ElementsAre(1, 3, 5, 7, 9));
}
