#pragma once

#ifndef TRX_NAMESPACE
#define TRX_NAMESPACE trx
#endif  // TRX_NAMESPACE

#include <bitset>
#include <functional>
#include <istream>
#include <tuple>
#include <type_traits>

namespace TRX_NAMESPACE
{

template <class State, class Reducer>
struct reductor_t
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
    constexpr auto operator()(Args&&... args) -> bool
    {
        return std::invoke(reducer, state, std::forward<Args>(args)...);
    }

    constexpr auto get() const& -> const state_type&
    {
        return state;
    }

    constexpr auto get() & -> state_type&
    {
        return state;
    }

    constexpr auto get() && -> state_type&&
    {
        return std::move(state);
    }
};

template <class State, class Reducer>
reductor_t(State&&, Reducer&&) -> reductor_t<std::decay_t<State>, std::decay_t<Reducer>>;

template <class State, class Reducer>
constexpr auto make_reductor(State&& state, Reducer&& reducer) -> reductor_t<std::decay_t<State>, std::decay_t<Reducer>>
{
    return { std::forward<State>(state), std::forward<Reducer>(reducer) };
}

template <class Signature>
struct function_ref;

template <class Ret, class... Args>
struct function_ref<Ret(Args...)>
{
    using return_type = Ret;
    using func_type = return_type (*)(void*, Args...);

    void* m_obj;
    func_type m_func;

    constexpr function_ref() = delete;
    constexpr function_ref(const function_ref&) = default;

    template <class Func>
    constexpr function_ref(Func&& func)
        : m_obj{ const_cast<void*>(reinterpret_cast<const void*>(std::addressof(func))) }
        , m_func{ [](void* obj, Args... args) -> return_type
                  { return std::invoke(*static_cast<std::add_pointer_t<Func>>(obj), std::forward<Args>(args)...); } }
    {
    }

    constexpr auto operator()(Args... args) const -> return_type
    {
        return m_func(m_obj, std::forward<Args>(args)...);
    }
};

template <class... Args>
using yield_fn = function_ref<bool(Args...)>;

template <class... Args>
using generator_t = std::function<void(yield_fn<Args...>)>;

namespace detail
{

template <class Callable>
using return_type_t = typename decltype(std::function{ std::declval<Callable>() })::result_type;

template <class T, class = void>
struct is_generator_impl : std::false_type
{
};

template <class YieldFn>
struct is_generator_impl<std::function<void(YieldFn)>, std::enable_if_t<std::is_convertible_v<return_type_t<YieldFn>, bool>>>
    : std::true_type
{
};

template <class T>
struct is_reductor_impl : std::false_type
{
};

template <class S, class R>
struct is_reductor_impl<reductor_t<S, R>> : std::true_type
{
};

template <class T, class = void>
struct is_range_impl : std::false_type
{
};

template <class T>
struct is_range_impl<T, std::void_t<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>>
    : std::true_type
{
};

template <class T>
struct is_transducer_impl
{
    static constexpr bool value = !is_generator_impl<T>::value && !is_reductor_impl<T>::value && !is_range_impl<T>::value;
};

}  // namespace detail

template <class T>
inline constexpr bool is_generator_v = detail::is_generator_impl<T>::value;

template <class T>
inline constexpr bool is_transducer_v = detail::is_transducer_impl<T>::value;

template <class T>
inline constexpr bool is_range_v = detail::is_range_impl<T>::value;

template <
    class Generator,
    class State,
    class Reducer,
    class G = std::decay_t<Generator>,
    std::enable_if_t<is_generator_v<G>, int> = 0>
constexpr auto operator|=(Generator&& generator, reductor_t<State, Reducer> reductor) -> State
{
    std::forward<Generator>(generator)(reductor);
    return reductor.state;
}

template <
    class Transducer,
    class State,
    class Reducer,
    class T = std::decay_t<Transducer>,
    std::enable_if_t<is_transducer_v<T>, int> = 0>
