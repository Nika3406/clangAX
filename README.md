# ClangAX Programming Language

<img src="Cax.png" width="50%">

**Version 3.0** — A lightweight, bytecode-compiled language with manual memory management, built-in math, algorithms, data structures, and machine learning libraries.

---

## Table of Contents

- [Overview](#overview)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Language Syntax](#language-syntax)
  - [Functions](#functions)
  - [Variables and Types](#variables-and-types)
  - [Operators](#operators)
  - [Control Flow](#control-flow)
  - [Arrays](#arrays)
  - [Input / Output](#input--output)
  - [Arguments and Return Values](#arguments-and-return-values)
  - [User-Defined Libraries](#user-defined-libraries)
- [Memory Management](#memory-management)
  - [alloc and free](#alloc-and-free)
  - [Pointer Types](#pointer-types)
  - [Dereferencing and Address-Of](#dereferencing-and-address-of)
  - [Owning Pointers and Auto-Free](#owning-pointers-and-auto-free)
  - [Safety Model](#safety-model)
- [Built-in Libraries](#built-in-libraries)
  - [_math](#_math---mathematical-functions)
  - [_datastr](#_datastr---data-structures)
  - [_alg](#_alg---algorithms)
  - [_ml](#_ml---machine-learning)
- [Toolchain](#toolchain)
  - [clangax — Compiler](#clangax--compiler)
  - [caxvm — Virtual Machine](#caxvm--virtual-machine)
  - [caxdis — Disassembler](#caxdis--disassembler)
- [Examples](#examples)
- [Error Reference](#error-reference)
- [Limitations and Roadmap](#limitations-and-roadmap)
- [Quick Reference Card](#quick-reference-card)

---

## Overview

ClangAX is a compiled programming language that generates bytecode (`.caxb`) for a custom virtual machine (`caxvm`). Key features:

- **Simple C-like syntax** with modern conveniences
- **Manual memory management** — `alloc`, `free`, owning pointers (`own<T>`), raw pointers (`ptr<T>`), borrows (`borrow<T>`)
- **Generation-based use-after-free and double-free detection** at runtime
- **Four built-in libraries**: `_math`, `_datastr`, `_alg`, `_ml`
- **Dynamic typing** — `int`, `float`, `bool`, `string`, and `array`
- **No LLVM dependency** — fully self-contained compiler and VM
- **Cross-platform** — Windows, macOS, Linux

---

## Installation

### Build from Source

```bash
rm -rf cmake-build-debug
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
./clangax.sh
```

This produces three executables:

| Executable | Purpose |
|---|---|
| `clangax` | Compiler — converts `.cax` source to `.caxb` bytecode |
| `caxvm` | Virtual Machine — executes `.caxb` bytecode |
| `caxdis` | Disassembler — inspects `.caxb` bytecode |

---

## Quick Start

### 1. Write a program

`hello.cax`:
```
func(Main) {
    print("Hello, ClangAX!")
}
```

### 2. Compile

```bash
clangax hello.cax -o hello
```

Produces `hello.caxb`.

### 3. Run

```bash
caxvm hello.caxb
```

```
=== Execution ===
Hello, ClangAX!
=== Execution Complete ===
```

### 4. Inspect bytecode (optional)

```bash
caxdis hello.caxb --verbose
```

---

## Language Syntax

### Functions

Every ClangAX program is composed of functions. All executable code must be inside a function.

```
// Named function — called by name from other functions
func() = "greet" {
    print("Hello!")
}

// Main entry point — program execution begins here
func(Main) {
    greet()
}

// Typed function — grants access to specific library categories
func(Math) = "computeArea" {
    r = arg(0)
    pi = 3.14159
    area = pi * r * r
    return area
}

// Exec function — can use the privileged exec() dispatcher
func(Exec) = "runOp" {
    result = exec("math.sqrt", 16)
    return result
}
```

**Rules:**
- `func(Main)` is required — it is the program entry point.
- `func() = "name"` declares a callable function.
- `func(TypeTag) = "name"` associates a function with a library type (e.g., `Math`, `ML`, `Exec`).
- Functions without an explicit name default to `anonymous_func` unless the tag is `Main`.
- Nested `func` declarations inside a function body are allowed.

---

### Variables and Types

Variables are declared by assignment. Types are inferred dynamically.

```
// Integer
x = 42
count = 0

// Float
pi = 3.14159
ratio = 1.5

// Boolean
flag = true
done = false

// String
name = "ClangAX"
msg = "Hello, world!"

// Array
numbers = [1, 2, 3, 4, 5]
mixed = [1, 2.5, "hello", true]

// Null (uninitialized locals are null)
```

**Type annotation** (used with pointer types — see [Memory Management](#memory-management)):
```
own<int> p = alloc(42)
ptr<int> q = alloc(99)
borrow<int> r = &someVar
```

---

### Operators

```
// Arithmetic
sum       = a + b
diff      = a - b
product   = a * b
quotient  = a / b      // integer division when both sides are int
remainder = a % b      // modulo

// Comparison — produce bool
equal        = (a == b)
notEqual     = (a != b)
less         = (a < b)
greater      = (a > b)
lessEqual    = (a <= b)
greaterEqual = (a >= b)

// Increment / Decrement (post-fix only)
i++
j--

// Unary negation
neg = -x

// String concatenation via +
greeting = "Hello, " + name
```

**Notes:**
- Integer `+` integer → integer. Float involved → float.
- Integer `/` integer → integer (truncated toward zero). Float involved → float.
- String `+` anything → string (auto-converts the other side).

---

### Control Flow

#### If / Else

```
if (x > 10) {
    print("large")
} else {
    print("small")
}
```

Braces are required for both branches. There is no braceless single-line form.

#### While

```
count = 0
while (count < 5) {
    print(count)
    count++
}
```

#### For

```
for (i = 0; i < 10; i++) {
    print(i)
}
```

All three clauses (init, condition, increment) are optional:

```
// Infinite loop with manual break (not yet supported — use while(1))
for ( ; ; ) {
    // ...
}
```

#### Return

```
func() = "max" {
    a = arg(0)
    b = arg(1)
    if (a > b) {
        return a
    }
    return b
}
```

`return` with no expression returns from the function without pushing a value onto the caller's stack.

---

### Arrays

```
// Array literal
arr = [10, 20, 30, 40, 50]

// Element access (0-indexed)
first = arr[0]      // 10
last  = arr[4]      // 50

// Element assignment
arr[2] = 99

// Length
size = len(arr)     // 5

// Iterate
for (i = 0; i < len(arr); i++) {
    print(arr[i])
}
```

Arrays are dynamically sized at creation time. Currently, only fixed-size arrays defined by a literal are directly supported. Use the `_datastr` library for dynamic lists.

---

### Input / Output

```
// Print any value — integers, floats, bools, strings, arrays, pointers
print("Hello")
print(42)
print(3.14)
print(true)
print(arr)

// Read a line of input from stdin and coerce it to match the variable's current type
x = 0
write(x)        // reads an integer from stdin into x

name = "?"
write(name)     // reads a string from stdin into name
```

`write` coerces the input string to match the current type of the target variable:
- If `x` is `Int`, input is parsed as integer.
- If `x` is `Float`, input is parsed as float.
- If `x` is `Bool`, `"true"` / `"1"` → `true`, otherwise `false`.
- Otherwise the raw string is stored.

---

### Arguments and Return Values

```
func() = "add" {
    a = arg(0)
    b = arg(1)
    result = a + b
    return result
}

func() = "greet" {
    name = arg(0)
    print("Hello, ")
    print(name)
}

func(Main) {
    sum = add(3, 7)
    print(sum)          // 10

    greet("Alice")      // Hello, Alice
}
```

- `arg(n)` retrieves the n-th positional argument (0-indexed).
- Arguments are passed positionally when calling: `funcName(val0, val1, ...)`.
- Named arguments are supported for `exec()`-based library calls (see Built-in Libraries).
- Return values are placed on the caller's stack; void functions return nothing.

---

### User-Defined Libraries

You can split code across multiple `.cax` files and import them.

`mathutils.cax`:
```
func() = "square" {
    x = arg(0)
    return x * x
}

func() = "cube" {
    x = arg(0)
    return x * x * x
}
```

`main.cax`:
```
#import "mathutils"

func(Main) {
    s = square(5)
    c = cube(3)
    print(s)    // 25
    print(c)    // 27
}
```

Compile with a library path if the file is in a different directory:

```bash
clangax main.cax -L ./libs -o main
```

---

## Memory Management

ClangAX v2.0 introduces manual heap memory management with runtime safety checks.

### alloc and free

`alloc(expr)` allocates a heap cell containing the value of `expr` and returns a pointer to it.  
`free(ptr)` marks the heap cell as dead. Any further use of the pointer is a safety violation.

```
func(Main) {
    p = alloc(42)       // allocate int 42 on the heap; p is a pointer
    print(*p)           // dereference: prints 42
    free(p)             // release the heap cell
}
```

The heap uses a **generation counter**: when a cell is freed, its generation is incremented. Any pointer that still holds the old generation is detected as stale on the next use, preventing use-after-free silently corrupting data.

---

### Pointer Types

Type annotations describe ownership semantics. They are optional but recommended for clarity and for triggering automatic cleanup.

| Type | Meaning |
|---|---|
| `own<T>` | **Owning pointer** — the variable is responsible for freeing the allocation. Auto-freed when it goes out of scope. |
| `ptr<T>` | **Raw pointer** — non-owning. You must call `free()` manually if appropriate. |
| `borrow<T>` | **Borrow** — a temporary, non-owning reference. Semantically identical to `ptr<T>` at the VM level; the distinction is for the programmer. |

```
func(Main) {
    p: own<int>  = alloc(10)    // p owns this allocation
    q: ptr<int>  = alloc(20)    // q is a raw pointer (manual free required)
    r: borrow<int> = &someVar   // r borrows a local variable

    print(*p)    // 10
    print(*q)    // 20

    free(q)      // must free raw pointer manually
    // p is freed automatically at end of scope
}
```

---

### Dereferencing and Address-Of

**Dereference** (`*ptr`) — reads the value stored at the pointer's heap cell:

```
p = alloc(99)
val = *p            // val == 99
```

**Address-of** (`&var`) — creates a heap snapshot of a local variable and returns a pointer to it. The local itself is unaffected.

```
x = 55
p = &x              // p points to a heap copy of x (value 55)
val = *p            // val == 55
x = 100             // x changes; *p still == 55 (snapshot, not a reference)
```

> **Note:** `&var` creates a *snapshot copy*, not a live reference to the stack slot. Writes through `p` do not modify `x`.

**Storing through a pointer** — use assignment with a dereferenced pointer on the left-hand side via `DEREF_STORE` (generated automatically when the compiler detects a dereference-assignment pattern):

```
p = alloc(0)
*p = 77             // writes 77 into the heap cell
print(*p)           // 77
```

---

### Owning Pointers and Auto-Free

When a variable is declared with `own<T>`, the compiler automatically emits a `FREE` instruction at the end of the enclosing block. You do not need to call `free()` manually.

```
func(Main) {
    if (true) {
        p: own<int> = alloc(42)
        print(*p)
        // p is automatically freed here, at end of the if-block
    }
    // p no longer exists; its heap cell has been reclaimed
}
```

Auto-free respects **lexical scope**: if you declare multiple `own<>` variables in the same block, they are freed in **reverse declaration order** (LIFO), matching the natural stack discipline.

```
func(Main) {
    a: own<int> = alloc(1)
    b: own<int> = alloc(2)
    c: own<int> = alloc(3)
    // at end of block: c freed, then b, then a
}
```

**Important:** Re-assigning an `own<>` variable does **not** free the previous allocation. If you need to replace the value, free manually first:

```
p: own<int> = alloc(10)
free(p)
p: own<int> = alloc(20)    // safe — previous cell already freed
```

---

### Safety Model

The VM enforces the following at runtime:

| Violation | Detection | Behaviour |
|---|---|---|
| **Use-after-free** | Generation mismatch on `DEREF` or `DEREF_STORE` | Error printed; `null` pushed (program continues) |
| **Double-free** | `alive == false` on `FREE` | Error printed; `FREE` is a no-op |
| **Invalid pointer** | Tag is not `Pointer` on `DEREF`/`FREE` | Error printed; no-op / `null` |
| **Out-of-range pointer** | `heapIdx >= heap.size()` | Error printed; no-op / `null` |

Example of a caught use-after-free:

```
p = alloc(5)
free(p)
val = *p    // VM prints: "VM error: DEREF — use-after-free (block already freed)"
            // val == null
```

---

## Built-in Libraries

Import at the top of your file before any function declarations:

```
#import "_math"
#import "_datastr"
#import "_alg"
#import "_ml"
```

Multiple libraries can be imported in the same file.

---

### _math — Mathematical Functions

```
#import "_math"

func(Main) {
    // Basic
    s  = sqrt(16)          // 4.0
    p  = pow(2, 10)        // 1024.0
    a  = abs(-7)           // 7
    cb = cbrt(27)          // 3.0
    h  = hypot(3, 4)       // 5.0

    // Trigonometry (radians)
    s  = sin(3.14159 / 2)  // ~1.0
    c  = cos(0)            // 1.0
    t  = tan(0.785)        // ~1.0
    as = asin(1)           // ~1.5708
    ac = acos(1)           // 0.0
    at = atan(1)           // ~0.785
    a2 = atan2(1, 1)       // ~0.785

    // Hyperbolic
    sh = sinh(1)
    ch = cosh(1)
    th = tanh(0.5)

    // Exponential / Logarithmic
    e  = exp(1)            // ~2.718
    l  = log(2.718)        // ~1.0
    l10 = log10(100)       // 2.0
    l2  = log2(8)          // 3.0

    // Rounding
    fl = floor(4.9)        // 4.0
    ce = ceil(4.1)         // 5.0
    ro = round(4.5)        // 5.0
    tr = trunc(-3.7)       // -3.0
    fm = fmod(10, 3)       // 1.0

    // Min / Max
    mn = min(3, 7)         // 3
    mx = max(3, 7)         // 7

    // Number theory
    g  = gcd(48, 18)       // 6
    l  = lcm(4, 6)         // 12
    f  = factorial(6)      // 720

    // Quadratic solver — returns array of roots
    roots = quad(1, -5, 6) // [3.0, 2.0]  (solves x²-5x+6=0)

    // Statistics (take an array)
    data = [2, 4, 4, 4, 5, 5, 7, 9]
    avg = mean(data)       // 5.0
    med = median(data)     // 4.5
    sd  = stddev(data)     // 2.0
    var = variance(data)   // 4.0

    // Numerical calculus (take array of y-values + step size)
    ys  = [0, 1, 4, 9, 16]
    d   = derivative(ys, 1)    // [1, 3, 5, 7]
    ig  = integral(ys, 1)      // ~10.0
}
```

**Full function list:**

| Category | Functions |
|---|---|
| Basic | `sqrt`, `pow`, `abs`, `cbrt`, `hypot` |
| Trigonometry | `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2` |
| Hyperbolic | `sinh`, `cosh`, `tanh` |
| Exponential | `exp`, `log`, `log10`, `log2` |
| Rounding | `floor`, `ceil`, `round`, `trunc`, `fmod` |
| Comparison | `min`, `max` |
| Number theory | `quad`, `gcd`, `lcm`, `factorial` |
| Statistics | `mean`, `median`, `stddev`, `variance` |
| Calculus | `derivative`, `integral` |

---

### _datastr — Data Structures

```
#import "_datastr"

func(Main) {
    // --- Stack (LIFO) ---
    s = stack_new()
    stack_push(s, 10)
    stack_push(s, 20)
    top   = stack_pop(s)     // 20
    peek  = stack_peek(s)    // 10
    empty = stack_empty(s)   // false

    // --- Queue (FIFO) ---
    q = queue_new()
    queue_enqueue(q, 1)
    queue_enqueue(q, 2)
    front = queue_dequeue(q) // 1
    qf    = queue_front(q)   // 2
    qe    = queue_empty(q)   // false

    // --- Heap (min-priority queue) ---
    h = heap_new()
    heap_insert(h, 50)
    heap_insert(h, 20)
    heap_insert(h, 70)
    mn = heap_extract(h)     // 20
    pk = heap_peek(h)        // 50

    // --- Map (key-value) ---
    m = map_new()
    map_set(m, "name", "Alice")
    map_set(m, "age", 30)
    name   = map_get(m, "name")   // "Alice"
    has    = map_has(m, "age")    // true
    keys   = map_keys(m)          // ["name", "age"]
    map_delete(m, "age")

    // --- Set ---
    st = set_new()
    set_add(st, 10)
    set_add(st, 20)
    has2   = set_has(st, 10)        // true
    union  = set_union(st, other)
    inter  = set_intersect(st, other)
    set_delete(st, 10)

    // --- Linked List ---
    l = list_new()
    list_append(l, 1)
    list_append(l, 2)
    list_prepend(l, 0)
    val    = list_get(l, 1)         // 1
    list_insert(l, 2, 99)
    list_remove(l, 0)

    // --- Graph ---
    g = graph_new()
    graph_add_vertex(g, "A")
    graph_add_vertex(g, "B")
    graph_add_edge(g, "A", "B", 5, false)  // undirected, weight 5
    nb = graph_neighbors(g, "A")

    // --- Tree ---
    t   = tree_new("root")
    tree_add_child(t, "root", "child1")
    tree_add_child(t, "root", "child2")
    ch  = tree_children(t, "root")
}
```

**Full function list:**

| Structure | Functions |
|---|---|
| Stack | `stack_new`, `stack_push`, `stack_pop`, `stack_peek`, `stack_empty` |
| Queue | `queue_new`, `queue_enqueue`, `queue_dequeue`, `queue_front`, `queue_empty` |
| Heap | `heap_new`, `heap_insert`, `heap_extract`, `heap_peek` |
| Map | `map_new`, `map_set`, `map_get`, `map_has`, `map_delete`, `map_keys` |
| Set | `set_new`, `set_add`, `set_has`, `set_delete`, `set_union`, `set_intersect` |
| List | `list_new`, `list_append`, `list_prepend`, `list_get`, `list_insert`, `list_remove` |
| Graph | `graph_new`, `graph_add_vertex`, `graph_add_edge`, `graph_neighbors` |
| Tree | `tree_new`, `tree_add_child`, `tree_children` |

---

### _alg — Algorithms

```
#import "_alg"

func(Main) {
    data = [5, 2, 8, 1, 9, 3]

    // Sorting (modify array in-place; no return value)
    quicksort(data)        // [1, 2, 3, 5, 8, 9]
    mergesort(data)
    heapsort(data)
    bubblesort(data)
    insertionsort(data)

    // Searching (return index, or -1 if not found)
    sorted = [1, 2, 3, 5, 8, 9]
    idx = binary_search(sorted, 5)   // 3
    idx = linear_search(data, 8)     // 4

    // Graph traversal
    g = graph_new()
    // ... add vertices/edges ...
    visited  = bfs(g, "A")
    visited2 = dfs(g, "A")
    path     = dijkstra(g, "A", "Z")

    // Other graph algorithms
    dist  = bellman_ford(g, "A")     // shortest distances from A
    allP  = floyd_warshall(g)        // all-pairs shortest paths
    mst   = kruskal(g)               // minimum spanning tree edges
    mst2  = prim(g)

    // Dynamic programming
    sub   = lcs("ABCBDAB", "BDCAB")  // longest common subsequence
    val   = knapsack(weights, values, capacity)
    ed    = edit_distance("kitten", "sitting")

    // String algorithms (return index of match, or -1)
    pos = kmp_search("hello world", "world")    // 6
    pos = rabin_karp("hello world", "world")    // 6

    // Combinatorial
    perms  = permutations([1, 2, 3])
    combs  = combinations([1, 2, 3, 4], 2)
    shuffle(data)                   // randomises in-place
}
```

**Full function list:**

| Category | Functions |
|---|---|
| Sorting | `quicksort`, `mergesort`, `heapsort`, `bubblesort`, `insertionsort` |
| Searching | `binary_search`, `linear_search` |
| Graph | `bfs`, `dfs`, `dijkstra`, `bellman_ford`, `floyd_warshall`, `kruskal`, `prim` |
| Dynamic Programming | `lcs`, `knapsack`, `edit_distance` |
| String | `kmp_search`, `rabin_karp` |
| Combinatorial | `permutations`, `combinations`, `shuffle` |

---

### _ml — Machine Learning

```
#import "_ml"

func(Main) {
    // --- Regression ---
    x = [1, 2, 3, 4, 5]
    y = [2, 4, 6, 8, 10]
    lm     = linear_fit(x, y)
    pred   = linear_predict(lm, 6)     // ~12.0

    pm     = poly_fit(x, y, 2)         // degree-2 polynomial fit
    pp     = poly_predict(pm, 6)

    logm   = logistic_fit(x, y, 1000, 0.01)
    logp   = logistic_predict(logm, 3)

    // --- Classification ---
    xTr = [[1, 2], [2, 3], [3, 4]]
    yTr = [0, 0, 1]
    knn    = knn_fit(xTr, yTr, 3)
    label  = knn_predict(knn, [2.5, 3.5])

    dt     = dtree_fit(xTr, yTr, 5)
    dlabel = dtree_predict(dt, [2, 3])

    // --- Clustering ---
    data   = [[1, 2], [1.5, 1.8], [5, 8], [8, 8]]
    km     = kmeans_fit(data, 2, 100)
    cl     = kmeans_predict(km, [3, 4])

    // --- Neural Networks ---
    layers = [4, 8, 1]
    nn     = nn_new(layers)
    nn_train(nn, xTr, yTr, 1000, 0.01)
    out    = nn_predict(nn, [1, 2])

    // --- Activation functions ---
    sig  = sigmoid(0.5)        // 0.622
    rel  = relu(-0.5)          // 0.0
    tanh = tanh_act(0.5)       // 0.462
    sm   = softmax([1, 2, 3])  // [0.09, 0.245, 0.665]

    // --- Loss ---
    yTrue = [1, 2, 3, 4]
    yPred = [1.1, 2.1, 2.9, 4.2]
    mserr = mse(yTrue, yPred)
    cent  = cross_entropy(yTrue, yPred)

    // --- Metrics ---
    r2    = r2_score(yTrue, yPred)
    acc   = accuracy(yTrue, yPred)
    cm    = confusion_matrix(yTrue, yPred)

    // --- Preprocessing ---
    norm  = normalize(data)
    std   = standardize(data)
    split = train_test_split(x, y, 0.2)  // 80/20 split
}
```

**Full function list:**

| Category | Functions |
|---|---|
| Regression | `linear_fit`, `linear_predict`, `poly_fit`, `poly_predict`, `logistic_fit`, `logistic_predict` |
| Classification | `knn_fit`, `knn_predict`, `dtree_fit`, `dtree_predict` |
| Clustering | `kmeans_fit`, `kmeans_predict` |
| Neural Networks | `nn_new`, `nn_train`, `nn_predict` |
| Activation | `sigmoid`, `relu`, `tanh_act`, `softmax` |
| Loss | `mse`, `cross_entropy` |
| Metrics | `r2_score`, `accuracy`, `confusion_matrix` |
| Preprocessing | `normalize`, `standardize`, `train_test_split` |

---

## Toolchain

### clangax — Compiler

```bash
clangax <source.cax> [options]
```

| Option | Description |
|---|---|
| `-o <name>` | Output file base name (default: `output`) — produces `<name>.caxb` |
| `-L <path>` | Library search path for user `.cax` libraries |
| `--libs` | Print all available built-in libraries and their functions |
| `--verbose` | Print each compilation stage (tokenization, AST, bytecode) |
| `-h`, `--help` | Show usage information |
| `-v`, `--version` | Show compiler version |

```bash
# Compile with default output name (output.caxb)
clangax program.cax

# Specify output name
clangax program.cax -o myprogram

# User library in a subdirectory
clangax main.cax -L ./libs -o main

# Verbose compilation
clangax program.cax --verbose

# List available libraries
clangax --libs
```

---

### caxvm — Virtual Machine

```bash
caxvm <bytecode.caxb> [options]
```

| Option | Description |
|---|---|
| `--verbose`, `-v` | Print VM statistics (constant pool size, function count, heap usage) before and after execution |

```bash
# Run a compiled program
caxvm output.caxb

# Run with statistics
caxvm output.caxb --verbose
```

**Verbose output includes:**
- Constant pool size
- Number of functions
- Entry point function name
- Heap block count (live vs. freed) — useful for detecting memory leaks

---

### caxdis — Disassembler

```bash
caxdis <bytecode.caxb> [options]
```

| Option | Description |
|---|---|
| `--verbose`, `-v` | Show magic number and version header in addition to bytecode |

```bash
# Disassemble to stdout
caxdis output.caxb

# Verbose (includes header info)
caxdis output.caxb --verbose
```

**Output sections:**

1. **Constant pool** — all interned integers, floats, strings
2. **Functions** — for each function: name, local count, stack depth, and disassembled instructions with operands resolved against the constant pool
3. **Entry point** — which function index is the program entry

**Memory management opcodes shown:**

| Opcode | Operands | Description |
|---|---|---|
| `ALLOC` | — | Pops value, allocates heap cell, pushes pointer |
| `FREE` | — | Pops pointer, marks cell dead |
| `DEREF` | — | Pops pointer, pushes stored value |
| `DEREF_STORE` | — | Pops value then pointer, writes value into cell |
| `ADDR_OF` | `slot#N` | Creates heap snapshot of local slot N, pushes pointer |

---

## Examples

### Example 1: Variables, Operators, and Control Flow

```
func(Main) {
    x = 15
    y = 4

    sum  = x + y
    diff = x - y
    prod = x * y
    quot = x / y     // integer division: 3
    rem  = x % y     // modulo: 3

    print(sum)
    print(diff)
    print(prod)
    print(quot)
    print(rem)

    if (x > 10) {
        print("x is large")
    } else {
        print("x is small")
    }

    i = 0
    while (i < 5) {
        print(i)
        i++
    }

    for (j = 0; j < 3; j++) {
        print(j)
    }
}
```

---

### Example 2: Functions and Recursion

```
func() = "factorial" {
    n = arg(0)
    if (n <= 1) {
        return 1
    }
    sub = n - 1
    prev = factorial(sub)
    return n * prev
}

func() = "fibonacci" {
    n = arg(0)
    if (n <= 1) {
        return n
    }
    a = n - 1
    b = n - 2
    return fibonacci(a) + fibonacci(b)
}

func(Main) {
    print(factorial(6))    // 720
    print(fibonacci(10))   // 55
}
```

---

### Example 3: Arrays and Sorting

```
#import "_alg"
#import "_math"

func(Main) {
    data = [64, 34, 25, 12, 22, 11, 90]

    print("Before:")
    for (i = 0; i < len(data); i++) {
        print(data[i])
    }

    quicksort(data)

    print("After:")
    for (i = 0; i < len(data); i++) {
        print(data[i])
    }

    // Binary search on sorted array
    idx = binary_search(data, 34)
    print(idx)             // index of 34

    // Statistics
    avg = mean(data)
    sd  = stddev(data)
    print(avg)
    print(sd)
}
```

---

### Example 4: Data Structures

```
#import "_datastr"

func(Main) {
    // Stack — reverse a sequence
    s = stack_new()
    arr = [1, 2, 3, 4, 5]
    for (i = 0; i < len(arr); i++) {
        stack_push(s, arr[i])
    }
    while (stack_empty(s) == false) {
        print(stack_pop(s))    // prints 5 4 3 2 1
    }

    // Queue — FIFO processing
    q = queue_new()
    queue_enqueue(q, "first")
    queue_enqueue(q, "second")
    queue_enqueue(q, "third")
    print(queue_dequeue(q))    // "first"
    print(queue_dequeue(q))    // "second"

    // Map — key-value storage
    m = map_new()
    map_set(m, "alice", 90)
    map_set(m, "bob", 75)
    map_set(m, "carol", 88)
    print(map_get(m, "alice"))     // 90
    print(map_has(m, "dave"))      // false
}
```

---

### Example 5: Manual Memory Management

```
func(Main) {
    // Basic alloc and free
    p = alloc(100)
    print(*p)              // 100
    free(p)

    // Owning pointer — auto-freed at end of block
    if (true) {
        q: own<int> = alloc(42)
        print(*q)          // 42
        // q freed automatically here
    }

    // Multiple owning pointers — freed in reverse order
    a: own<int> = alloc(1)
    b: own<int> = alloc(2)
    c: own<int> = alloc(3)
    print(*a)
    print(*b)
    print(*c)
    // freed in order: c, b, a

    // Address-of — snapshot pointer to a local
    x = 55
    ref = &x
    print(*ref)            // 55
    x = 99
    print(*ref)            // still 55 — snapshot, not live reference
    free(ref)
}
```

---

### Example 6: Pointer Safety — Catching Violations

```
func(Main) {
    p = alloc(10)
    free(p)

    // Use-after-free: VM will print an error and push null
    val = *p
    print(val)             // prints null

    // Double-free: VM will print an error and ignore the second free
    free(p)

    // Safe usage pattern using ptr<T>
    q: ptr<int> = alloc(77)
    print(*q)              // 77
    free(q)                // explicit free for raw pointer
}
```

---

### Example 7: Machine Learning

```
#import "_ml"
#import "_math"

func(Main) {
    // Linear regression on y = 2x + 1
    x = [1, 2, 3, 4, 5]
    y = [3, 5, 7, 9, 11]

    model = linear_fit(x, y)

    pred6  = linear_predict(model, 6)
    pred10 = linear_predict(model, 10)

    print(pred6)     // ~13.0
    print(pred10)    // ~21.0

    // Evaluate
    yPred = [3.0, 5.0, 7.0, 9.0, 11.0]
    err   = mse(y, yPred)
    score = r2_score(y, yPred)

    print(err)       // ~0.0
    print(score)     // ~1.0

    // Normalise before clustering
    raw  = [[1, 100], [2, 200], [10, 1000]]
    norm = normalize(raw)
    km   = kmeans_fit(norm, 2, 50)
}
```

---

### Example 8: Graph Algorithms

```
#import "_datastr"
#import "_alg"

func(Main) {
    g = graph_new()

    graph_add_vertex(g, "A")
    graph_add_vertex(g, "B")
    graph_add_vertex(g, "C")
    graph_add_vertex(g, "D")

    graph_add_edge(g, "A", "B", 1, false)
    graph_add_edge(g, "A", "C", 4, false)
    graph_add_edge(g, "B", "C", 2, false)
    graph_add_edge(g, "B", "D", 5, false)
    graph_add_edge(g, "C", "D", 1, false)

    bfsResult  = bfs(g, "A")
    dfsResult  = dfs(g, "A")
    shortPath  = dijkstra(g, "A", "D")

    print(bfsResult)
    print(dfsResult)
    print(shortPath)
}
```

---

### Example 9: User Input and Interactive Program

```
func(Main) {
    name = "?"
    write(name)

    greeting = "Hello, "
    print(greeting)
    print(name)

    x = 0
    y = 0
    write(x)
    write(y)

    sum = x + y
    print("Sum: ")
    print(sum)

    diff = x - y
    print("Difference: ")
    print(diff)
}
```

---

### Example 10: Combined — Statistics Calculator

```
#import "_math"
#import "_alg"

func() = "printStats" {
    data = arg(0)
    label = arg(1)

    print(label)
    print("  mean:   ")
    print(mean(data))
    print("  median: ")
    print(median(data))
    print("  stddev: ")
    print(stddev(data))
    print("  min:    ")
    m = data[0]
    for (i = 1; i < len(data); i++) {
        if (data[i] < m) {
            m = data[i]
        }
    }
    print(m)
}

func(Main) {
    scores = [72, 85, 91, 63, 77, 88, 95, 70, 82, 79]

    printStats(scores, "Scores before sort:")

    quicksort(scores)

    printStats(scores, "Scores after sort:")

    // Quadratic: find roots of x^2 - 6x + 8
    roots = quad(1, -6, 8)
    print("Roots of x^2-6x+8:")
    print(roots)
}
```

---

## Error Reference

| Error Message | Cause | Fix |
|---|---|---|
| `Parse error: Expected 'func'` | Code outside any function | Wrap all code in `func` |
| `Parse error: Expected '('` | Missing parenthesis | Check syntax around the reported line |
| `Unknown function: X` | Function not defined or not imported | Define it or `#import` the library |
| `Failed to import library: X` | Library name wrong or file missing | Built-in names start with `_`; user libs need the correct path |
| `Unknown library function: lib.fn` | Function doesn't exist in that library | Check the library function table above |
| `Invalid bytecode (bad magic)` | File is corrupt or not a `.caxb` file | Recompile the source |
| `VM error: Stack underflow` | VM bug or malformed bytecode | Report as a bug |
| `VM error: DEREF — use-after-free` | Dereferencing a freed pointer | Audit pointer lifetimes; use `own<T>` for auto-management |
| `VM error: DEREF — stale pointer` | Generation mismatch (pointer copied before free) | Do not use pointer copies after calling `free()` on the original |
| `VM error: FREE — use-after-free` | Calling `free()` on an already-freed pointer | Double-free detected; check control flow |
| `VM error: ALLOC/FREE on non-pointer` | Wrong type passed to a memory opcode | Verify the variable holds a pointer |

---

## Limitations and Roadmap

### Current Limitations

- No string manipulation functions (length, substring, etc.) — planned
- No file I/O — planned
- `&var` produces a snapshot, not a live reference to the stack slot
- No multi-dimensional array literal syntax (use arrays of arrays)
- Error messages show line numbers for parse errors; runtime errors show opcode context only
- No module system beyond `#import` for `.cax` files and built-in libraries

### Roadmap

- [x] Function parameters (`arg(n)`) and return values
- [x] Manual memory management (`alloc`, `free`, `own<T>`, `ptr<T>`, `borrow<T>`)
- [x] Use-after-free and double-free detection
- [x] Auto-free for `own<T>` at scope exit
- [ ] String manipulation library (`strlen`, `substr`, `split`, `join`, `format`)
- [ ] File I/O (`file_open`, `file_read`, `file_write`, `file_close`)
- [ ] JSON parsing
- [ ] Better runtime error messages with source line numbers
- [ ] Debugger support (`caxdbg`)
- [ ] GPU acceleration for `_ml` operations
- [ ] Standard library expansion

---

## Quick Reference Card

### Compile and Run
```bash
clangax source.cax -o program    # Compile  → program.caxb
caxvm program.caxb               # Execute
caxvm program.caxb --verbose     # Execute with stats
caxdis program.caxb              # Disassemble
clangax --libs                   # List built-in libraries
```

### Syntax Cheatsheet
```
#import "_math"              // import built-in library
#import "mylib"              // import user library

func(Main) { }              // entry point
func() = "name" { }        // named function
func(Math) = "name" { }    // typed function

x = 42                      // integer variable
f = 3.14                    // float variable
b = true                    // bool variable
s = "hello"                 // string variable
a = [1, 2, 3]               // array variable

x: own<int>    = alloc(0)   // owning pointer (auto-freed)
x: ptr<int>    = alloc(0)   // raw pointer    (manual free)
x: borrow<int> = &var       // borrow / snapshot

val = *ptr                  // dereference
ref = &var                  // snapshot address-of
free(ptr)                   // explicit deallocation

n = arg(0)                  // first argument
return expr                 // return value

if (cond) { } else { }      // conditional
while (cond) { }            // while loop
for (init; cond; incr) { }  // for loop

print(x)                    // output
write(x)                    // input (reads into x)
len(arr)                    // array length
arr[i]                      // array access
```

### Library Quick Import
```
#import "_math"      // sqrt, pow, sin, cos, mean, stddev, quad, ...
#import "_datastr"   // stack, queue, heap, map, set, list, graph, tree
#import "_alg"       // quicksort, binary_search, dijkstra, lcs, ...
#import "_ml"        // linear_fit, knn_fit, nn_new, sigmoid, mse, ...
```

---

**Version:** 3.0 | **Last Updated:** April 2026
