#pragma once

#include <tuple>
#include <type_traits>

namespace flux
{

template <class State, class Reducer>
struct reducer_proxy_t
{
    using state_type = State;
    using reducer_type = Reducer;

    state_type state;
    reducer_type reducer;

    constexpr operator state_type() const
    {
        return state;
    }

    template <class... Args>
    void operator()(Args&&... args)
    {
        std::invoke(reducer, state, std::forward<Args>(args)...);
    }
};

template <class State, class Reducer>
reducer_proxy_t(State&&, Reducer&&) -> reducer_proxy_t<std::decay_t<State>, std::decay_t<Reducer>>;

template <class Transducer, class State, class Reducer>
constexpr auto operator|=(Transducer&& transducer, const reducer_proxy_t<State, Reducer>& proxy)
    -> reducer_proxy_t<State, std::invoke_result_t<Transducer, Reducer>>
{
    return { proxy.state, std::invoke(std::forward<Transducer>(transducer), proxy.reducer) };
}

template <class Transducer, class State, class Reducer>
constexpr auto operator|=(Transducer&& transducer, reducer_proxy_t<State, Reducer>&& proxy)
    -> reducer_proxy_t<State, std::invoke_result_t<Transducer, Reducer>>
{
    return { std::move(proxy.state), std::invoke(std::forward<Transducer>(transducer), std::move(proxy.reducer)) };
}

namespace detail
{

struct to_reducer_fn
{
    template <class Reducer>
    struct impl_t
    {
        Reducer m_reducer;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            state = std::invoke(m_reducer, std::move(state), std::forward<Args>(args)...);
        }
    };

    template <class Reducer>
    constexpr auto operator()(Reducer&& reducer) const -> impl_t<std::decay_t<Reducer>>
    {
        return { std::forward<Reducer>(reducer) };
    }
};

struct reduce_fn
{
    template <class State, class Reducer, class Range>
    constexpr auto operator()(reducer_proxy_t<State, Reducer> reducer, Range&& range) const -> State
    {
        auto it = std::begin(range);
        const auto end = std::end(range);
        for (; it != end; ++it)
        {
            std::invoke(reducer.reducer, reducer.state, *it);
        }
        return reducer.state;
    }

    template <class State, class Reducer, class Range_0, class Range_1>
    constexpr auto operator()(reducer_proxy_t<State, Reducer> reducer, Range_0&& range_0, Range_1&& range_1) const -> State
    {
        auto it_0 = std::begin(range_0);
        auto it_1 = std::begin(range_1);
        const auto end_0 = std::end(range_0);
        const auto end_1 = std::end(range_1);
        for (; it_0 != end_0 && it_1 != end_1; ++it_0, ++it_1)
        {
            std::invoke(reducer.reducer, reducer.state, *it_0, *it_1);
        }
        return reducer.state;
    }

    template <class State, class Reducer, class Range_0, class Range_1, class Range_2>
    constexpr auto operator()(
        reducer_proxy_t<State, Reducer> reducer, Range_0&& range_0, Range_1&& range_1, Range_2&& range_2) const -> State
    {
        auto it_0 = std::begin(range_0);
        auto it_1 = std::begin(range_1);
        auto it_2 = std::begin(range_2);
        const auto end_0 = std::end(range_0);
        const auto end_1 = std::end(range_1);
        const auto end_2 = std::end(range_2);
        for (; it_0 != end_0 && it_1 != end_1 && it_2 != end_2; ++it_0, ++it_1, ++it_2)
        {
            std::invoke(reducer.reducer, reducer.state, *it_0, *it_1, *it_2);
        }
        return reducer.state;
    }
};

struct invoke_fn
{
    template <class State, class Reducer, class... Args>
    constexpr auto operator()(reducer_proxy_t<State, Reducer> reducer, Args&&... args) const -> State
    {
        std::invoke(reducer.reducer, reducer.state, std::forward<Args>(args)...);
        return reducer.state;
    }
};

template <template <class...> class R, class Arg>
struct transducer_t
{
    Arg m_arg;