constexpr auto operator|=(Transducer&& transducer, const reductor_t<State, Reducer>& reductor)
    -> reductor_t<State, std::invoke_result_t<Transducer, Reducer>>
{
    return { reductor.state, std::invoke(std::forward<Transducer>(transducer), reductor.reducer) };
}

template <
    class Transducer,
    class State,
    class Reducer,
    class T = std::decay_t<Transducer>,
    std::enable_if_t<is_transducer_v<T>, int> = 0>
constexpr auto operator|=(Transducer&& transducer, reductor_t<State, Reducer>&& reductor)
    -> reductor_t<State, std::invoke_result_t<Transducer, Reducer>>
{
    return { std::move(reductor.state), std::invoke(std::forward<Transducer>(transducer), std::move(reductor.reducer)) };
}

template <class Range, class State, class Reducer, class R = std::decay_t<Range>, std::enable_if_t<is_range_v<R>, int> = 0>
constexpr auto operator|=(Range&& range, reductor_t<State, Reducer> reductor) -> State
{
    auto it = std::begin(range);
    const auto end = std::end(range);
    for (; it != end; ++it)
    {
        if (!reductor(*it))
        {
            break;
        }
    }
    return reductor.state;
}

namespace detail
{

struct to_reducer_fn
{
    template <class Reducer>
    struct adapter_t
    {
        Reducer m_reducer;

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            state = std::invoke(m_reducer, std::move(state), std::forward<Args>(args)...);
            return true;
        }
    };

    template <class Reducer>
    constexpr auto operator()(Reducer&& reducer) const -> adapter_t<std::decay_t<Reducer>>
    {
        return { std::forward<Reducer>(reducer) };
    }
};

template <class State, class Reducer>
struct output_iterator_t
{
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using state_type = State;
    using reducer_type = Reducer;

    reductor_t<State, Reducer> m_reductor;

    constexpr auto operator*() -> output_iterator_t&
    {
        return *this;
    }

    constexpr auto operator++() -> output_iterator_t&
    {
        return *this;
    }

    constexpr auto operator++(int) -> output_iterator_t&
    {
        return *this;
    }

    template <class Arg>
    constexpr auto operator=(Arg&& arg) -> output_iterator_t&
    {
        m_reductor(std::forward<Arg>(arg));
        return *this;
    }

    constexpr auto get() const& -> const state_type&
    {
        return m_reductor.state;
    }

    constexpr auto get() & -> state_type&
    {
        return m_reductor.state;
    }

    constexpr auto get() && -> state_type&&
    {
        return std::move(m_reductor.state);
    }
};

struct out_fn
{
    template <class State, class Reducer>
    constexpr auto operator()(reductor_t<State, Reducer> reductor) const -> output_iterator_t<State, Reducer>
    {
        return { std::move(reductor) };
    }
};

struct reduce_fn
{
    template <class State, class Reducer, class Range_0>
    constexpr auto operator()(reductor_t<State, Reducer> reductor, Range_0&& range_0) const -> State
    {
        auto it_0 = std::begin(range_0);
        const auto end_0 = std::end(range_0);
        for (; it_0 != end_0; ++it_0)
        {
            if (!reductor(*it_0))
            {
                break;
            }
        }
        return reductor.state;
    }

    template <class State, class Reducer, class Range_0, class Range_1>
    constexpr auto operator()(reductor_t<State, Reducer> reductor, Range_0&& range_0, Range_1&& range_1) const -> State
    {
        auto it_0 = std::begin(range_0);
        auto it_1 = std::begin(range_1);
        const auto end_0 = std::end(range_0);
        const auto end_1 = std::end(range_1);
        for (; it_0 != end_0 && it_1 != end_1; ++it_0, ++it_1)
        {
            if (!reductor(*it_0, *it_1))
            {
                break;
            }
        }
        return reductor.state;
    }

