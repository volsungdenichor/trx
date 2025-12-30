#pragma once

#ifndef TRX_NAMESPACE
#define TRX_NAMESPACE trx
#endif  // TRX_NAMESPACE

#include <bitset>
#include <functional>
#include <tuple>
#include <type_traits>

namespace TRX_NAMESPACE
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
    bool operator()(Args&&... args)
    {
        return std::invoke(reducer, state, std::forward<Args>(args)...);
    }

    constexpr const state_type& get() const&
    {
        return state;
    }

    constexpr state_type& get() &
    {
        return state;
    }

    constexpr state_type&& get() &&
    {
        return std::move(state);
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

template <class Signature>
struct function_ref;

template <class Ret, class... Args>
struct function_ref<Ret(Args...)>
{
    using return_type = Ret;
    using func_type = return_type (*)(void*, Args...);

    void* m_obj;
    func_type m_func;

    template <class Func>
    constexpr function_ref(Func&& func)
        : m_obj{ const_cast<void*>(reinterpret_cast<const void*>(std::addressof(func))) }
        , m_func{ [](void* obj, Args... args) -> return_type
                  {
                      if constexpr (std::is_void_v<return_type>)
                      {
                          std::invoke(*static_cast<std::add_pointer_t<Func>>(obj), std::forward<Args>(args)...);
                      }
                      else
                      {
                          return std::invoke(*static_cast<std::add_pointer_t<Func>>(obj), std::forward<Args>(args)...);
                      }
                  } }
    {
    }

    template <class... CallArgs>
    constexpr return_type operator()(CallArgs&&... args) const
    {
        return m_func(m_obj, std::forward<CallArgs>(args)...);
    }
};

template <class... Args>
struct generator_t : public std::function<void(function_ref<bool(Args...)>)>
{
    using consumer_type = function_ref<bool(Args...)>;
    using base_t = std::function<void(function_ref<bool(Args...)>)>;

    using base_t::base_t;
};

template <class... Args, class State, class Reducer>
constexpr State operator|=(generator_t<Args...> generator, reducer_proxy_t<State, Reducer> reducer)
{
    generator(typename generator_t<Args...>::consumer_type{ reducer });
    return reducer.state;
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
        constexpr bool operator()(State& state, Args&&... args) const
        {
            state = std::invoke(m_reducer, std::move(state), std::forward<Args>(args)...);
            return true;
        }
    };

    template <class Reducer>
    constexpr auto operator()(Reducer&& reducer) const -> impl_t<std::decay_t<Reducer>>
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

    reducer_proxy_t<State, Reducer> m_reducer_proxy;

    constexpr output_iterator_t& operator*()
    {
        return *this;
    }

    constexpr output_iterator_t& operator++()
    {
        return *this;
    }

    constexpr output_iterator_t& operator++(int)
    {
        return *this;
    }

    template <class Arg>
    constexpr output_iterator_t& operator=(Arg&& arg)
    {
        m_reducer_proxy(std::forward<Arg>(arg));
        return *this;
    }

    constexpr const state_type& get() const&
    {
        return m_reducer_proxy.state;
    }

    constexpr state_type& get() &
    {
        return m_reducer_proxy.state;
    }

    constexpr state_type&& get() &&
    {
        return std::move(m_reducer_proxy.state);
    }
};

struct out_fn
{
    template <class State, class Reducer>
    constexpr auto operator()(reducer_proxy_t<State, Reducer> reducer) const -> output_iterator_t<State, Reducer>
    {
        return { std::move(reducer) };
    }
};

struct reduce_fn
{
    template <class State, class Reducer, class Range_0>
    constexpr auto operator()(reducer_proxy_t<State, Reducer> reducer, Range_0&& range_0) const -> State
    {
        auto it_0 = std::begin(range_0);
        const auto end_0 = std::end(range_0);
        for (; it_0 != end_0; ++it_0)
        {
            if (!reducer.reducer(reducer.state, *it_0))
            {
                break;
            }
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
            if (!reducer.reducer(reducer.state, *it_0, *it_1))
            {
                break;
            }
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
            if (!reducer.reducer(reducer.state, *it_0, *it_1, *it_2))
            {
                break;
            }
        }
        return reducer.state;
    }
};

constexpr inline auto reduce = reduce_fn{};

struct from_fn
{
    template <class Range_0, class State, class Reducer>
    constexpr auto operator()(Range_0&& range_0, reducer_proxy_t<State, Reducer> reducer) const -> State
    {
        return reduce(std::move(reducer), std::forward<Range_0>(range_0));
    }

    template <class Range_0, class Range_1, class State, class Reducer>
    constexpr auto operator()(Range_0&& range_0, Range_1&& range_1, reducer_proxy_t<State, Reducer> reducer) const -> State
    {
        return reduce(std::move(reducer), std::forward<Range_0>(range_0), std::forward<Range_1>(range_1));
    }

