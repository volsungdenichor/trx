# trx

## reducers

### dev_null
A reducer that discards all items, useful for situations where you want to run transducers for side effects only.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
trx::from(input, trx::inspect([](int x) { std::cout << x << " "; }) |= trx::dev_null);
// Prints: 1 2 3 4 5
```

### partition
Distributes items into two separate reducers based on a predicate condition, creating a pair of results.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto [first, second] = trx::from(
    input,
    trx::partition(
        [](int x) { return x % 2 == 0; },
        trx::into(std::vector<int>{}),
        trx::into(std::vector<int>{}))
);
// first: {2, 4} (even numbers)
// second: {1, 3, 5} (odd numbers)
```

### fork
Sends the same item to multiple reducers simultaneously, collecting results into a tuple.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto [first, second] = trx::from(
    input,
    trx::fork(
        trx::into(std::vector<int>{}),
        trx::count)
);
// first: {1, 2, 3, 4, 5}
// second: 5
```

### copy_to
Copies items to an output iterator, advancing the iterator with each element.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
std::vector<int> dest(5);
trx::from(input, trx::copy_to(dest.begin()));
// dest: {1, 2, 3, 4, 5}
```

### push_back
Appends items to the end of a container.

```cpp
std::vector<int> input = {1, 2, 3};
std::vector<int> result;
trx::from(input, trx::push_back(result));
// result: {1, 2, 3}
```

### into
Creates a reducer that builds a container by appending items to it using push_back semantics.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto result = trx::from(input, trx::into(std::vector<int>{}));
// result: {1, 2, 3, 4, 5}
```

### count
Counts the total number of items processed by the reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto result = trx::from(input, trx::count);
// result: 5
```

## transducers

### all_of
Tests whether all items satisfy a predicate condition.

```cpp
std::vector<int> input = {2, 4, 6, 8};
auto result = trx::from(input, trx::all_of([](int x) { return x % 2 == 0; }));
// result: true (all are even)
```

### any_of
Tests whether at least one item satisfies a predicate condition.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto result = trx::from(input, trx::any_of([](int x) { return x % 2 == 0; }));
// result: true (2 and 4 are even)
```

### none_of
Tests whether no items satisfy a predicate condition.

```cpp
std::vector<int> input = {1, 3, 5, 7};
auto result = trx::from(input, trx::none_of([](int x) { return x % 2 == 0; }));
// result: true (none are even)
```

### transform
Applies a function to each item and passes the result to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto result = trx::from(input, trx::transform([](int x) { return x * 2; }) |= trx::into(std::vector<int>{}));
// result: {2, 4, 6, 8, 10}
```

## transform_indexed
Applies a function to each item along with its index and passes the result to the next reducer.

```cpp
std::vector<std::string> input = {"a", "b", "c"};
auto result = trx::from(
    input,
    trx::transform_indexed([](std::ptrdiff_t idx, const std::string& s) {
        return std::to_string(idx) + ":" + s;
    }) |= trx::into(std::vector<std::string>{}));
// result: {"0:a", "1:b", "2:c"}
```

### filter
Only passes items that satisfy a predicate to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 6};
auto result = trx::from(input, trx::filter([](int x) { return x % 2 == 0; }) |= trx::into(std::vector<int>{}));
// result: {2, 4, 6}
```

### filter_indexed
Only passes items that satisfy an index-aware predicate to the next reducer.

```cpp
std::vector<int> input = {10, 20, 30, 40, 50};
auto result = trx::from(
    input,
    trx::filter_indexed([](std::ptrdiff_t idx, int x) {
        return idx % 2 == 0;  // keep items at even indices
    }) |= trx::into(std::vector<int>{}));
// result: {10, 30, 50}