    template <class State, class Reducer, class Range_0, class Range_1, class Range_2>
    constexpr auto operator()(
        reductor_t<State, Reducer> reductor, Range_0&& range_0, Range_1&& range_1, Range_2&& range_2) const -> State
    {
        auto it_0 = std::begin(range_0);
        auto it_1 = std::begin(range_1);
        auto it_2 = std::begin(range_2);
        const auto end_0 = std::end(range_0);
        const auto end_1 = std::end(range_1);
        const auto end_2 = std::end(range_2);
        for (; it_0 != end_0 && it_1 != end_1 && it_2 != end_2; ++it_0, ++it_1, ++it_2)
        {
            if (!reductor(*it_0, *it_1, *it_2))
            {
                break;
            }
        }
        return reductor.state;
    }
};

constexpr inline auto reduce = reduce_fn{};

template <class Range>
using range_value_t = typename std::decay_t<Range>::value_type;

struct from_fn
{
    template <class Range_0>
    constexpr auto operator()(Range_0&& range_0) const -> generator_t<range_value_t<Range_0>>
    {
        using generator_type = generator_t<range_value_t<Range_0>>;
        return generator_type(
            [&](auto yield)
            {
                auto it_0 = std::begin(range_0);
                const auto end_0 = std::end(range_0);
                for (; it_0 != end_0; ++it_0)
                {
                    if (!yield(*it_0))
                    {
                        break;
                    }
                }
            });
    }

    template <class Range_0, class Range_1>
    constexpr auto operator()(Range_0&& range_0, Range_1&& range_1) const
        -> generator_t<range_value_t<Range_0>, range_value_t<Range_1>>
    {
        using generator_type = generator_t<range_value_t<Range_0>, range_value_t<Range_1>>;
        return generator_type(
            [&](auto yield)
            {
                auto it_0 = std::begin(range_0);
                auto it_1 = std::begin(range_1);
                const auto end_0 = std::end(range_0);
                const auto end_1 = std::end(range_1);
                for (; it_0 != end_0 && it_1 != end_1; ++it_0, ++it_1)
                {
                    if (!yield(*it_0, *it_1))
                    {
                        break;
                    }
                }
            });
    }

    template <class Range_0, class Range_1, class Range_2>
    constexpr auto operator()(Range_0&& range_0, Range_1&& range_1, Range_2&& range_2) const
        -> generator_t<range_value_t<Range_0>, range_value_t<Range_1>, range_value_t<Range_2>>

    {
        using generator_type = generator_t<range_value_t<Range_0>, range_value_t<Range_1>, range_value_t<Range_2>>;
        return generator_type(
            [&](auto yield)
            {
                auto it_0 = std::begin(range_0);
                auto it_1 = std::begin(range_1);
                auto it_2 = std::begin(range_2);
                const auto end_0 = std::end(range_0);
                const auto end_1 = std::end(range_1);
                const auto end_2 = std::end(range_2);
                for (; it_0 != end_0 && it_1 != end_1 && it_2 != end_2; ++it_0, ++it_1, ++it_2)
                {
                    if (!yield(*it_0, *it_1, *it_2))
                    {
                        break;
                    }
                }
            });
    }
};

struct chain_fn
{
    template <class Range_0, class Range_1>
    constexpr auto operator()(Range_0&& range_0, Range_1&& range_1) const
        -> generator_t<std::common_type_t<range_value_t<Range_0>, range_value_t<Range_1>>>
    {
        using generator_type = generator_t<std::common_type_t<range_value_t<Range_0>, range_value_t<Range_1>>>;
        return generator_type(
            [&](auto yield)
            {
                for (auto&& item : range_0)
                {
                    if (!yield(item))
                    {
                        return;
                    }
                }
                for (auto&& item : range_1)
                {
                    if (!yield(item))
                    {
                        return;
                    }
                }
            });
    }
};

struct range_fn
{
    template <class T>
    constexpr auto operator()(T lower, T upper) const -> generator_t<T>
    {
        using generator_type = generator_t<T>;
        return generator_type(
            [=](auto yield)
            {
                for (T value = lower; value < upper; ++value)
                {
                    if (!yield(value))
                    {
                        return;
                    }
                }
            });
    }

