# ClangAX Programming Language

**Version 2.0** - A lightweight, bytecode-compiled language with built-in math, algorithms, data structures, and machine learning libraries.

---

## Table of Contents
- [Overview](#overview)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Language Syntax](#language-syntax)
- [Built-in Libraries](#built-in-libraries)
- [Toolchain](#toolchain)
- [Examples](#examples)

---

## Overview

ClangAX is a compiled programming language that generates bytecode for a custom virtual machine. It features:

- **Simple C-like syntax** with modern conveniences
- **Four built-in libraries**: `_math`, `_datastr`, `_alg`, `_ml`
- **Dynamic typing** with int, float, bool, string, and array types
- **No LLVM dependency** - fully self-contained compiler and VM
- **Cross-platform** - runs on Windows, macOS, and Linux

---

## Installation

### Build from Source

```bash
./clangax.sh
```

This produces three executables:
- `clangax` - Compiler
- `caxvm` - Virtual Machine
- `caxdis` - Disassembler

---

## Quick Start

### 1. Write a Program

Create `hello.cax`:

```cax
func(Main) {
    print("Hello, ClangAX!");
}
```

### 2. Compile

```bash
clangax hello.cax -o hello
```

This generates `hello.caxb` (bytecode file).

### 3. Run

```bash
caxvm hello.caxb
```

Output:
```
=== Execution ===
Hello, ClangAX!
=== Execution Complete ===
```

---

## Language Syntax

### Function Declaration

```cax
// Basic function
func() = "myFunction" {
    // function body
}

// Main entry point
func(Main) {
    // program starts here
}

// Typed function (for library access)
func(Math) = "calculate" {
    // can use _math library functions
}
```

### Variables and Types

```cax
// Integer
x = 42;

// Float
pi = 3.14159;

// Boolean
flag = true;

// String
name = "ClangAX";

// Array
numbers = [1, 2, 3, 4, 5];
```

### Control Flow

```cax
// If statement
if (x > 10) {
    print("x is large");
} else {
    print("x is small");
}

// While loop
count = 0;
while (count < 5) {
    print(count);
    count++;
}

// For loop
for (i = 0; i < 10; i++) {
    print(i);
}
```

### Arrays

```cax
// Create array
arr = [10, 20, 30, 40];

// Access elements
first = arr[0];      // 10

// Modify elements
arr[1] = 25;

// Array length
size = len(arr);     // 4
```

### Operators

```cax
// Arithmetic
sum = a + b;
diff = a - b;
product = a * b;
quotient = a / b;
remainder = a % b;

// Comparison
equal = (a == b);
notEqual = (a != b);
less = (a < b);
greater = (a > b);
lessEqual = (a <= b);
greaterEqual = (a >= b);

// Increment/Decrement
i++;
j--;
```

### Input/Output

```cax
// Print to console
print("Hello");
print(42);
print(variable);

// Read from console
write(x);  // Reads input and stores in variable x
```

---

## Built-in Libraries

Import libraries at the top of your file:

```cax
#import "_math"
#import "_datastr"
#import "_alg"
#import "_ml"
```

### _math - Mathematical Functions

```cax
#import "_math"

func(Main) {
    // Basic operations
    result = sqrt(16);           // 4.0
    power = pow(2, 8);           // 256.0
    absolute = abs(-42);         // 42
    
    // Trigonometry
    sine = sin(1.57);            // ~1.0
    cosine = cos(0);             // 1.0
    tangent = tan(0.785);        // ~1.0
    
    // Rounding
    down = floor(4.7);           // 4.0
    up = ceil(4.3);              // 5.0
    nearest = round(4.5);        // 5.0
    
    // Min/Max
    minimum = min(10, 20);       // 10
    maximum = max(10, 20);       // 20
    
    // Statistics
    data = [1, 2, 3, 4, 5];
    average = mean(data);        // 3.0
    stdDev = stddev(data);       // Standard deviation
    variance = variance(data);   // Variance
    
    // Algebra
    roots = quad(1, -5, 6);      // Solve x² - 5x + 6 = 0
    greatest = gcd(48, 18);      // 6
    least = lcm(12, 18);         // 36
    fact = factorial(5);         // 120
}
```

**Available Functions:**
- **Basic:** `sqrt`, `pow`, `abs`, `cbrt`, `hypot`
- **Trigonometric:** `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`
- **Hyperbolic:** `sinh`, `cosh`, `tanh`
- **Exponential:** `exp`, `log`, `log10`, `log2`
- **Rounding:** `floor`, `ceil`, `round`, `trunc`, `fmod`
- **Comparison:** `min`, `max`
- **Algebra:** `quad`, `gcd`, `lcm`, `factorial`
- **Statistics:** `mean`, `median`, `stddev`, `variance`
- **Calculus:** `derivative`, `integral`

### _datastr - Data Structures

```cax
#import "_datastr"

func(Main) {
    // Stack (LIFO)
    s = stack_new();
    stack_push(s, 10);
    stack_push(s, 20);
    top = stack_pop(s);          // 20
    isEmpty = stack_empty(s);    // false
    
    // Queue (FIFO)
    q = queue_new();
    queue_enqueue(q, 1);
    queue_enqueue(q, 2);
    front = queue_dequeue(q);    // 1
    
    // Heap (Priority Queue)
    h = heap_new();
    heap_insert(h, 50);
    heap_insert(h, 20);
    heap_insert(h, 70);
    min = heap_extract(h);       // 20 (min-heap)
    
    // Map (Key-Value)
    m = map_new();
    map_set(m, "name", "Alice");
    map_set(m, "age", 30);
    name = map_get(m, "name");   // "Alice"
    hasKey = map_has(m, "age");  // true
    
    // Set
    set = set_new();
    set_add(set, 10);
    set_add(set, 20);
    contains = set_has(set, 10); // true
    
    // Graph
    g = graph_new();
    graph_add_vertex(g, "A");
    graph_add_vertex(g, "B");
    graph_add_edge(g, "A", "B", 5);
    neighbors = graph_neighbors(g, "A");
}
```

**Available Structures:**
- **Stack:** `stack_new`, `stack_push`, `stack_pop`, `stack_peek`, `stack_empty`
- **Queue:** `queue_new`, `queue_enqueue`, `queue_dequeue`, `queue_front`, `queue_empty`
- **Heap:** `heap_new`, `heap_insert`, `heap_extract`, `heap_peek`
- **Map:** `map_new`, `map_set`, `map_get`, `map_has`, `map_delete`, `map_keys`
- **Set:** `set_new`, `set_add`, `set_has`, `set_delete`, `set_union`, `set_intersect`
- **List:** `list_new`, `list_append`, `list_prepend`, `list_get`, `list_insert`, `list_remove`
- **Graph:** `graph_new`, `graph_add_vertex`, `graph_add_edge`, `graph_neighbors`
- **Tree:** `tree_new`, `tree_add_child`, `tree_children`

### _alg - Algorithms

```cax
#import "_alg"

func(Main) {
    data = [5, 2, 8, 1, 9];
    
    // Sorting (modifies array in-place)
    quicksort(data);             // [1, 2, 5, 8, 9]
    
    // Searching
    sorted = [1, 2, 5, 8, 9];
    index = binary_search(sorted, 5);  // 2
    
    // Shuffle
    shuffle(data);               // Randomizes order
}
```

**Available Algorithms:**
- **Sorting:** `quicksort`, `mergesort`, `heapsort`, `bubblesort`, `insertionsort`
- **Searching:** `binary_search`, `linear_search`
- **Graph:** `bfs`, `dfs`, `dijkstra`, `bellman_ford`, `floyd_warshall`, `kruskal`, `prim`
- **Dynamic Programming:** `lcs`, `knapsack`, `edit_distance`
- **String:** `kmp_search`, `rabin_karp`
- **Combinatorial:** `permutations`, `combinations`, `shuffle`

### _ml - Machine Learning

```cax
#import "_ml"

func(Main) {
    // Linear Regression
    x = [1, 2, 3, 4, 5];
    y = [2, 4, 6, 8, 10];
    model = linear_fit(x, y);
    prediction = linear_predict(model, 6);  // ~12
    
    // K-Nearest Neighbors
    xTrain = [[1, 2], [2, 3], [3, 4]];
    yTrain = [0, 0, 1];
    knn = knn_fit(xTrain, yTrain, 3);
    label = knn_predict(knn, [2.5, 3.5]);
    
    // K-Means Clustering
    data = [[1, 2], [1.5, 1.8], [5, 8], [8, 8]];
    clusters = kmeans_fit(data, 2, 100);
    
    // Activation Functions
    activated = sigmoid(0.5);
    rectified = relu(-0.5);         // 0
    hyperbolic = tanh_act(0.5);
    
    // Metrics
    yTrue = [1, 2, 3, 4];
    yPred = [1.1, 2.1, 2.9, 4.2];
    error = mse(yTrue, yPred);
    score = r2_score(yTrue, yPred);
    
    // Preprocessing
    normalized = normalize(data);
    standardized = standardize(data);
}
```

**Available Functions:**
- **Regression:** `linear_fit`, `linear_predict`, `poly_fit`, `poly_predict`, `logistic_fit`, `logistic_predict`
- **Classification:** `knn_fit`, `knn_predict`, `dtree_fit`, `dtree_predict`
- **Clustering:** `kmeans_fit`, `kmeans_predict`
- **Neural Networks:** `nn_new`, `nn_train`, `nn_predict`
- **Activation:** `sigmoid`, `relu`, `tanh_act`, `softmax`
- **Loss:** `mse`, `cross_entropy`
- **Metrics:** `r2_score`, `accuracy`, `confusion_matrix`
- **Preprocessing:** `normalize`, `standardize`, `train_test_split`

---

## Toolchain

### clangax - Compiler

Compiles `.cax` source files to `.caxb` bytecode.

```bash
clangax <source.cax> [options]
```

**Options:**
- `-o <name>` - Output filename (default: `output`)
- `-L <path>` - Library search path
- `--libs` - List available libraries
- `--verbose` - Print compilation stages
- `-h, --help` - Show help
- `-v, --version` - Show version

**Examples:**
```bash
# Basic compilation
clangax program.cax

# Custom output name
clangax program.cax -o myprogram

# Verbose output
clangax program.cax --verbose

# List libraries
clangax --libs
```

### caxvm - Virtual Machine

Executes `.caxb` bytecode files.

```bash
caxvm <bytecode.caxb> [options]
```

**Options:**
- `--verbose` or `-v` - Show VM statistics

**Examples:**
```bash
# Run bytecode
caxvm output.caxb

# Verbose execution
caxvm output.caxb --verbose
```

### caxdis - Disassembler

Disassembles `.caxb` bytecode for inspection.

```bash
caxdis <bytecode.caxb> [options]
```

**Options:**
- `--verbose` or `-v` - Show detailed information

**Examples:**
```bash
# Disassemble bytecode
caxdis output.caxb

# Verbose disassembly
caxdis output.caxb --verbose
```

**Output includes:**
- Constant pool entries
- Function definitions
- Bytecode instructions with operands
- Entry point information

---

## Examples

### Example 1: Basic Math

```cax
#import "_math"

func(Main) {
    numbers = [1, 2, 3, 4, 5];
    
    avg = mean(numbers);
    sd = stddev(numbers);
    
    print("Average: ");
    print(avg);
    print("Std Dev: ");
    print(sd);
}
```

### Example 2: Sorting

```cax
#import "_alg"

func(Main) {
    data = [64, 34, 25, 12, 22, 11, 90];
    
    print("Before sorting:");
    for (i = 0; i < len(data); i++) {
        print(data[i]);
    }
    
    quicksort(data);
    
    print("After sorting:");
    for (i = 0; i < len(data); i++) {
        print(data[i]);
    }
}
```

### Example 3: Linear Regression

```cax
#import "_ml"

func(Main) {
    // Training data: y = 2x
    x = [1, 2, 3, 4, 5];
    y = [2, 4, 6, 8, 10];
    
    // Fit model
    model = linear_fit(x, y);
    
    // Make predictions
    pred1 = linear_predict(model, 6);
    pred2 = linear_predict(model, 10);
    
    print("Prediction for x=6: ");
    print(pred1);
    
    print("Prediction for x=10: ");
    print(pred2);
}
```

### Example 4: Stack Usage

```cax
#import "_datastr"

func(Main) {
    stack = stack_new();
    
    // Push elements
    stack_push(stack, 10);
    stack_push(stack, 20);
    stack_push(stack, 30);
    
    // Pop and print
    while (!stack_empty(stack)) {
        value = stack_pop(stack);
        print(value);
    }
}
```

### Example 5: Fibonacci

```cax
func() = "fibonacci" {
    n = 10;
    a = 0;
    b = 1;
    
    for (i = 0; i < n; i++) {
        print(a);
        temp = a + b;
        a = b;
        b = temp;
    }
}

func(Main) {
    fibonacci();
}
```

---

## Error Handling

Common errors and solutions:

| Error | Cause | Solution |
|-------|-------|----------|
| `Parse error: Expected 'func'` | Missing function declaration | Ensure all code is inside functions |
| `Unknown function: X` | Function not defined | Define function or import library |
| `Failed to import library: X` | Library doesn't exist | Check library name (must start with `_`) |
| `Unknown library function: X` | Function not in library | Check library documentation |
| `Invalid bytecode (bad magic)` | Corrupted bytecode | Recompile source file |
| `Stack underflow` | VM error | Report as bug |

---

## Performance Tips

1. **Use appropriate data structures** - Choose the right structure from `_datastr`
2. **Prefer `quicksort` for large datasets** - Fastest sorting algorithm
3. **Use `binary_search` on sorted arrays** - Much faster than linear search
4. **Normalize/standardize data** - Improves ML model performance
5. **Avoid nested loops** - Use built-in algorithms when possible

---

## Limitations

Current version limitations:

- No string manipulation functions (planned)
- No file I/O operations (planned)
- Limited error messages (improving)
- No multi-dimensional arrays (use arrays of arrays)
- No function parameters (v0.x limitation)
- No return values from user functions (v0.x limitation)

---

## Roadmap

Planned features:

- [x] Function parameters and return values
- [x] String manipulation library
- [x] File I/O operations
- [ ] JSON parsing
- [ ] Better error messages with line numbers
- [ ] Debugger support
- [ ] Standard library expansion
- [ ] GPU acceleration for ML operations

---

## Contributing

ClangAX is open for contributions! Areas of focus:

- Library implementations
- VM optimizations
- Language features
- Documentation
- Example programs


---

## Support

For issues, questions, or contributions, please visit the project repository.

**Version:** 2.0  
**Last Updated:** January 2026

---

## Quick Reference Card

### Compilation & Execution
```bash
clangax source.cax -o program    # Compile
caxvm program.caxb               # Run
caxdis program.caxb              # Disassemble
```

### Basic Syntax
```cax
#import "_math"              // Import library
func(Main) { }              // Main function
x = 42;                     // Variable
arr = [1, 2, 3];           // Array
if (x > 0) { }             // Conditional
for (i = 0; i < 10; i++) { } // Loop
print(x);                  // Output
```

### Library Prefixes
- `_math` - Mathematical operations
- `_datastr` - Data structures
- `_alg` - Algorithms
- `_ml` - Machine learning