    template <class Reducer>
    constexpr auto operator()(Reducer&& next_reducer) const& -> R<std::decay_t<Reducer>, Arg>
    {
        return {
            std::forward<Reducer>(next_reducer),
            m_arg,
        };
    }

    template <class Reducer>
    constexpr auto operator()(Reducer&& next_reducer) && -> R<std::decay_t<Reducer>, Arg>
    {
        return {
            std::forward<Reducer>(next_reducer),
            std::move(m_arg),
        };
    }
};

template <template <class...> class R>
struct transducer_t<R, void>
{
    template <class Reducer>
    constexpr auto operator()(Reducer&& next_reducer) const& -> R<std::decay_t<Reducer>>
    {
        return { std::forward<Reducer>(next_reducer) };
    }

    template <class Reducer>
    constexpr auto operator()(Reducer&& next_reducer) && -> R<std::decay_t<Reducer>>
    {
        return { std::forward<Reducer>(next_reducer) };
    }
};

struct all_of_fn
{
    template <class Pred>
    struct reducer_t
    {
        Pred m_pred;

        template <class... Args>
        constexpr void operator()(bool& state, Args&&... args) const
        {
            state = state && std::invoke(m_pred, std::forward<Args>(args)...);
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reducer_proxy_t<bool, reducer_t<std::decay_t<Pred>>>
    {
        return { true, reducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) } };
    }
};

struct any_of_fn
{
    template <class Pred>
    struct reducer_t
    {
        Pred m_pred;

        template <class... Args>
        constexpr void operator()(bool& state, Args&&... args) const
        {
            state = state || std::invoke(m_pred, std::forward<Args>(args)...);
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reducer_proxy_t<bool, reducer_t<std::decay_t<Pred>>>
    {
        return { false, reducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) } };
    }
};

struct none_of_fn
{
    template <class Pred>
    struct reducer_t
    {
        Pred m_pred;

        template <class... Args>
        constexpr void operator()(bool& state, Args&&... args) const
        {
            state = state && !std::invoke(m_pred, std::forward<Args>(args)...);
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reducer_proxy_t<bool, reducer_t<std::decay_t<Pred>>>
    {
        return { true, reducer_t<std::decay_t<Pred>>{ std::forward<Pred>(pred) } };
    }
};

struct filter_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            if (std::invoke(m_pred, args...))
            {
                m_next_reducer(state, std::forward<Args>(args)...);
            }
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> transducer_t<reducer_t, std::decay_t<Pred>>
    {
        return { std::forward<Pred>(pred) };
    }
};

struct transform_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            return m_next_reducer(state, std::invoke(m_func, std::forward<Args>(args)...));
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> transducer_t<reducer_t, std::decay_t<Func>>
    {
        return { std::forward<Func>(func) };
    }
};

struct inspect_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            std::invoke(m_func, args...);
            m_next_reducer(state, std::forward<Args>(args)...);
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> transducer_t<reducer_t, std::decay_t<Func>>
    {
        return { std::forward<Func>(func) };
    }
};

struct transform_maybe_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            if (auto res = std::invoke(m_func, std::forward<Args>(args)...))
            {
                m_next_reducer(state, *std::move(res));
            }
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> transducer_t<reducer_t, std::decay_t<Func>>
    {
        return { std::forward<Func>(func) };
    }
};

struct take_while_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;

        mutable bool m_done = false;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            m_done |= !std::invoke(m_pred, args...);
            if (!m_done)
            {
                m_next_reducer(state, std::forward<Args>(args)...);
            }
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> transducer_t<reducer_t, std::decay_t<Pred>>
    {
        return { std::forward<Pred>(pred) };
    }
};

struct drop_while_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;

        mutable bool m_done = false;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            m_done |= !std::invoke(m_pred, args...);
            if (m_done)
            {
                m_next_reducer(state, std::forward<Args>(args)...);
            }
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> transducer_t<reducer_t, std::decay_t<Pred>>
    {
        return { std::forward<Pred>(pred) };
    }
};