    template <class T>
    constexpr auto operator()(T upper) const -> generator_t<T>
    {
        return (*this)(T{}, upper);
    }
};

struct iota_fn
{
    template <class T = std::ptrdiff_t>
    constexpr auto operator()(T lower = {}) const -> generator_t<T>
    {
        using generator_type = generator_t<T>;
        return generator_type(
            [=](auto yield)
            {
                T value = lower;
                while (true)
                {
                    if (!yield(value))
                    {
                        return;
                    }
                    ++value;
                }
            });
    }
};

struct read_lines_fn
{
    static auto read_line(std::istream& in, std::string& line) -> std::istream&
    {
        line.clear();
        char ch;

        while (true)
        {
            if (!in.get(ch))
            {
                if (!line.empty())
                {
                    in.clear(in.rdstate() & ~std::ios::failbit);
                    in.setstate(std::ios::eofbit);
                    return in;
                }

                in.setstate(std::ios::failbit);
                return in;
            }

            if (ch == '\n')
            {
                return in;
            }

            if (ch == '\r')
            {
                if (in.peek() == '\n')
                {
                    in.get();
                }

                return in;
            }

            line.push_back(ch);
        }
    }

    auto operator()(std::istream& is) const -> generator_t<std::string>
    {
        using generator_type = generator_t<std::string>;
        return generator_type(
            [&](auto yield)
            {
                std::string line;
                while (read_line(is, line))
                {
                    if (!yield(line))
                    {
                        return;
                    }
                }
            });
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
        constexpr auto operator()(bool& state, Args&&... args) const -> bool
        {
            state = state && std::invoke(m_pred, std::forward<Args>(args)...);
            return state;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reductor_t<bool, reducer_t<std::decay_t<Pred>>>
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
        constexpr auto operator()(bool& state, Args&&... args) const -> bool
        {
            state = state || std::invoke(m_pred, std::forward<Args>(args)...);
            return !state;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reductor_t<bool, reducer_t<std::decay_t<Pred>>>
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
        constexpr auto operator()(bool& state, Args&&... args) const -> bool
        {
            state = state && !std::invoke(m_pred, std::forward<Args>(args)...);
            return state;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> reductor_t<bool, reducer_t<std::decay_t<Pred>>>
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            if (std::invoke(m_pred, args...))
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return true;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> transducer_t<reducer_t, std::decay_t<Pred>>
    {
        return { std::forward<Pred>(pred) };
    }
};

struct filter_indexed_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;
        mutable std::ptrdiff_t m_index = 0;

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            if (std::invoke(m_pred, m_index++, std::forward<Args>(args)...))
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return true;
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
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

struct transform_indexed_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;
        mutable std::ptrdiff_t m_index = 0;

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            return m_next_reducer(state, std::invoke(m_func, m_index++, std::forward<Args>(args)...));
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            std::invoke(m_func, args...);
            return m_next_reducer(state, std::forward<Args>(args)...);
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> transducer_t<reducer_t, std::decay_t<Func>>
    {
        return { std::forward<Func>(func) };
    }
};

struct inspect_indexed_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;
        mutable std::ptrdiff_t m_index = 0;

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            std::invoke(m_func, m_index++, args...);
            return m_next_reducer(state, std::forward<Args>(args)...);
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            if (auto res = std::invoke(m_func, std::forward<Args>(args)...))
            {
                return m_next_reducer(state, *std::move(res));
            }
            return true;
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> transducer_t<reducer_t, std::decay_t<Func>>
    {
        return { std::forward<Func>(func) };
    }
};

struct transform_maybe_indexed_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;
        mutable std::ptrdiff_t m_index = 0;

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            if (auto res = std::invoke(m_func, m_index++, std::forward<Args>(args)...))
            {
                return m_next_reducer(state, *std::move(res));
            }
            return true;
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> transducer_t<reducer_t, std::decay_t<Func>>
    {
        return { std::forward<Func>(func) };
    }
};

struct unpack_fn
{
    template <class Reducer>
    struct reducer_t
    {
        Reducer m_next_reducer;