    template <class Range_0, class Range_1, class Range_2, class State, class Reducer>
    constexpr auto operator()(
        Range_0&& range_0, Range_1&& range_1, Range_2&& range_2, reducer_proxy_t<State, Reducer> reducer) const -> State
    {
        return reduce(
            std::move(reducer),
            std::forward<Range_0>(range_0),
            std::forward<Range_1>(range_1),
            std::forward<Range_2>(range_2));
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
        constexpr bool operator()(bool& state, Args&&... args) const
        {
            state = state && std::invoke(m_pred, std::forward<Args>(args)...);
            return state;
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
        constexpr bool operator()(bool& state, Args&&... args) const
        {
            state = state || std::invoke(m_pred, std::forward<Args>(args)...);
            return !state;
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
        constexpr bool operator()(bool& state, Args&&... args) const
        {
            state = state && !std::invoke(m_pred, std::forward<Args>(args)...);
            return state;
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
        constexpr bool operator()(State& state, Args&&... args) const
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

struct transform_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;

        template <class State, class... Args>
        constexpr bool operator()(State& state, Args&&... args) const
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
        constexpr bool operator()(State& state, Args&&... args) const
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

struct transform_maybe_fn
{
    template <class Reducer, class Func>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Func m_func;

        template <class State, class... Args>
        constexpr bool operator()(State& state, Args&&... args) const
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

struct take_while_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;

        mutable bool m_done = false;

        template <class State, class... Args>
        constexpr bool operator()(State& state, Args&&... args) const
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

struct drop_while_fn
{
    template <class Reducer, class Pred>
    struct reducer_t
    {
        Reducer m_next_reducer;
        Pred m_pred;

        mutable bool m_done = false;

        template <class State, class... Args>
        constexpr bool operator()(State& state, Args&&... args) const
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

struct take_fn
{
    template <class Reducer, class>
    struct reducer_t
    {
        Reducer m_next_reducer;
        mutable std::ptrdiff_t m_count;

        template <class State, class... Args>
        constexpr bool operator()(State& state, Args&&... args) const
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
        constexpr bool operator()(State& state, Args&&... args) const
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
        constexpr bool operator()(State& state, Args&&... args) const
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
        constexpr bool operator()(State& state, Arg&& arg) const
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
        constexpr bool operator()(State& state, Arg&& arg) const
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
    bool operator()(State&, Args&&...) const
    {
        return true;
    }
};

struct copy_to_reducer_t
{
    template <class State, class Arg>
    bool operator()(State& state, Arg&& arg) const
    {
        *state = std::forward<Arg>(arg);
        ++state;
        return true;
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
    bool operator()(State& state, Arg&& arg) const
    {
        deref(state).push_back(std::forward<Arg>(arg));
        return true;
    }
};

struct copy_to_fn
{
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

struct partition_fn
{
    template <class Pred, class OnTrueReducer, class OnFalseReducer>
    struct reducer_t
    {
        Pred m_pred;
        std::tuple<OnTrueReducer, OnFalseReducer> m_reducers;
        mutable std::bitset<2> m_done{};

        template <class State, class... Args>
        constexpr bool operator()(State& state, Args&&... args) const
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
    constexpr auto operator()(Pred&& pred, reducer_proxy_t<S0, R0> on_true_reducer, reducer_proxy_t<S1, R1> on_false_reducer)
        const -> reducer_proxy_t<std::pair<S0, S1>, reducer_t<std::decay_t<Pred>, R0, R1>>
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
        bool operator()(State& state, Args&&... args) const
        {
            call<0>(state, args...);
            return !m_done.all();
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

struct sum_fn
{
    template <class T>
    constexpr auto operator()(T value) const
    {
        return reducer_proxy_t{ value, to_reducer_fn{}(std::plus<>{}) };
    }
};

}  // namespace detail

constexpr inline auto reduce = detail::reduce_fn{};
constexpr inline auto out = detail::out_fn{};
constexpr inline auto from = detail::from_fn{};
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

static constexpr inline auto dev_null = reducer_proxy_t{ 0, detail::ignoring_reducer_t{} };

static constexpr inline auto partition = detail::partition_fn{};
static constexpr inline auto fork = detail::fork_fn{};

static constexpr inline auto copy_to = detail::copy_to_fn{};
static constexpr inline auto push_back = detail::push_back_fn{};
static constexpr inline auto into = detail::into_fn{};

static constexpr inline auto count = reducer_proxy_t{ std::size_t{ 0 },
                                                      [](std::size_t& state, auto&&...) -> bool
                                                      {
                                                          state += 1;
                                                          return true;
                                                      } };

static constexpr inline auto sum = detail::sum_fn{};

}  // namespace TRX_NAMESPACE
