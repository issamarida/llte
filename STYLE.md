# LLTE C++ Style Guide

This document is the source of truth for how C++ code is written in this
project. It exists because LLTE is a low-latency system where a single
unnecessary copy or cache miss is observable, and because cognitive load
in concurrent code is the leading cause of bugs.

The rules are **non-negotiable**. If you find yourself fighting them,
the answer is almost always that the design is wrong, not the rule.

---

## The five rules, compressed

1. **Name things clearly.** If you need a comment to explain *what* it
   is, the name is wrong.
2. **Avoid unnecessary work.** Every line of code runs at runtime.
3. **Keep data contiguous.** The CPU cares about cache lines, not
   abstractions.
4. **Measure everything.** If you didn't measure it, you don't know.
5. **Make behaviour obvious.** Cleverness is a liability.

---

## 1. Code explains itself; comments are a last resort

Bad:
```cpp
int x;  // milliseconds since last update
```

Good:
```cpp
std::int64_t last_update_ms;
```

Comments are valid only for:
- **Why** a non-obvious decision was made
- **Performance** trade-offs that aren't visible from the code
- **Concurrency** reasoning (memory orders, lock-free invariants)

Example of a valid comment:
```cpp
// volatile prevents the optimiser from deleting the loop entirely
volatile std::uint64_t accumulator = 0;
```

If you delete a comment and the code still makes sense, the comment was
noise.

---

## 2. Naming

Use **domain + intent**, never abbreviations.

### Variables: nouns, with units

```cpp
double bid_price;
int    order_quantity;
auto   latency_ns;
auto   timeout_ms;
```

### Functions: verbs

```cpp
process_order();
update_order_book();
compute_pnl();
```

### Data types: nouns

```cpp
struct OrderBook { ... };
struct TradeEvent { ... };
struct MarketSnapshot { ... };
```

### Anti-patterns

| Bad             | Good                          |
|-----------------|-------------------------------|
| `int p;`        | `int order_priority;`         |
| `auto d;`       | `auto duration_ns;`           |
| `int t;`        | `auto timestamp_cycles;`      |
| `auto a, b, c;` | `auto first, second, third;`  |
| `int x;` (loop) | `int iteration;`              |
| `int n;`        | `int message_count;`          |

### Casing

- `snake_case` for variables, functions, namespaces
- `PascalCase` for types (`struct`, `class`, `enum class`)
- `SCREAMING_SNAKE_CASE` for `constexpr` constants and macros

```cpp
namespace llte::timing {
    struct CalibrationResult {
        double tsc_hz;
        std::uint64_t reference_cycles;
    };
    constexpr int CALIBRATION_SAMPLES = 5;
    void compute_calibration();
}
```

---

## 3. Avoid unnecessary work

Always ask: *"What is this code doing at runtime?"*

```cpp
// Pass by const reference, not by value
void process_book(const OrderBook& book);

// Reserve capacity when size is known
std::vector<Order> orders;
orders.reserve(1024);

// Stack over heap whenever lifetime allows
Order order;                              // good
auto  order = std::make_unique<Order>();  // only if shared/moved
```

**Zero allocations in the hot path.** This is the most important rule
of the project. Pre-allocate everything in `init()`. The hot path
*must not* call `new`, `malloc`, `std::vector::push_back` past
capacity, `std::string` operations that allocate, or anything that
takes a mutex.

---

## 4. Data layout beats algorithms

CPUs are fast; memory is slow. The biggest performance wins in
low-latency systems come from layout, not better Big-O.

```cpp
// Bad: pointer-chasing, cache-hostile
std::map<double, int> price_levels;

// Good: contiguous, cache-friendly
struct PriceLevel {
    double price;
    int    quantity;
};
std::vector<PriceLevel> price_levels;
```

Ask of every data structure:
- Is it **contiguous** in memory?
- Does it fit in a **cache line** (64 bytes on x86)?
- Are fields accessed together **adjacent**?
- Does it need **padding** to avoid false sharing between threads?