        template <class State, class Arg>
        constexpr auto operator()(State& state, Arg&& arg) const -> bool
        {
            return std::apply(
                [&](auto&&... args) { return m_next_reducer(state, std::forward<decltype(args)>(args)...); },
                std::forward<Arg>(arg));
        }
    };

    constexpr auto operator()() const -> transducer_t<reducer_t, void>
    {
        return {};
    }
};

struct project_fn
{
    template <class Reducer, class Funcs>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Funcs m_funcs;

        template <class State, class Arg>
        constexpr auto operator()(State& state, Arg&& arg) const -> bool
        {
            return std::apply(
                [&](auto&&... funcs) { return m_next_reducer(state, std::invoke(funcs, std::forward<Arg>(arg))...); },
                m_funcs);
        }
    };

    template <class... Funcs>
    constexpr auto operator()(Funcs&&... funcs) const -> transducer_t<reducer_t, std::tuple<std::decay_t<Funcs>...>>
    {
        return { std::make_tuple(std::forward<Funcs>(funcs)...) };
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            m_done |= !std::invoke(m_pred, args...);
            if (!m_done)
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return false;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> transducer_t<reducer_t, std::decay_t<Pred>>
    {
        return { std::forward<Pred>(pred) };
    }
};

struct take_while_indexed_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;

        mutable bool m_done = false;
        mutable std::ptrdiff_t m_index = 0;

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            m_done |= !std::invoke(m_pred, m_index++, args...);
            if (!m_done)
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return false;
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            m_done |= !std::invoke(m_pred, args...);
            if (m_done)
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return true;
        }
    };

    template <class Pred>
    constexpr auto operator()(Pred&& pred) const -> transducer_t<reducer_t, std::decay_t<Pred>>
    {
        return { std::forward<Pred>(pred) };
    }
};

struct drop_while_indexed_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;

        mutable bool m_done = false;
        mutable std::ptrdiff_t m_index = 0;

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            m_done |= !std::invoke(m_pred, m_index++, args...);
            if (m_done)
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return true;
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            if (m_count-- > 0)
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return false;
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            if (m_count-- <= 0)
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return true;
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
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            if (m_index++ % m_count == 0)
            {
                return m_next_reducer(state, std::forward<Args>(args)...);
            }
            return true;
        }
    };

    constexpr auto operator()(std::ptrdiff_t count) const -> transducer_t<reducer_t, std::ptrdiff_t>
    {
        return { count };
    }
};

struct join_fn
{
    template <class Reducer>
    struct reducer_t
    {
        Reducer m_next_reducer;

        template <class State, class Arg>
        constexpr auto operator()(State& state, Arg&& arg) const -> bool
        {
            for (auto&& item : arg)
            {
                if (!m_next_reducer(state, std::forward<decltype(item)>(item)))
                {
                    return false;
                }
            }
            return true;
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
        constexpr auto operator()(State& state, Arg&& arg) const -> bool
        {
            if (!m_first)
            {
                if (!m_next_reducer(state, m_separator))
                {
                    return false;
                }
            }
            m_first = false;
            return m_next_reducer(state, std::forward<Arg>(arg));
        }
    };

    template <class Separator>
    constexpr auto operator()(Separator&& separator) const -> transducer_t<reducer_t, std::decay_t<Separator>>
    {
        return { std::forward<Separator>(separator) };
    }
};

struct ignoring_reducer_t
{
    template <class State, class... Args>
    constexpr auto operator()(State&, Args&&...) const -> bool
    {
        return true;
    }
};

struct copy_to_reducer_t
{
    template <class State, class Arg>
    constexpr auto operator()(State& state, Arg&& arg) const -> bool
    {
        *state = std::forward<Arg>(arg);
        ++state;
        return true;
    }
};

struct deref_fn
{
    template <class T>
    constexpr auto operator()(T& arg) const -> T&
    {
        return arg;
    }

    template <class T>
    constexpr auto operator()(T* arg) const -> T&
    {
        return *arg;
    }

