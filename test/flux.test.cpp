#include <gmock/gmock.h>

#include <flux/flux.hpp>

constexpr auto is_even = [](int value) { return value % 2 == 0; };

TEST(flux, basic_usage)
{
    const auto result
        = flux::reduce(flux::reducer_proxy_t{ 0, flux::to_reducer(std::plus<>{}) }, std::vector<int>{ 1, 2, 3, 4, 5 });

    EXPECT_THAT(result, 15);
}

TEST(flux, any_of)
{
    EXPECT_THAT(flux::reduce(flux::any_of(is_even), std::vector<int>{}), false);
    EXPECT_THAT(flux::reduce(flux::any_of(is_even), std::vector<int>{ 1, 3, 5, 7, 8 }), true);
    EXPECT_THAT(flux::reduce(flux::any_of(is_even), std::vector<int>{ 3, 5, 7, 9 }), false);
}

TEST(flux, all_of)
{
    EXPECT_THAT(flux::reduce(flux::all_of(is_even), std::vector<int>{}), true);
    EXPECT_THAT(flux::reduce(flux::all_of(is_even), std::vector<int>{ 2, 4, 6, 8 }), true);
    EXPECT_THAT(flux::reduce(flux::all_of(is_even), std::vector<int>{ 2, 4, 5, 8 }), false);
}

TEST(flux, none_of)
{
    EXPECT_THAT(flux::reduce(flux::none_of(is_even), std::vector<int>{}), true);
    EXPECT_THAT(flux::reduce(flux::none_of(is_even), std::vector<int>{ 3, 5, 7, 9 }), true);
    EXPECT_THAT(flux::reduce(flux::none_of(is_even), std::vector<int>{ 3, 4, 7, 9 }), false);
}