struct take_fn
{
    template <class Reducer, class>
    struct reducer_t
    {
        Reducer m_next_reducer;
        mutable std::ptrdiff_t m_count;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            if (m_count-- > 0)
            {
                m_next_reducer(state, std::forward<Args>(args)...);
            }
        }
    };

    constexpr auto operator()(std::ptrdiff_t count) const -> transducer_t<reducer_t, std::ptrdiff_t>
    {
        return { count };
    }
};

struct drop_fn
{
    template <class Reducer, class>
    struct reducer_t
    {
        Reducer m_next_reducer;
        mutable std::ptrdiff_t m_count;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            if (m_count-- <= 0)
            {
                m_next_reducer(state, std::forward<Args>(args)...);
            }
        }
    };

    constexpr auto operator()(std::ptrdiff_t count) const -> transducer_t<reducer_t, std::ptrdiff_t>
    {
        return { count };
    }
};

struct stride_fn
{
    template <class Reducer, class>
    struct reducer_t
    {
        Reducer m_next_reducer;
        std::ptrdiff_t m_count;
        mutable std::ptrdiff_t m_index = 0;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            if (m_index++ % m_count == 0)
            {
                m_next_reducer(state, std::forward<Args>(args)...);
            }
        }
    };

    constexpr auto operator()(std::ptrdiff_t count) const -> transducer_t<reducer_t, std::ptrdiff_t>
    {
        return { count };
    }
};

struct partition_fn
{
    template <class Pred, class OnTrueReducer, class OnFalseReducer>
    struct reducer_t
    {
        Pred m_pred;
        OnTrueReducer m_on_true_reducer;
        OnFalseReducer m_on_false_reducer;

        template <class State, class... Args>
        constexpr void operator()(State& state, Args&&... args) const
        {
            if (std::invoke(m_pred, args...))
            {
                std::invoke(m_on_true_reducer, state.first, std::forward<Args>(args)...);
            }
            else
            {
                std::invoke(m_on_false_reducer, state.second, std::forward<Args>(args)...);
            }
        }
    };

    template <class Pred, class S0, class R0, class S1, class R1>
    constexpr auto operator()(Pred&& pred, reducer_proxy_t<S0, R0> on_true_reducer, reducer_proxy_t<S1, R1> on_false_reducer)
        const -> reducer_proxy_t<std::pair<S0, S1>, reducer_t<std::decay_t<Pred>, R0, R1>>
    {
        return { std::pair<S0, S1>{ std::move(on_true_reducer.state), std::move(on_false_reducer.state) },
                 {
                     std::forward<Pred>(pred),
                     std::move(on_true_reducer.reducer),
                     std::move(on_false_reducer.reducer),
                 } };
    }
};

struct join_fn
{
    template <class Reducer>
    struct reducer_t
    {
        Reducer m_next_reducer;

        template <class State, class Arg>
        constexpr void operator()(State& state, Arg&& arg) const
        {
            for (auto&& item : arg)
            {
                m_next_reducer(state, std::forward<decltype(item)>(item));
            }
        }
    };

    constexpr auto operator()() const -> transducer_t<reducer_t, void>
    {
        return {};
    }
};

struct intersperse_fn
{
    template <class Reducer, class Separator>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Separator m_separator;
        mutable bool m_first = true;

        template <class State, class Arg>
        constexpr void operator()(State& state, Arg&& arg) const
        {
            if (!m_first)
            {
                m_next_reducer(state, m_separator);
            }
            m_first = false;
            m_next_reducer(state, std::forward<Arg>(arg));
        }
    };

    template <class Separator>
    constexpr auto operator()(Separator&& separator) const -> transducer_t<reducer_t, std::decay_t<Separator>>
    {
        return { std::forward<Separator>(separator) };
    }
};

// REDUCERS

struct ignore_reducer_t
{
    template <class State, class... Args>
    void operator()(State&, Args&&...) const
    {
    }
};

struct copy_to_reducer_t
{
    template <class State, class Arg>
    void operator()(State& state, Arg&& arg) const
    {
        *state = std::forward<Arg>(arg);
        ++state;
    }
};

struct deref_fn
{
    template <class T>
    constexpr T& operator()(T& arg) const
    {
        return arg;
    }