    template <class T>
    constexpr auto operator()(std::reference_wrapper<T> arg) const -> T&
    {
        return (*this)(arg.get());
    }
};

static constexpr inline auto deref = deref_fn{};

struct push_back_reducer_t
{
    template <class State, class Arg>
    constexpr auto operator()(State& state, Arg&& arg) const -> bool
    {
        deref(state).push_back(std::forward<Arg>(arg));
        return true;
    }
};

struct copy_to_fn
{
    template <class Out>
    constexpr auto operator()(Out out) const -> reductor_t<Out, copy_to_reducer_t>
    {
        return { std::move(out), copy_to_reducer_t{} };
    }
};

struct push_back_fn
{
    template <class Container>
    constexpr auto operator()(Container& container) const
        -> reductor_t<std::reference_wrapper<Container>, push_back_reducer_t>
    {
        return { container, push_back_reducer_t{} };
    }
};

struct into_fn
{
    template <class Container>
    constexpr auto operator()(Container&& container) const -> reductor_t<std::decay_t<Container>, push_back_reducer_t>
    {
        return { std::forward<Container>(container), push_back_reducer_t{} };
    }
};

struct partition_fn
{
    template <class Pred, class OnTrueReducer, class OnFalseReducer>
    struct reducer_t
    {
        Pred m_pred;
        std::tuple<OnTrueReducer, OnFalseReducer> m_reducers;
        mutable std::bitset<2> m_done{};

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            if (std::invoke(m_pred, args...))
            {
                m_done[0] = !std::get<0>(m_reducers)(state.first, std::forward<Args>(args)...);
            }
            else
            {
                m_done[1] = !std::get<1>(m_reducers)(state.second, std::forward<Args>(args)...);
            }
            return !m_done.all();
        }
    };

    template <class Pred, class S0, class R0, class S1, class R1>
    constexpr auto operator()(Pred&& pred, reductor_t<S0, R0> on_true_reducer, reductor_t<S1, R1> on_false_reducer) const
        -> reductor_t<std::pair<S0, S1>, reducer_t<std::decay_t<Pred>, R0, R1>>
    {
        return { { std::move(on_true_reducer.state), std::move(on_false_reducer.state) },
                 { std::forward<Pred>(pred), { std::move(on_true_reducer.reducer), std::move(on_false_reducer.reducer) } } };
    }
};

struct fork_fn
{
    template <class... Reducers>
    struct reducer_t
    {
        std::tuple<Reducers...> m_reducers;
        mutable std::bitset<sizeof...(Reducers)> m_done{};

        template <std::size_t N, class State, class... Args>
        void call(State& state, Args&&... args) const
        {
            m_done[N] = !std::get<N>(m_reducers)(std::get<N>(state), args...);
            if constexpr (N + 1 < sizeof...(Reducers))
            {
                call<N + 1>(state, args...);
            }
        }

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            call<0>(state, args...);
            return !m_done.all();
        }
    };

    template <class... Reducers>
    constexpr auto operator()(Reducers... reducers) const
        -> reductor_t<std::tuple<typename Reducers::state_type...>, reducer_t<typename Reducers::reducer_type...>>
    {
        return { { std::move(reducers.state)... },
                 {
                     { std::move(reducers.reducer)... },
                 } };
    }
};

struct sum_fn
{
    template <class T>
    constexpr auto operator()(T value) const -> reductor_t<T, to_reducer_fn::adapter_t<std::plus<>>>
    {
        return reductor_t{ value, to_reducer_fn{}(std::plus<>{}) };
    }
};

struct for_each_fn
{
    template <class Func>
    struct reducer_t
    {
        Func m_func;

        template <class... Args>
        constexpr auto operator()(std::ptrdiff_t& state, Args&&... args) const -> bool
        {
            std::invoke(m_func, std::forward<Args>(args)...);
            ++state;
            return true;
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> reductor_t<std::ptrdiff_t, reducer_t<std::decay_t<Func>>>
    {
        return { 0, { std::forward<Func>(func) } };
    }
};

struct for_each_indexed_fn
{
    template <class Func>
    struct reducer_t
    {
        Func m_func;

