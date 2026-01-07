# trx

## usage

```
generator |= transducer* |= reductor
```

Single input range is treated as a generator:

```
range |= transducer* |= reductor
```

### reductor
Aggregated `State` and state mutating function - the actual reducer with signature `(State&, Args&&...) -> bool`. Iteration is terminated, when the function returns `false`.

### transducer
A function which transforms a reductor into another reductor. Chaining multiple transducers and a final reductor creates a single reductor.

### generator
Function which produces values passed on to the reductor. It's implemented by returning a callable object:
```
auto generate() -> generator_t<Types...> {
    return [](typename generator_t<Types...>::yield_fn yield)
    {
        // ...
        if (!yield(args...))
        {
            return;
        }
        // ...
    };
}
```
calling the `yield` function on arguments will push them to the reductor.

## transducers

### transform
Applies a function to each item and passes the result to the next reductor.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::vector<int> result = input
    |= trx::transform([](int x) { return x * 2; })
    |= trx::into(std::vector<int>{});
// result: {2, 4, 6, 8, 10}
```

### transform_indexed
Applies a function to each item along with its index and passes the result to the next reducer.

```cpp
std::vector<std::string> input = {"a", "b", "c"};
std::vector<std::string> result = input
    |= trx::transform_indexed([](std::ptrdiff_t idx, const std::string& s) { return std::to_string(idx) + ":" + s; })
    |= trx::into(std::vector<std::string>{});
// result: {"0:a", "1:b", "2:c"}
```

### filter
Only passes items that satisfy a predicate to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 6};
std::vector<int> result = input
    |= trx::filter([](int x) { return x % 2 == 0; })
    |= trx::into(std::vector<int>{});
// result: {2, 4, 6}
```

### filter_indexed
Only passes items that satisfy an index-aware predicate to the next reducer.

```cpp
std::vector<int> input = {10, 20, 30, 40, 50};
std::vector<int> result = input
    |= trx::filter_indexed([](std::ptrdiff_t idx, int x) { return idx % 2 == 0; })
    |= trx::into(std::vector<int>{});
// result: {10, 30, 50}
```

### inspect
Executes a function for side effects on each item without modifying the item passed to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3};
std::vector<int> result = input
    |= trx::inspect([](int x) { std::cout << x << " "; })
    |= trx::into(std::vector<int>{});
// Prints: 1 2 3
// result: {1, 2, 3}
```

### inspect_indexed
Executes a function for side effects on each item and its index without modifying the item passed to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3};
std::vector<int> result = input
    |= trx::inspect_indexed([](std::ptrdiff_t idx, int x) { std::cout << "[" << idx << "]=" << x << " "; })
    |= trx::into(std::vector<int>{});
// Prints: [0]=1 [1]=2 [2]=3
// result: {1, 2, 3}
```

### transform_maybe
Applies a function that returns an optional value, only passing non-empty results to the next reducer.

```cpp
std::vector<std::string> input = {"1", "2", "abc", "4"};
std::vector<int> result = input
    |= trx::transform_maybe([](const std::string& s) -> std::optional<int> {
        try {
            return std::stoi(s);
        }
        catch (...) {
            return std::nullopt;
        }
    })
    |= trx::into(std::vector<int>{});
// result: {1, 2, 4} (skips "abc")
```

### transform_maybe_indexed
Applies an index-aware function that returns an optional value, only passing non-empty results to the next reducer.

```cpp
std::vector<std::string> input = {"1", "2", "abc", "4"};
std::vector<int> result = input
    |= trx::transform_maybe_indexed([](std::ptrdiff_t idx, const std::string& s) -> std::optional<int> {
        try {
            return std::stoi(s) * idx;  // multiply by index
        }
        catch (...) {
            return std::nullopt;
        }
    })
    |= trx::into(std::vector<int>{});
// result: {0, 2, 12} (0*0, 2*1, skips "abc", 4*3)
```

### take_while
Passes items to the next reducer while a predicate is true, stopping once it becomes false.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 3, 2, 1};
std::vector<int> result = input
    |= trx::take_while([](int x) { return x < 4; })
    |= trx::into(std::vector<int>{});
// result: {1, 2, 3}
```

### take_while_indexed
Passes items to the next reducer while an index-aware predicate is true, stopping once it becomes false.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 3, 2, 1};
std::vector<int> result = input
    |= trx::take_while_indexed([](std::ptrdiff_t idx, int x) { return idx < 3; })
    |= trx::into(std::vector<int>{});
// result: {1, 2, 3}
```