### inspect
Executes a function for side effects on each item without modifying the item passed to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3};
auto result = trx::from(input, trx::inspect([](int x) { std::cout << x << " "; }) |= trx::into(std::vector<int>{}));
// Prints: 1 2 3
// result: {1, 2, 3}
```

### inspect_indexed
Executes a function for side effects on each item and its index without modifying the item passed to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3};
auto result = trx::from(
    input,
    trx::inspect_indexed([](std::ptrdiff_t idx, int x) {
        std::cout << "[" << idx << "]=" << x << " ";
    }) |= trx::into(std::vector<int>{}));
// Prints: [0]=1 [1]=2 [2]=3
// result: {1, 2, 3}
```

### transform_maybe
Applies a function that returns an optional value, only passing non-empty results to the next reducer.

```cpp
std::vector<std::string> input = {"1", "2", "abc", "4"};
auto result = trx::from(
    input,
    trx::transform_maybe([](const std::string& s) -> std::optional<int> {
        try {
            return std::stoi(s);
        }
        catch (...) {
            return std::nullopt;
        }
    }) |= trx::into(std::vector<int>{}));
// result: {1, 2, 4} (skips "abc")
```

### transform_maybe_indexed
Applies an index-aware function that returns an optional value, only passing non-empty results to the next reducer.

```cpp
std::vector<std::string> input = {"1", "2", "abc", "4"};
auto result = trx::from(
    input,
    trx::transform_maybe_indexed([](std::ptrdiff_t idx, const std::string& s) -> std::optional<int> {
        try {
            return std::stoi(s) * idx;  // multiply by index
        }
        catch (...) {
            return std::nullopt;
        }
    }) |= trx::into(std::vector<int>{}));
// result: {0, 2, 12} (0*0, 2*1, skips "abc", 4*3)
```

### take_while
Passes items to the next reducer while a predicate is true, stopping once it becomes false.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 3, 2, 1};
auto result = trx::from(input, trx::take_while([](int x) { return x < 4; }) |= trx::into(std::vector<int>{}));
// result: {1, 2, 3}
```

### take_while_indexed
Passes items to the next reducer while an index-aware predicate is true, stopping once it becomes false.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 3, 2, 1};
auto result = trx::from(
    input,
    trx::take_while_indexed([](std::ptrdiff_t idx, int x) {
        return idx < 3;  // take first 3 items
    }) |= trx::into(std::vector<int>{}));
// result: {1, 2, 3}
```

### drop_while
Skips items while a predicate is true, then passes all remaining items to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 3, 2, 1};
auto result = trx::from(input, trx::drop_while([](int x) { return x < 4; }) |= into(std::vector<int>{}));
// result: {4, 5, 3, 2, 1}
```

### drop_while_indexed
Skips items while an index-aware predicate is true, then passes all remaining items to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5, 3, 2, 1};
auto result = trx::from(
    input,
    trx::drop_while_indexed([](std::ptrdiff_t idx, int x) {
        return idx < 3;  // skip first 3 items
    }) |= trx::into(std::vector<int>{}));
// result: {4, 5, 3, 2, 1}
```

### take
Limits the number of items passed to the next reducer to the specified count.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto result = trx::from(input, trx::take(3) |= trx::into(std::vector<int>{}));
// result: {1, 2, 3}
```

### drop
Skips the first N items, passing only items after that count to the next reducer.

```cpp
std::vector<int> input = {1, 2, 3, 4, 5};
auto result = trx::from(input, trx::drop(2) |= trx::into(std::vector<int>{}));
// result: {3, 4, 5}
```

### stride
Passes every Nth item to the next reducer based on the specified stride value.

```cpp
std::vector<int> input = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
auto result = trx::from(input, trx::stride(2) |= trx::into(std::vector<int>{}));
// result: {0, 2, 4, 6, 8}
```

### join
Flattens nested ranges by iterating through each element in the input container and passing individual items to the next reducer.

```cpp
std::vector<std::vector<int>> input = {{1, 2}, {3, 4}, {5, 6}};
auto result = trx::from(input, trx::join |= trx::into(std::vector<int>{}));
// result: {1, 2, 3, 4, 5, 6}
```

### intersperse

```cpp
std::string input = "ABCD";
auto result = trx::from(input, trx::intersperse(',') |= trx::into(std::string{}));
// result: "A,B,C,D"
```