        template <class... Args>
        constexpr auto operator()(std::ptrdiff_t& state, Args&&... args) const -> bool
        {
            std::invoke(m_func, state++, std::forward<Args>(args)...);
            return true;
        }
    };

    template <class Func>
    constexpr auto operator()(Func&& func) const -> reductor_t<std::ptrdiff_t, reducer_t<std::decay_t<Func>>>
    {
        return { 0, { std::forward<Func>(func) } };
    }
};

struct accumulate_fn
{
    template <class Func>
    struct reducer_t
    {
        Func m_func;

        template <class State, class... Args>
        constexpr auto operator()(State& state, Args&&... args) const -> bool
        {
            state = std::invoke(m_func, std::move(state), std::forward<Args>(args)...);
            return true;
        }
    };

    template <class State, class Func>
    constexpr auto operator()(State state, Func&& func) const -> reductor_t<State, reducer_t<std::decay_t<Func>>>
    {
        return { std::move(state), { std::forward<Func>(func) } };
    }
};

}  // namespace detail

constexpr inline auto reduce = detail::reduce_fn{};
constexpr inline auto out = detail::out_fn{};
constexpr inline auto from = detail::from_fn{};
constexpr inline auto chain = detail::chain_fn{};
constexpr inline auto range = detail::range_fn{};
constexpr inline auto iota = detail::iota_fn{};
constexpr inline auto read_lines = detail::read_lines_fn{};

constexpr inline auto to_reducer = detail::to_reducer_fn{};

static constexpr inline auto all_of = detail::all_of_fn{};
static constexpr inline auto any_of = detail::any_of_fn{};
static constexpr inline auto none_of = detail::none_of_fn{};

static constexpr inline auto transform = detail::transform_fn{};
static constexpr inline auto transform_indexed = detail::transform_indexed_fn{};

static constexpr inline auto filter = detail::filter_fn{};
static constexpr inline auto filter_indexed = detail::filter_indexed_fn{};

static constexpr inline auto inspect = detail::inspect_fn{};
static constexpr inline auto inspect_indexed = detail::inspect_indexed_fn{};

static constexpr inline auto transform_maybe = detail::transform_maybe_fn{};
static constexpr inline auto transform_maybe_indexed = detail::transform_maybe_indexed_fn{};

static constexpr inline auto unpack = detail::unpack_fn{}();
static constexpr inline auto project = detail::project_fn{};

static constexpr inline auto take_while = detail::take_while_fn{};
static constexpr inline auto take_while_indexed = detail::take_while_indexed_fn{};

static constexpr inline auto drop_while = detail::drop_while_fn{};
static constexpr inline auto drop_while_indexed = detail::drop_while_indexed_fn{};

static constexpr inline auto take = detail::take_fn{};
static constexpr inline auto drop = detail::drop_fn{};
static constexpr inline auto stride = detail::stride_fn{};

static constexpr inline auto join = detail::join_fn{}();
static constexpr inline auto intersperse = detail::intersperse_fn{};

static constexpr inline auto dev_null = reductor_t{ 0, detail::ignoring_reducer_t{} };

static constexpr inline auto partition = detail::partition_fn{};
static constexpr inline auto fork = detail::fork_fn{};

static constexpr inline auto copy_to = detail::copy_to_fn{};
static constexpr inline auto push_back = detail::push_back_fn{};
static constexpr inline auto into = detail::into_fn{};

static constexpr inline auto for_each = detail::for_each_fn{};
static constexpr inline auto for_each_indexed = detail::for_each_indexed_fn{};

static constexpr inline auto accumulate = detail::accumulate_fn{};

static constexpr inline auto count = reductor_t{ std::size_t{ 0 },
                                                 [](std::size_t& state, auto&&...) -> bool
                                                 {
                                                     state += 1;
                                                     return true;
                                                 } };

static constexpr inline auto sum = detail::sum_fn{};

}  // namespace TRX_NAMESPACE