### drop_while
Skips items while a predicate is true, then passes all remaining items to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 3, 2, 1};
std::vector<int> result = input
    |= trx::drop_while([](int x) { return x < 4; })
    |= trx::into(std::vector<int>{});
// result: {4, 5, 3, 2, 1}
```

### drop_while_indexed
Skips items while an index-aware predicate is true, then passes all remaining items to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 3, 2, 1};
std::vector<int> result = input
    |= trx::drop_while_indexed([](std::ptrdiff_t idx, int x) { return idx < 3; })
    |= trx::into(std::vector<int>{});
// result: {4, 5, 3, 2, 1}
```

### take
Limits the number of items passed to the next reducer to the specified count.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::vector<int> result = input
    |= trx::take(3)
    |= trx::into(std::vector<int>{});
// result: {1, 2, 3}
```

### drop
Skips the first N items, passing only items after that count to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::vector<int> result = input
    |= trx::drop(2)
    |= trx::into(std::vector<int>{});
// result: {3, 4, 5}
```

### stride
Passes every Nth item to the next reducer based on the specified stride value.

```cpp
std::vector<int> input = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
std::vector<int> result = input
    |= trx::stride(2)
    |= trx::into(std::vector<int>{});
// result: {0, 2, 4, 6, 8}
```

### join
Flattens nested ranges by iterating through each element in the input container and passing individual items to the next reducer.

```cpp
std::vector<std::vector<int>> input = {{1, 2}, {3, 4}, {5, 6}};
std::vector<int> result = input
    |= trx::join
    |= trx::into(std::vector<int>{});
// result: {1, 2, 3, 4, 5, 6}
```

### intersperse

```cpp
std::string input = "ABCD";
std::string result = input
    |= trx::intersperse(',')
    |= trx::into(std::string{});
// result: "A,B,C,D"
```

### all_of
Tests whether all items satisfy a predicate condition.

```cpp
std::vector<int> input = {2, 4, 6, 8};
bool result = input |= trx::all_of([](int x) { return x % 2 == 0; });
// result: true (all are even)
```

### any_of
Tests whether at least one item satisfies a predicate condition.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
bool result = input |= trx::any_of([](int x) { return x % 2 == 0; });
// result: true (2 and 4 are even)
```

### none_of
Tests whether no items satisfy a predicate condition.

```cpp
std::vector<int> input = {1, 3, 5, 7};
bool result = input |= trx::none_of([](int x) { return x % 2 == 0; });
// result: true (none are even)
```

### unpack
Unpacks tuple-like objects (tuples, pairs, arrays) and passes their elements as separate arguments to the next reducer. Uses `std::apply` internally to expand the tuple elements, allowing downstream transducers to work with individual components instead of the packed structure.

```cpp
std::vector<std::tuple<int, int, char>> input = {{1, 2, 'a'}, {2, 3, 'b'}};
std::vector<std::string> result = input
    |= trx::unpack
    |= trx::transform([](int a, int b, char c) { return std::to_string(10 * a + b) + c;  })
    |= trx::into(std::vector<std::string>{});
// result: {"12a", "23b"};
```

## reductors

### dev_null
A reducer that discards all items, useful for situations where you want to run transducers for side effects only.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
input |= trx::inspect([](int x) { std::cout << x << " "; }) |= trx::dev_null;
// Prints: 1 2 3 4 5
```

### partition
Distributes items into two separate reductors based on a predicate condition, creating a pair of results.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto [first, second] = input
    |= trx::partition(
        [](int x) { return x % 2 == 0; },
        trx::into(std::vector<int>{}),
        trx::into(std::vector<int>{}));
// first: {2, 4} (even numbers)
// second: {1, 3, 5} (odd numbers)
```

### fork
Sends the same item to multiple reductors simultaneously, collecting results into a tuple.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto [first, second] = input
    |= trx::fork(
        trx::into(std::vector<int>{}),
        trx::count);
// first: {1, 2, 3, 4, 5}
// second: 5
```