---

## 5. Concurrency: simple beats clever

- Prefer **SPSC** queues over MPMC; only graduate when forced
- **No locks** in the hot path — ever
- **No shared mutable state** — pass messages instead
- If you can't reason about it on a whiteboard in five minutes, redesign

---

## 6. Make performance visible

Every performance claim is backed by a benchmark in `benches/`. No
exceptions. If you optimised something, prove it:

```cpp
auto start_cycles   = llte::timing::now_cycles();
process_message(msg);
auto end_cycles     = llte::timing::now_cycles();
auto elapsed_cycles = end_cycles - start_cycles;
```

If you don't measure, you're guessing.

---

## 7. Keep functions small and focused

Each function does **one thing**, named for that thing.

```cpp
// Bad
void handle_everything();

// Good
void parse_market_message();
void update_order_book();
void publish_book_snapshot();
```

A function that doesn't fit on one screen is doing too much.

---

## 8. Use types to prevent bugs

Type aliases at minimum:
```cpp
using Price        = double;
using Quantity     = std::int64_t;
using OrderId      = std::uint64_t;
using TimestampNs  = std::uint64_t;
```

Strong types (struct wrappers) when mixing them up would be a real bug:
```cpp
struct Price { double value; };
struct Quantity { std::int64_t value; };
// Now you can't accidentally pass a Price where a Quantity is expected.
```

Don't strong-type everything. Only where the bug-prevention is worth
the syntactic noise.

---

## 9. Be explicit about ownership

- `std::unique_ptr<T>` — exclusive ownership
- `std::shared_ptr<T>` — shared ownership (rare, justify in a comment)
- `T&` / `const T&` — borrowed, no ownership transfer
- `T*` — only for nullable non-owning, or interop with C APIs

Raw `new` and `delete` are forbidden outside of fundamental primitives
(e.g. inside the seqlock implementation, where they are unavoidable).

---

## 10. Remove noise

```cpp
// Bad
if (flag == true) { ... }
if (ptr != nullptr) { ... }
return (x);

// Good
if (flag) { ... }
if (ptr)  { ... }
return x;
```

Good code is **minimal, direct, predictable**.

---

## 11. Consistency beats cleverness

Same names, same patterns, same idioms — everywhere. A reader should
be able to predict the next line of code from the previous one.

If you find a better way to do something, change it everywhere or
nowhere. Mixed conventions are worse than a worse convention applied
uniformly.

---

## 12. Think like the machine

For every line of hot-path code, answer:

- Is this **cached**?
- Is this **contiguous**?
- Is this **branching**? (predictable or not?)
- Is this **allocating**?
- Is this **synchronising**? (atomics, fences, syscalls?)

You don't have to optimise everything. You do have to know.

---

## 13. Design for debugging

Concurrency bugs are unobservable unless you build observability in
from day one:

- Every message gets a **timestamp** at every stage
- Every order gets a **monotonic ID**
- Every event is **logged** to the persistence stream
- Every run can be **replayed deterministically**

This is non-optional.

---

## What to avoid

- **Premature abstractions.** Build the concrete thing first; abstract
  on the third repetition, not the first.
- **Inheritance.** Composition and free functions. Inheritance is
  reserved for genuine `is-a` relationships, which are rare.
- **Templates early.** Templates are a tax on compile time and a
  multiplier on error messages. Use them when you have two real call
  sites that need to vary, not before.
- **Clever code.** If a junior engineer can't read it, rewrite it.

---

## Mechanical enforcement

Formatting is enforced by `clang-format` (`.clang-format` in repo root).
Run before every commit:

```bash
clang-format -i $(git diff --name-only --diff-filter=ACM | grep -E '\.(hpp|cpp)$')
```

Or set up a pre-commit hook. There is no debate over formatting in
review; clang-format is right by definition.

---

## The mental model

> **Readable like English, predictable like math, efficient like C.**

If a line of code is two of those three, it's wrong.
