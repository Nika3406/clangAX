#ifndef CLANGAX_LIBRARY_H
#define CLANGAX_LIBRARY_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "ast.h"

namespace ClangAX {

// Library types
enum class LibraryType {
    BUILTIN,    // Built-in compiler library (_math, _io, etc.)
    USER        // User-created .cax library
};

// Function signature for library functions
struct LibraryFunction {
    std::string name;
    std::string category;  // e.g., "_math", "_io"
    std::vector<std::string> parameters;
    std::shared_ptr<ASTNode> body;  // For user libraries
    bool isBuiltin;

    LibraryFunction(const std::string& n, const std::string& cat, bool builtin = false)
        : name(n), category(cat), isBuiltin(builtin) {}
};

// Library definition
struct Library {
    std::string name;
    LibraryType type;
    std::map<std::string, std::shared_ptr<LibraryFunction>> functions;

    Library(const std::string& n, LibraryType t)
        : name(n), type(t) {}

    void addFunction(std::shared_ptr<LibraryFunction> func) {
        functions[func->name] = func;
    }

    std::shared_ptr<LibraryFunction> getFunction(const std::string& name) {
        auto it = functions.find(name);
        return (it != functions.end()) ? it->second : nullptr;
    }
};

// Library manager - handles all library operations
class LibraryManager {
private:
    std::map<std::string, std::shared_ptr<Library>> libraries;
    std::string stdlibPath;  // Path to standard library directory

    void initializeBuiltinLibraries();
    std::shared_ptr<Library> loadUserLibrary(const std::string& path);

public:
    LibraryManager() {
        initializeBuiltinLibraries();
    }

    void setStdlibPath(const std::string& path) {
        stdlibPath = path;
    }

    // Import a library (either builtin or user)
    std::shared_ptr<Library> importLibrary(const std::string& name);

    // Check if a library is loaded
    bool isLibraryLoaded(const std::string& name) const {
        return libraries.find(name) != libraries.end();
    }

    // Get a loaded library
    std::shared_ptr<Library> getLibrary(const std::string& name) {
        auto it = libraries.find(name);
        return (it != libraries.end()) ? it->second : nullptr;
    }