### copy_to
Copies items to an output iterator, advancing the iterator with each element.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::vector<int> dest(5);
input |= trx::copy_to(dest.begin());
// dest: {1, 2, 3, 4, 5}
```

### push_back
Appends items to the end of a container.

```cpp
std::vector<int> input = {1, 2, 3};
std::vector<int> result;
input |= trx::push_back(result);
// result: {1, 2, 3}
```

### into
Creates a reductor that builds a container by appending items to it using `push_back` semantics.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::vector<int> result = input |= trx::into(std::vector<int>{});
// result: {1, 2, 3, 4, 5}
```

### count
Counts the total number of items processed by the reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::size_t result = input |= trx::count;
// result: 5
```

### for_each
Executes a function on each item, useful for performing side effects.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::vector<int> result;
input |= trx::for_each([&result](int x) { result.push_back(x * 2); });
// result: {2, 4, 6, 8, 10}
```

### for_each_indexed
Executes a function on each item along with its index, useful for index-aware side effects.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::vector<int> result;
input |= trx::for_each_indexed([&result](std::ptrdiff_t idx, int x) {
    result.push_back(x * 2 + 100 * idx);
});
// result: {2, 104, 206, 308, 410}
```

### accumulate
Reduces items to a single value by repeatedly applying a function to an accumulated state and each item.
Compliant with std::accumulate but enables multiple input ranges.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
int result = input |= trx::accumulate(0, [](int state, int x) {
    return state + x * x;
});
// result: 55 (1² + 2² + 3² + 4² + 5²)
```

## Generators

### from
Zips up to three ranges pushing the corresponding elements together. The number of pushed elements is equal to the length of the shortest range.

```cpp
std::vector<int> input_a = {1, 2, 3, 4};
std::vector<std::string> input_b = {"one", "two", "three", "four"};
std::vector<std::string> result = trx::from(input_a, input_b)
    |= trx::filter([](int a, const std::string& b) { return a % 2 == 0; })
    |= trx::transform([](int a, const std::string& b) -> std::string { return b + ":" + std::to_string(a); })
    |= trx::into(std::vector<std::string>{});
// result: {"two:2", "four:4"}
```

### chain
Joins two ranges producing a range containing elements of the first range followed by elements of the second range.

```cpp
std::vector<int> input_a = {1, 2, 3};
std::vector<int> input_b = {10, 20, 30};
std::vector<int> result = trx::chain(input_a, input_b)
    |= trx::into(std::vector<int>{});
// result: {1, 2, 3, 10, 20, 30}
```

### range
Generates integral numbers from range `[lower, upper)` or `[0, upper)`.

```cpp
std::vector<int> result = trx::range(2, 8) |= trx::into(std::vector<int>{});
// result: {2, 3, 4, 5, 6, 7}

std::vector<int> result = trx::range(5) |= trx::into(std::vector<int>{});
// result: {0, 1, 2, 3, 4}
```

### iota
Generates infinite number of values starting from `lower`.

```cpp
std::vector<int> result = trx::iota(0) |= trx::take(3) |= trx::into(std::vector<int>{});
// result: {0, 1, 2}
```

### read_lines
Reads lines from stream one by one. Handles both CRLF and LF line-breaks.

```cpp
std::istringstream is{ "One\nTwo\nThree" };
std::vector<std::string> result = trx::read_lines(is) |= trx::into(std::vector<std::string>{}),
// result: {"One", "Two", "Three"}
```

### custom generators
```cpp
std::vector<std::string> result = trx::generator_t<int, int>([](auto yield) {
        for (int i = 0; i < 10; ++i>) {
            if (!yield(i, i * i)) {
                break;
            }
        }
    })
    |= trx::transform([](int v, int square) -> std::string { return std::to_string(v) + " " + std::to_string(square); })
    |= trx::take(4)
    |= trx::into(std::vector<std::string>{});
// result: {"0 0", "1 1", "2 4", "3 9"}
```

## other functions

### out

Changes the reducer into an output iterator:

```cpp
std::vector<int> input = {1, 2, 3};
std::vector<std::string> result = std::copy(
    input.begin(),
    input.end(),
    trx::out(trx::transfom([](int x) { return std::to_string(x); }) |= trx::into(std::vector<std::string>{}))).get();
// result: {"1", "2", "3"}
```

### to_reducer

Adapts a binary operator to match the reducer function syntax

```cpp
std::vector<int> input = {5, 10, 15};
int result = input |= trx::make_reductor(0, trx::to_reducer(std::plus<>{}));
// result: 30
```