    template <class T>
    constexpr T& operator()(T* arg) const
    {
        return *arg;
    }

    template <class T>
    constexpr T& operator()(std::reference_wrapper<T> arg) const
    {
        return (*this)(arg.get());
    }
};

static constexpr inline auto deref = deref_fn{};

struct push_back_reducer_t
{
    template <class State, class Arg>
    void operator()(State& state, Arg&& arg) const
    {
        deref(state).push_back(std::forward<Arg>(arg));
    }
};

struct copy_to_fn
{
    template <class>
    struct reducer_t
    {
        template <class State, class Arg>
        void operator()(State& state, Arg&& arg) const
        {
            *state = std::forward<Arg>(arg);
            ++state;
        }
    };

    template <class Out>
    constexpr auto operator()(Out out) const -> reducer_proxy_t<Out, copy_to_reducer_t>
    {
        return { std::move(out), copy_to_reducer_t{} };
    }
};

struct push_back_fn
{
    template <class Container>
    constexpr auto operator()(Container& container) const
        -> reducer_proxy_t<std::reference_wrapper<Container>, push_back_reducer_t>
    {
        return { container, push_back_reducer_t{} };
    }
};

struct into_fn
{
    template <class Container>
    constexpr auto operator()(Container&& container) const -> reducer_proxy_t<std::decay_t<Container>, push_back_reducer_t>
    {
        return { std::forward<Container>(container), push_back_reducer_t{} };
    }
};

struct fork_fn
{
    template <class... Reducers>
    struct reducer_t
    {
        std::tuple<Reducers...> m_reducers;

        template <std::size_t N, class State, class... Args>
        void call(State& state, Args&&... args) const
        {
            std::invoke(std::get<N>(m_reducers), std::get<N>(state), args...);
            if constexpr (N + 1 < sizeof...(Reducers))
            {
                call<N + 1>(state, args...);
            }
        }

        template <class State, class... Args>
        void operator()(State& state, Args&&... args) const
        {
            call<0>(state, args...);
        }
    };

    template <class... Reducers>
    constexpr auto operator()(Reducers... reducers) const
        -> reducer_proxy_t<std::tuple<typename Reducers::state_type...>, reducer_t<typename Reducers::reducer_type...>>
    {
        return { { std::move(reducers.state)... },
                 {
                     { std::move(reducers.reducer)... },
                 } };
    }
};

}  // namespace detail

constexpr inline auto reduce = detail::reduce_fn{};
constexpr inline auto invoke = detail::invoke_fn{};
constexpr inline auto to_reducer = detail::to_reducer_fn{};

static constexpr inline auto all_of = detail::all_of_fn{};
static constexpr inline auto any_of = detail::any_of_fn{};
static constexpr inline auto none_of = detail::none_of_fn{};

static constexpr inline auto transform = detail::transform_fn{};
static constexpr inline auto filter = detail::filter_fn{};
static constexpr inline auto inspect = detail::inspect_fn{};
static constexpr inline auto transform_maybe = detail::transform_maybe_fn{};

static constexpr inline auto take_while = detail::take_while_fn{};
static constexpr inline auto drop_while = detail::drop_while_fn{};

static constexpr inline auto take = detail::take_fn{};
static constexpr inline auto drop = detail::drop_fn{};
static constexpr inline auto stride = detail::stride_fn{};

static constexpr inline auto join = detail::join_fn{}();
static constexpr inline auto intersperse = detail::intersperse_fn{};

static constexpr inline auto dev_null = reducer_proxy_t{ 0, detail::ignore_reducer_t{} };

static constexpr inline auto partition = detail::partition_fn{};
static constexpr inline auto fork = detail::fork_fn{};

static constexpr inline auto copy_to = detail::copy_to_fn{};
static constexpr inline auto push_back = detail::push_back_fn{};
static constexpr inline auto into = detail::into_fn{};

static constexpr inline auto count
    = reducer_proxy_t{ std::ptrdiff_t{ 0 }, [](std::ptrdiff_t& state, auto&&...) { state += 1; } };

}  // namespace flux