    // Get all loaded libraries
    const std::map<std::string, std::shared_ptr<Library>>& getLoadedLibraries() const {
        return libraries;
    }
};

// Built-in library definitions
namespace BuiltinLibraries {

// =============================================================================
// _MATH LIBRARY - Mathematical operations
// =============================================================================
inline std::shared_ptr<Library> createMathLibrary() {
    auto lib = std::make_shared<Library>("_math", LibraryType::BUILTIN);

    // Basic operations
    auto sqrt = std::make_shared<LibraryFunction>("sqrt", "_math", true);
    sqrt->parameters = {"x"};
    lib->addFunction(sqrt);

    auto pow = std::make_shared<LibraryFunction>("pow", "_math", true);
    pow->parameters = {"base", "exp"};
    lib->addFunction(pow);

    auto abs = std::make_shared<LibraryFunction>("abs", "_math", true);
    abs->parameters = {"x"};
    lib->addFunction(abs);

    // Trigonometric functions
    auto sin = std::make_shared<LibraryFunction>("sin", "_math", true);
    sin->parameters = {"x"};
    lib->addFunction(sin);

    auto cos = std::make_shared<LibraryFunction>("cos", "_math", true);
    cos->parameters = {"x"};
    lib->addFunction(cos);

    auto tan = std::make_shared<LibraryFunction>("tan", "_math", true);
    tan->parameters = {"x"};
    lib->addFunction(tan);

    auto asin = std::make_shared<LibraryFunction>("asin", "_math", true);
    asin->parameters = {"x"};
    lib->addFunction(asin);

    auto acos = std::make_shared<LibraryFunction>("acos", "_math", true);
    acos->parameters = {"x"};
    lib->addFunction(acos);

    auto atan = std::make_shared<LibraryFunction>("atan", "_math", true);
    atan->parameters = {"x"};
    lib->addFunction(atan);

    auto atan2 = std::make_shared<LibraryFunction>("atan2", "_math", true);
    atan2->parameters = {"y", "x"};
    lib->addFunction(atan2);

    // Hyperbolic functions
    auto sinh = std::make_shared<LibraryFunction>("sinh", "_math", true);
    sinh->parameters = {"x"};
    lib->addFunction(sinh);

    auto cosh = std::make_shared<LibraryFunction>("cosh", "_math", true);
    cosh->parameters = {"x"};
    lib->addFunction(cosh);

    auto tanh = std::make_shared<LibraryFunction>("tanh", "_math", true);
    tanh->parameters = {"x"};
    lib->addFunction(tanh);

    // Exponential and logarithmic
    auto exp = std::make_shared<LibraryFunction>("exp", "_math", true);
    exp->parameters = {"x"};
    lib->addFunction(exp);

    auto log = std::make_shared<LibraryFunction>("log", "_math", true);
    log->parameters = {"x"};
    lib->addFunction(log);

    auto log10 = std::make_shared<LibraryFunction>("log10", "_math", true);
    log10->parameters = {"x"};
    lib->addFunction(log10);

    auto log2 = std::make_shared<LibraryFunction>("log2", "_math", true);
    log2->parameters = {"x"};
    lib->addFunction(log2);

    // Rounding and remainder
    auto floor = std::make_shared<LibraryFunction>("floor", "_math", true);
    floor->parameters = {"x"};
    lib->addFunction(floor);

    auto ceil = std::make_shared<LibraryFunction>("ceil", "_math", true);
    ceil->parameters = {"x"};
    lib->addFunction(ceil);

    auto round = std::make_shared<LibraryFunction>("round", "_math", true);
    round->parameters = {"x"};
    lib->addFunction(round);

    auto trunc = std::make_shared<LibraryFunction>("trunc", "_math", true);
    trunc->parameters = {"x"};
    lib->addFunction(trunc);

    auto fmod = std::make_shared<LibraryFunction>("fmod", "_math", true);
    fmod->parameters = {"x", "y"};
    lib->addFunction(fmod);

    // Min/Max
    auto min = std::make_shared<LibraryFunction>("min", "_math", true);
    min->parameters = {"a", "b"};
    lib->addFunction(min);

    auto max = std::make_shared<LibraryFunction>("max", "_math", true);
    max->parameters = {"a", "b"};
    lib->addFunction(max);

    // Special functions
    auto hypot = std::make_shared<LibraryFunction>("hypot", "_math", true);
    hypot->parameters = {"x", "y"};
    lib->addFunction(hypot);

    auto cbrt = std::make_shared<LibraryFunction>("cbrt", "_math", true);
    cbrt->parameters = {"x"};
    lib->addFunction(cbrt);

    // Algebra
    auto quad = std::make_shared<LibraryFunction>("quad", "_math", true);
    quad->parameters = {"a", "b", "c"};
    lib->addFunction(quad);

    auto gcd = std::make_shared<LibraryFunction>("gcd", "_math", true);
    gcd->parameters = {"a", "b"};
    lib->addFunction(gcd);

    auto lcm = std::make_shared<LibraryFunction>("lcm", "_math", true);
    lcm->parameters = {"a", "b"};
    lib->addFunction(lcm);

    auto factorial = std::make_shared<LibraryFunction>("factorial", "_math", true);
    factorial->parameters = {"n"};
    lib->addFunction(factorial);

    // Statistics
    auto mean = std::make_shared<LibraryFunction>("mean", "_math", true);
    mean->parameters = {"arr"};
    lib->addFunction(mean);

    auto median = std::make_shared<LibraryFunction>("median", "_math", true);
    median->parameters = {"arr"};
    lib->addFunction(median);

    auto stddev = std::make_shared<LibraryFunction>("stddev", "_math", true);
    stddev->parameters = {"arr"};
    lib->addFunction(stddev);

    auto variance = std::make_shared<LibraryFunction>("variance", "_math", true);
    variance->parameters = {"arr"};
    lib->addFunction(variance);

    // Calculus approximations
    auto derivative = std::make_shared<LibraryFunction>("derivative", "_math", true);
    derivative->parameters = {"arr", "h"};
    lib->addFunction(derivative);

    auto integral = std::make_shared<LibraryFunction>("integral", "_math", true);
    integral->parameters = {"arr", "dx"};
    lib->addFunction(integral);

    return lib;
}

// =============================================================================
// _DATASTR LIBRARY - Data structures
// =============================================================================
inline std::shared_ptr<Library> createDatastrLibrary() {
    auto lib = std::make_shared<Library>("_datastr", LibraryType::BUILTIN);

    // Stack operations
    auto stack_new = std::make_shared<LibraryFunction>("stack_new", "_datastr", true);
    lib->addFunction(stack_new);

    auto stack_push = std::make_shared<LibraryFunction>("stack_push", "_datastr", true);
    stack_push->parameters = {"stack", "value"};
    lib->addFunction(stack_push);

    auto stack_pop = std::make_shared<LibraryFunction>("stack_pop", "_datastr", true);
    stack_pop->parameters = {"stack"};
    lib->addFunction(stack_pop);

    auto stack_peek = std::make_shared<LibraryFunction>("stack_peek", "_datastr", true);
    stack_peek->parameters = {"stack"};
    lib->addFunction(stack_peek);

    auto stack_empty = std::make_shared<LibraryFunction>("stack_empty", "_datastr", true);
    stack_empty->parameters = {"stack"};
    lib->addFunction(stack_empty);

    // Queue operations
    auto queue_new = std::make_shared<LibraryFunction>("queue_new", "_datastr", true);
    lib->addFunction(queue_new);

    auto queue_enqueue = std::make_shared<LibraryFunction>("queue_enqueue", "_datastr", true);
    queue_enqueue->parameters = {"queue", "value"};
    lib->addFunction(queue_enqueue);

    auto queue_dequeue = std::make_shared<LibraryFunction>("queue_dequeue", "_datastr", true);
    queue_dequeue->parameters = {"queue"};
    lib->addFunction(queue_dequeue);

    auto queue_front = std::make_shared<LibraryFunction>("queue_front", "_datastr", true);
    queue_front->parameters = {"queue"};
    lib->addFunction(queue_front);

    auto queue_empty = std::make_shared<LibraryFunction>("queue_empty", "_datastr", true);
    queue_empty->parameters = {"queue"};
    lib->addFunction(queue_empty);

    // Heap/Priority Queue
    auto heap_new = std::make_shared<LibraryFunction>("heap_new", "_datastr", true);
    lib->addFunction(heap_new);

    auto heap_insert = std::make_shared<LibraryFunction>("heap_insert", "_datastr", true);
    heap_insert->parameters = {"heap", "value"};
    lib->addFunction(heap_insert);

    auto heap_extract = std::make_shared<LibraryFunction>("heap_extract", "_datastr", true);
    heap_extract->parameters = {"heap"};
    lib->addFunction(heap_extract);

    auto heap_peek = std::make_shared<LibraryFunction>("heap_peek", "_datastr", true);
    heap_peek->parameters = {"heap"};
    lib->addFunction(heap_peek);

    // Hash Map
    auto map_new = std::make_shared<LibraryFunction>("map_new", "_datastr", true);
    lib->addFunction(map_new);

    auto map_set = std::make_shared<LibraryFunction>("map_set", "_datastr", true);
    map_set->parameters = {"map", "key", "value"};
    lib->addFunction(map_set);

    auto map_get = std::make_shared<LibraryFunction>("map_get", "_datastr", true);
    map_get->parameters = {"map", "key"};
    lib->addFunction(map_get);

    auto map_has = std::make_shared<LibraryFunction>("map_has", "_datastr", true);
    map_has->parameters = {"map", "key"};
    lib->addFunction(map_has);

    auto map_delete = std::make_shared<LibraryFunction>("map_delete", "_datastr", true);
    map_delete->parameters = {"map", "key"};
    lib->addFunction(map_delete);

    auto map_keys = std::make_shared<LibraryFunction>("map_keys", "_datastr", true);
    map_keys->parameters = {"map"};
    lib->addFunction(map_keys);

    // Set
    auto set_new = std::make_shared<LibraryFunction>("set_new", "_datastr", true);
    lib->addFunction(set_new);

    auto set_add = std::make_shared<LibraryFunction>("set_add", "_datastr", true);
    set_add->parameters = {"set", "value"};
    lib->addFunction(set_add);

    auto set_has = std::make_shared<LibraryFunction>("set_has", "_datastr", true);
    set_has->parameters = {"set", "value"};
    lib->addFunction(set_has);

    auto set_delete = std::make_shared<LibraryFunction>("set_delete", "_datastr", true);
    set_delete->parameters = {"set", "value"};
    lib->addFunction(set_delete);

    auto set_union = std::make_shared<LibraryFunction>("set_union", "_datastr", true);
    set_union->parameters = {"set1", "set2"};
    lib->addFunction(set_union);

    auto set_intersect = std::make_shared<LibraryFunction>("set_intersect", "_datastr", true);
    set_intersect->parameters = {"set1", "set2"};
    lib->addFunction(set_intersect);

    // Linked List
    auto list_new = std::make_shared<LibraryFunction>("list_new", "_datastr", true);
    lib->addFunction(list_new);

    auto list_append = std::make_shared<LibraryFunction>("list_append", "_datastr", true);
    list_append->parameters = {"list", "value"};
    lib->addFunction(list_append);

    auto list_prepend = std::make_shared<LibraryFunction>("list_prepend", "_datastr", true);
    list_prepend->parameters = {"list", "value"};
    lib->addFunction(list_prepend);

    auto list_get = std::make_shared<LibraryFunction>("list_get", "_datastr", true);
    list_get->parameters = {"list", "index"};
    lib->addFunction(list_get);

    auto list_insert = std::make_shared<LibraryFunction>("list_insert", "_datastr", true);
    list_insert->parameters = {"list", "index", "value"};
    lib->addFunction(list_insert);

    auto list_remove = std::make_shared<LibraryFunction>("list_remove", "_datastr", true);
    list_remove->parameters = {"list", "index"};
    lib->addFunction(list_remove);

    // Graph
    auto graph_new = std::make_shared<LibraryFunction>("graph_new", "_datastr", true);
    lib->addFunction(graph_new);

    auto graph_add_vertex = std::make_shared<LibraryFunction>("graph_add_vertex", "_datastr", true);
    graph_add_vertex->parameters = {"graph", "vertex"};
    lib->addFunction(graph_add_vertex);

    auto graph_add_edge = std::make_shared<LibraryFunction>("graph_add_edge", "_datastr", true);
    graph_add_edge->parameters = {"graph", "from", "to", "weight"};
    lib->addFunction(graph_add_edge);

    auto graph_neighbors = std::make_shared<LibraryFunction>("graph_neighbors", "_datastr", true);
    graph_neighbors->parameters = {"graph", "vertex"};
    lib->addFunction(graph_neighbors);

    // Tree
    auto tree_new = std::make_shared<LibraryFunction>("tree_new", "_datastr", true);
    tree_new->parameters = {"value"};
    lib->addFunction(tree_new);

    auto tree_add_child = std::make_shared<LibraryFunction>("tree_add_child", "_datastr", true);
    tree_add_child->parameters = {"node", "value"};
    lib->addFunction(tree_add_child);

    auto tree_children = std::make_shared<LibraryFunction>("tree_children", "_datastr", true);
    tree_children->parameters = {"node"};
    lib->addFunction(tree_children);

    return lib;
}

// =============================================================================
// _ALG LIBRARY - Algorithms
// =============================================================================
inline std::shared_ptr<Library> createAlgLibrary() {
    auto lib = std::make_shared<Library>("_alg", LibraryType::BUILTIN);

    // Sorting algorithms
    auto quicksort = std::make_shared<LibraryFunction>("quicksort", "_alg", true);
    quicksort->parameters = {"arr"};
    lib->addFunction(quicksort);

    auto mergesort = std::make_shared<LibraryFunction>("mergesort", "_alg", true);
    mergesort->parameters = {"arr"};
    lib->addFunction(mergesort);

    auto heapsort = std::make_shared<LibraryFunction>("heapsort", "_alg", true);
    heapsort->parameters = {"arr"};
    lib->addFunction(heapsort);

    auto bubblesort = std::make_shared<LibraryFunction>("bubblesort", "_alg", true);
    bubblesort->parameters = {"arr"};
    lib->addFunction(bubblesort);

    auto insertionsort = std::make_shared<LibraryFunction>("insertionsort", "_alg", true);
    insertionsort->parameters = {"arr"};
    lib->addFunction(insertionsort);

    // Search algorithms
    auto binary_search = std::make_shared<LibraryFunction>("binary_search", "_alg", true);
    binary_search->parameters = {"arr", "target"};
    lib->addFunction(binary_search);

    auto linear_search = std::make_shared<LibraryFunction>("linear_search", "_alg", true);
    linear_search->parameters = {"arr", "target"};
    lib->addFunction(linear_search);

    // Graph algorithms
    auto bfs = std::make_shared<LibraryFunction>("bfs", "_alg", true);
    bfs->parameters = {"graph", "start"};
    lib->addFunction(bfs);

    auto dfs = std::make_shared<LibraryFunction>("dfs", "_alg", true);
    dfs->parameters = {"graph", "start"};
    lib->addFunction(dfs);

    auto dijkstra = std::make_shared<LibraryFunction>("dijkstra", "_alg", true);
    dijkstra->parameters = {"graph", "start", "end"};
    lib->addFunction(dijkstra);

    auto bellman_ford = std::make_shared<LibraryFunction>("bellman_ford", "_alg", true);
    bellman_ford->parameters = {"graph", "start"};
    lib->addFunction(bellman_ford);

    auto floyd_warshall = std::make_shared<LibraryFunction>("floyd_warshall", "_alg", true);
    floyd_warshall->parameters = {"graph"};
    lib->addFunction(floyd_warshall);

    auto kruskal = std::make_shared<LibraryFunction>("kruskal", "_alg", true);
    kruskal->parameters = {"graph"};
    lib->addFunction(kruskal);

    auto prim = std::make_shared<LibraryFunction>("prim", "_alg", true);
    prim->parameters = {"graph"};
    lib->addFunction(prim);

    // Dynamic programming
    auto lcs = std::make_shared<LibraryFunction>("lcs", "_alg", true);
    lcs->parameters = {"s1", "s2"};
    lib->addFunction(lcs);

    auto knapsack = std::make_shared<LibraryFunction>("knapsack", "_alg", true);
    knapsack->parameters = {"weights", "values", "capacity"};
    lib->addFunction(knapsack);

    auto edit_distance = std::make_shared<LibraryFunction>("edit_distance", "_alg", true);
    edit_distance->parameters = {"s1", "s2"};
    lib->addFunction(edit_distance);

    // String algorithms
    auto kmp_search = std::make_shared<LibraryFunction>("kmp_search", "_alg", true);
    kmp_search->parameters = {"text", "pattern"};
    lib->addFunction(kmp_search);

    auto rabin_karp = std::make_shared<LibraryFunction>("rabin_karp", "_alg", true);
    rabin_karp->parameters = {"text", "pattern"};
    lib->addFunction(rabin_karp);

    // Other algorithms
    auto permutations = std::make_shared<LibraryFunction>("permutations", "_alg", true);
    permutations->parameters = {"arr"};
    lib->addFunction(permutations);

    auto combinations = std::make_shared<LibraryFunction>("combinations", "_alg", true);
    combinations->parameters = {"arr", "k"};
    lib->addFunction(combinations);

    auto shuffle = std::make_shared<LibraryFunction>("shuffle", "_alg", true);
    shuffle->parameters = {"arr"};
    lib->addFunction(shuffle);

    return lib;
}

// =============================================================================
// _ML LIBRARY - Machine Learning
// =============================================================================
inline std::shared_ptr<Library> createMLLibrary() {
    auto lib = std::make_shared<Library>("_ml", LibraryType::BUILTIN);

    // Linear Regression
    auto linear_fit = std::make_shared<LibraryFunction>("linear_fit", "_ml", true);
    linear_fit->parameters = {"x", "y"};
    lib->addFunction(linear_fit);

    auto linear_predict = std::make_shared<LibraryFunction>("linear_predict", "_ml", true);
    linear_predict->parameters = {"model", "x"};
    lib->addFunction(linear_predict);

    // Polynomial Regression
    auto poly_fit = std::make_shared<LibraryFunction>("poly_fit", "_ml", true);
    poly_fit->parameters = {"x", "y", "degree"};
    lib->addFunction(poly_fit);

    auto poly_predict = std::make_shared<LibraryFunction>("poly_predict", "_ml", true);
    poly_predict->parameters = {"model", "x"};
    lib->addFunction(poly_predict);

    // Logistic Regression
    auto logistic_fit = std::make_shared<LibraryFunction>("logistic_fit", "_ml", true);
    logistic_fit->parameters = {"x", "y", "iterations", "learning_rate"};
    lib->addFunction(logistic_fit);

    auto logistic_predict = std::make_shared<LibraryFunction>("logistic_predict", "_ml", true);
    logistic_predict->parameters = {"model", "x"};
    lib->addFunction(logistic_predict);

    // K-Nearest Neighbors
    auto knn_fit = std::make_shared<LibraryFunction>("knn_fit", "_ml", true);
    knn_fit->parameters = {"x", "y", "k"};
    lib->addFunction(knn_fit);

    auto knn_predict = std::make_shared<LibraryFunction>("knn_predict", "_ml", true);
    knn_predict->parameters = {"model", "x"};
    lib->addFunction(knn_predict);

    // K-Means Clustering
    auto kmeans_fit = std::make_shared<LibraryFunction>("kmeans_fit", "_ml", true);
    kmeans_fit->parameters = {"data", "k", "iterations"};
    lib->addFunction(kmeans_fit);

    auto kmeans_predict = std::make_shared<LibraryFunction>("kmeans_predict", "_ml", true);
    kmeans_predict->parameters = {"model", "point"};
    lib->addFunction(kmeans_predict);

    // Decision Tree
    auto dtree_fit = std::make_shared<LibraryFunction>("dtree_fit", "_ml", true);
    dtree_fit->parameters = {"x", "y", "max_depth"};
    lib->addFunction(dtree_fit);

    auto dtree_predict = std::make_shared<LibraryFunction>("dtree_predict", "_ml", true);
    dtree_predict->parameters = {"model", "x"};
    lib->addFunction(dtree_predict);

    // Neural Network (simple)
    auto nn_new = std::make_shared<LibraryFunction>("nn_new", "_ml", true);
    nn_new->parameters = {"layers"};
    lib->addFunction(nn_new);

    auto nn_train = std::make_shared<LibraryFunction>("nn_train", "_ml", true);
    nn_train->parameters = {"model", "x", "y", "epochs", "learning_rate"};
    lib->addFunction(nn_train);

    auto nn_predict = std::make_shared<LibraryFunction>("nn_predict", "_ml", true);
    nn_predict->parameters = {"model", "x"};
    lib->addFunction(nn_predict);

    // Activation functions
    auto sigmoid = std::make_shared<LibraryFunction>("sigmoid", "_ml", true);
    sigmoid->parameters = {"x"};
    lib->addFunction(sigmoid);

    auto relu = std::make_shared<LibraryFunction>("relu", "_ml", true);
    relu->parameters = {"x"};
    lib->addFunction(relu);

    auto tanh_act = std::make_shared<LibraryFunction>("tanh_act", "_ml", true);
    tanh_act->parameters = {"x"};
    lib->addFunction(tanh_act);

    auto softmax = std::make_shared<LibraryFunction>("softmax", "_ml", true);
    softmax->parameters = {"arr"};
    lib->addFunction(softmax);

    // Loss functions
    auto mse = std::make_shared<LibraryFunction>("mse", "_ml", true);
    mse->parameters = {"y_true", "y_pred"};
    lib->addFunction(mse);

    auto cross_entropy = std::make_shared<LibraryFunction>("cross_entropy", "_ml", true);
    cross_entropy->parameters = {"y_true", "y_pred"};
    lib->addFunction(cross_entropy);

    // Metrics
    auto r2_score = std::make_shared<LibraryFunction>("r2_score", "_ml", true);
    r2_score->parameters = {"y_true", "y_pred"};
    lib->addFunction(r2_score);

    auto accuracy = std::make_shared<LibraryFunction>("accuracy", "_ml", true);
    accuracy->parameters = {"y_true", "y_pred"};
    lib->addFunction(accuracy);

    auto confusion_matrix = std::make_shared<LibraryFunction>("confusion_matrix", "_ml", true);
    confusion_matrix->parameters = {"y_true", "y_pred"};
    lib->addFunction(confusion_matrix);

    // Data preprocessing
    auto normalize = std::make_shared<LibraryFunction>("normalize", "_ml", true);
    normalize->parameters = {"data"};
    lib->addFunction(normalize);

    auto standardize = std::make_shared<LibraryFunction>("standardize", "_ml", true);
    standardize->parameters = {"data"};
    lib->addFunction(standardize);

    auto train_test_split = std::make_shared<LibraryFunction>("train_test_split", "_ml", true);
    train_test_split->parameters = {"x", "y", "test_size"};
    lib->addFunction(train_test_split);

    return lib;
}

} // namespace BuiltinLibraries

} // namespace ClangAX

#endif // CLANGAX_LIBRARY_H