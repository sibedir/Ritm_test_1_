// Windows 10
// Visual Studio 2022
// C++20

#define TEST_MODE

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>
#include <type_traits>
#include <concepts>
#include <limits>
#include <functional>
#include <unordered_map>

/* my_graph.h --------------------------------------------------------------------------------------------- */
// описание графа

// TAttribute

template <typename value_type_>
struct TAttributeSpec {
public:
    using value_type = std::remove_reference_t<value_type_>;
private:
    struct TAEmpty {};

    template <typename T>
    struct TAValue {
        T attribute;
        TAValue(T attribute) : attribute(attribute) {}
    };

    template <typename T>
    struct TARef : public std::unique_ptr<T> {
        using TUniquePtr = std::unique_ptr<T>;
        TARef() : TUniquePtr(new T()) {}
        T& attribute() { return TUniquePtr::operator*(); };
    };

    enum atr_type { atEmpty = 0, atSimple, atRef, atFunc, atClass };

    static constexpr atr_type signal = std::is_void_v<value_type>
        ? atEmpty
        : (std::is_fundamental_v<value_type> or std::is_pointer_v<value_type>
            ? atSimple
            : (std::is_function_v<value_type>
                ? atFunc
                : atClass)
            );
                
    template <atr_type> struct Spec           { using type = TAEmpty             ; };
    template <        > struct Spec<atSimple> { using type = TAValue<value_type >; };
    template <        > struct Spec<atFunc  > { using type = TAValue<value_type*>; };
    template <        > struct Spec<atClass > { using type = TARef  <value_type >; };
public:
    using type = Spec<signal>::type;
};

template <typename value_type>
using TAttribute = TAttributeSpec<value_type>::type;

// TGraph

enum TAnnotatedSpec { asNone = 0, asVert = 1, asEdge = 2, asAll = asVert | asEdge };

template <TAnnotatedSpec annotated_spec>
constexpr bool IsVertAnnotated = static_cast<bool>(annotated_spec & asVert);

template <TAnnotatedSpec annotated_spec>
constexpr bool IsEdgeAnnotated = static_cast<bool>(annotated_spec & asEdge);

template <typename value_type, bool is_vert> struct TVertAnnotatedSpec                   { using arrt_value_type = void      ; };
template <typename value_type              > struct TVertAnnotatedSpec<value_type, true> { using arrt_value_type = value_type; };

template <typename value_type, bool is_edge> struct TEdgeAnnotatedSpec                   { using arrt_value_type = void      ; };
template <typename value_type              > struct TEdgeAnnotatedSpec<value_type, true> { using arrt_value_type = value_type; };

template <typename value_type, TAnnotatedSpec annotated_spec>
using TVertAttrValueType = TVertAnnotatedSpec<value_type, IsVertAnnotated<annotated_spec>>::arrt_value_type;

template <typename value_type, TAnnotatedSpec annotated_spec>
using TEdgeAttrValueType = TEdgeAnnotatedSpec<value_type, IsEdgeAnnotated<annotated_spec>>::arrt_value_type;

template <std::unsigned_integral idx_type_ = size_t, typename attr_value_type_ = void, TAnnotatedSpec annotated_spec = asNone>
class TGraph_ {
#ifdef TEST_MODE
    friend int test_main();
#endif // TEST_MODE
public:
    using idx_type = idx_type_;
    static constexpr idx_type const BAD_IDX = std::numeric_limits<idx_type>::max();
    using attr_value_type = attr_value_type_;
    using v_attr_value_t = TVertAttrValueType<attr_value_type_, annotated_spec>;
    using e_attr_value_t = TEdgeAttrValueType<attr_value_type_, annotated_spec>;

    using TVertAttrBase = TAttribute<v_attr_value_t>;
    using TEdgeAttrBase = TAttribute<e_attr_value_t>;
private:

    // TVertex
    struct TVertViewerBase {
        idx_type const first_input ;
        idx_type const first_output;
    };

    struct TVertBase {
        idx_type first_input { BAD_IDX };
        idx_type first_output{ BAD_IDX };
    };

    struct TVertViewer : public TVertViewerBase, public TVertAttrBase {};

    struct TVert : public TVertBase, public TVertAttrBase {
        TVert() : TVertBase(), TVertAttrBase() {};

        decltype(auto) view()       { return reinterpret_cast<TVertViewer      &>(*this); }
        decltype(auto) view() const { return reinterpret_cast<TVertViewer const&>(*this); }
    };

    struct TVertArrViewer {
    private:
        TGraph_& graph;
    public:
        TVertArrViewer() = delete;
        TVertArrViewer(TGraph_& graph) : graph(graph) {};
        idx_type size() { return static_cast<idx_type>(graph.vert_arr.size()); }
        decltype(auto) operator[] (idx_type idx)       { return graph.vert_arr[idx].view(); }
        decltype(auto) operator[] (idx_type idx) const { return graph.vert_arr[idx].view(); }
    };

    // TEdge
    struct TEdgeViewerBase {
        idx_type const from;
        idx_type const to;
        idx_type const next_from;
        idx_type const next_to;
    };

    struct TEdgeBase {
        idx_type from;
        idx_type to  ;
        idx_type next_from;
        idx_type next_to;

        TEdgeBase(idx_type from, idx_type to, idx_type next_from, idx_type next_to)
            : from(from)
            , to(to)
            , next_from(next_from)
            , next_to(next_to)
        {};
    };

    struct TEdgeViewer : public TEdgeViewerBase, public TEdgeAttrBase {};

    struct TEdge : public TEdgeBase, public TEdgeAttrBase {
        TEdge() = delete;
        TEdge(idx_type from, idx_type to, idx_type next_from, idx_type next_to)
            : TEdgeBase(from, to, next_from, next_to)
            , TEdgeAttrBase()
        {};

        decltype(auto) view()       { return reinterpret_cast<TEdgeViewer      &>(*this); }
        decltype(auto) view() const { return reinterpret_cast<TEdgeViewer const&>(*this); }
    };

    struct TEdgeArrViewer {
    private:
        TGraph_& graph;
    public:
        TEdgeArrViewer() = delete;
        TEdgeArrViewer(TGraph_& graph) : graph(graph) {};
        idx_type size() { return static_cast<idx_type>(graph._edges.size()); }
        decltype(auto) operator[] (idx_type idx)       { return graph.edge_arr[idx].view(); }
        decltype(auto) operator[] (idx_type idx) const { return graph.edge_arr[idx].view(); }
    };

    std::vector<TVert> vert_arr;
    std::vector<TEdge> edge_arr;

public:
    TVertArrViewer vertex;
    TEdgeArrViewer edge;

    TGraph_() : vert_arr(), edge_arr(), edge(*this), vertex(*this) {}

    void AddVertexes(idx_type count) {
        vert_arr.resize(vert_arr.size() + count);
    }

    TVert& AddVertex() {
        return vert_arr.emplace_back();
    }

    TEdge& AddEdge(idx_type from, idx_type to) {
        if (from >= vertex.size() or to >= vertex.size()) {
            throw std::invalid_argument("Error in AddEdge(from, to): from|to >= vertex.size()");
        }
        idx_type& vffo = vert_arr[from].first_output;
        idx_type& vtfi = vert_arr[to  ].first_input ;
        idx_type next_from = vffo;
        idx_type next_to   = vtfi;
        idx_type self = static_cast<idx_type>(edge_arr.size());
        vffo = self;
        vtfi = self;
        return edge_arr.emplace_back(from, to, next_from, next_to);
    }
};

template <std::integral idx_type = size_t>
using TGraph = TGraph_<std::make_unsigned_t<idx_type>>;

template <typename attr_value_type, TAnnotatedSpec annotated_spec = asAll, std::integral idx_type = size_t>
using TAnnotatedGraph = TGraph_<std::make_unsigned_t<idx_type>, attr_value_type, annotated_spec>;

/* agent_function.h --------------------------------------------------------------------------------------- */
// описание агент-функции

template <
    typename target_graph_type,
    std::unsigned_integral rules_idx_t_    = typename target_graph_type::idx_type,
    std::unsigned_integral func_arg_idx_t_ = unsigned short
>
class TRules {
public:
    using value_type = target_graph_type::attr_value_type;
    using link_idx_t = target_graph_type::idx_type;
    using rules_idx_t = rules_idx_t_;
    using func_arg_idx_t = func_arg_idx_t_;

    using TRuleFuncArgs = std::vector<value_type>;
    using TRuleFunc = std::function<value_type(TRuleFuncArgs const&)>;
private:
    struct TRuleFuncSpec {
        TRuleFunc func;
        func_arg_idx_t arg_count{ 0 };
    };

    using TRuleFuncSpecMap = std::unordered_map<std::string, TRuleFuncSpec>;

    enum class Graph_Elem_Type { Vert, Edge };

    static constexpr Graph_Elem_Type t = Graph_Elem_Type::Vert;

    enum TRuleType { rtNone, rtValue, rtVertLink, rtEdgeLink, rtFunc };

    template<Graph_Elem_Type type> struct TRuleLinkSpec                        {                                               };
    template<                    > struct TRuleLinkSpec<Graph_Elem_Type::Vert> { static constexpr TRuleType type = rtVertLink; };
    template<                    > struct TRuleLinkSpec<Graph_Elem_Type::Edge> { static constexpr TRuleType type = rtEdgeLink; };

    struct TRule {
        TRuleType type{ rtNone };
        union {
            value_type value;
            link_idx_t idx;
            TRuleFuncSpec const* func{ nullptr };
        };

        TRule() = default;

        explicit TRule(value_type val)
            : type { rtValue }
            , value{ val }
        {};

        template <typename graph_type>
        explicit TRule(Graph_Elem_Type type, link_idx_t idx)
            : type{ TRuleLinkSpec<type>::type }
            , idx { idx }
        {};

        explicit TRule(TRuleFuncSpecMap const& func_spec, std::string const& func_name)
            : type{ rtFunc }
        {
            auto tmp = func_spec.find(func_name);
            if (tmp == func_spec.end()) throw std::runtime_error("Unknown function: \"" + func_name + "\"");
            func = &(tmp->second);
        };
    };

private:
    TRuleFuncSpecMap functions_specification;
    TAnnotatedGraph<TRule, asVert, rules_idx_t> rules;
public:
    TRules() : functions_specification(), rules() {};

    void RegFunc(std::string name, func_arg_idx_t arg_count, TRuleFunc const& func) {
        TRuleFuncSpec& rfs = functions_specification[name];
        rfs.func = func;
        rfs.arg_count = arg_count;
    }

    void Add_Value(value_type val) {
        auto& r = rules.AddVertex();
        r.attribute().type = rtValue;
        r.attribute().value = val;
    }

    void Add_Function(std::string func_name) {
        auto& r = rules.AddVertex();
        r.attribute() = TRule(functions_specification, func_name);
    }

    void Add_VertexLink(link_idx_t idx) {
        auto& r = rules.AddVertex();
        r.attribute().type = rtVertLink;
        r.attribute().idx  = idx;
    }

    void Add_EdgeLink(link_idx_t idx) {
        auto& r = rules.AddVertex();
        r.attribute().type = rtEdgeLink;
        r.attribute().idx = idx;
    }
};

/*
// чтение строки с правилом агент-функции
TRuleBase& ReadRule(std::istream& IN) {
    TRuleBase& result;
    std::string buf;
    IN >> buf;
    if (buf == "min") {
        result.type = rtFunc;
        result.func.type = rftMin;
    };
    if (buf == "*") {
        result.type = rtFunc;
        result.func.type = rftProduct;
    };
    if (result.type == rtFunc) {
        IN >> buf;
        result.func.arg1.elemtype = (buf == "v") ? etVertix : etEdge;
        IN >> result.func.arg1.elemindex;
        result.func.arg1.elemindex--;
        IN >> buf;
        result.func.arg2.elemtype = (buf == "v") ? etVertix : etEdge;
        IN >> result.func.arg2.elemindex;
        result.func.arg2.elemindex--;
        return result;
    }

    if (buf == "v") {
        result.type = rtCopy;
        result.copy.elemtype = etVertix;
    };
    if (buf == "e") {
        result.type = rtCopy;
        result.copy.elemtype = etEdge;
    };
    if (result.type == rtCopy) {
        IN >> result.copy.elemindex;
        result.copy.elemindex--;
        return result;
    }

    result.type = rtValue;
    result.value = std::stof(buf);
    return result;
}

/* вспомогательные типы и методы для вычисления значений атрибутов ---------------------------------------- */
enum TState { sEmpty, sCalc, sReady };
/*
float CalculateRules(
    const TRule& rule,
    TAnnotatedGraph<float>& graph,
    std::vector<TState>& VertexState,
    std::vector<TState>& EdgeState,
    const std::vector<TRule>& VertexRules,
    const std::vector<TRule>& EdgeRules)
{
    if (rule.type == rtValue) return rule.value;

    if (rule.type == rtCopy) {
        if (rule.copy.elemtype == etVertix) {
            size_t i = rule.copy.elemindex;
            TState s = VertexState[i];
            if (s == sCalc) throw std::logic_error("Circular reference detected");
            if (s == sReady) return graph.vertex[i].atr;

            VertexState[i] = sCalc;
            auto res = CalculateRules(VertexRules[i], graph, VertexState, EdgeState, VertexRules, EdgeRules);
            graph.vertex[i].atr = res;
            VertexState[i] = sReady;
            return res;
        }
        else {
            size_t i = rule.copy.elemindex;
            TState s = EdgeState[i];
            if (s == sCalc) throw std::logic_error("Circular reference detected");
            if (s == sReady) return graph.edge[i].atr;

            EdgeState[i] = sCalc;
            auto res = CalculateRules(EdgeRules[i], graph, VertexState, EdgeState, VertexRules, EdgeRules);
            graph.edge[i].atr = res;
            EdgeState[i] = sReady;
            return res;
        }
    }
    TRule r;
    r.type = rtCopy; r.copy = rule.func.arg1;
    auto arg1 = CalculateRules(r, graph, VertexState, EdgeState, VertexRules, EdgeRules);
    r.type = rtCopy; r.copy = rule.func.arg2;
    auto arg2 = CalculateRules(r, graph, VertexState, EdgeState, VertexRules, EdgeRules);
    switch (rule.func.type) {
    case rftMin    : return std::min(arg1, arg2);
    case rftProduct: return arg1 * arg2;
    default: throw std::invalid_argument("Unknown agent-function");
    }
}
*/

/* ******************************************************************************************************** */
/*                                   ВСПОМОГАТЕЛЬНЫЕ ТИПЫ И МЕТОДЫ                                          */
/* ******************************************************************************************************** */

// исключения

class EAbort : public std::runtime_error {
public:
    using _Mybase = runtime_error;
private:
    int _exit_code;

    explicit EAbort(std::string const& _Message, int exit_code) : _Mybase(_Message.c_str()), _exit_code(exit_code) {}
    explicit EAbort(char        const* _Message, int exit_code) : _Mybase(_Message), _exit_code(exit_code) {}

public:
    [[nodiscard]] virtual int exit_code() const { return _exit_code; }

    [[noreturn]] static inline void throw_abort(const std::string& message, int exit_code) { throw EAbort(message, exit_code); }
    [[noreturn]] static inline void throw_abort(char        const* message, int exit_code) { throw EAbort(message, exit_code); }
};

[[noreturn]] inline void throw_abort(const std::string& message, int exit_code) { EAbort::throw_abort(message, exit_code); }
[[noreturn]] inline void throw_abort(char        const* message, int exit_code) { EAbort::throw_abort(message, exit_code); }

// чтение одного значения из строкового потока
template <typename T>
inline void ReadValue(std::istringstream& IN, T& val) {
    while (IN.get() == ' ') {}
    if (IN.fail()) throw std::runtime_error("Unexpected line termination");
    IN.unget();
    IN >> val;
    if (IN.fail()) throw std::runtime_error("Invalid format");
}

// запись одного значения в поток
template <typename T>
inline void ReadValue(std::istream& OUT, T& val) {
    OUT << val;
}

// поток ввода-вывода
class TInOut {
private:
    bool is_console{ true };
    std::ifstream* fin{ nullptr };
    std::ofstream* fout{ nullptr };
    size_t input_line{ 0 };

public:
    //bool IsConsole() const { return is_console; };
    std::istream& IN () { return is_console ? std::cin  : *fin ; };
    std::ostream& OUT() { return is_console ? std::cout : *fout; };
    size_t CurInputLine() const { return input_line; };

    TInOut(std::string const& input_file) {
        is_console = (input_file == "");
        if (!is_console) {
            try {
                fin = new std::ifstream;
                fin->open(input_file);
                if (fin->fail()) throw_abort("Ошибка при открытия файла: \"" + input_file + "\"", 1);

                fout = new std::ofstream;
                fout->open(input_file + ".out");
                if (fout->fail()) throw_abort("Ошибка при открытия файла: \"" + input_file + ".out\"", 1);
            }
            catch (...) {
                delete fin;
                delete fout;
                throw;
            }
        }
    }

    ~TInOut() {
        if (!is_console) {
            delete fin;
            delete fout;
        }
    }

    template <typename T>
    std::ostream& operator<<(T val) { return OUT() << val; }

    // чтение строки потока в строковый поток
    [[nodiscard]] std::istringstream ReadLine() {
        std::string s_buf;
        std::getline(IN(), s_buf);
        ++input_line;
        return std::istringstream(std::move(s_buf));
    }

    void IgnorLine() {
        std::string s_buf;
        std::getline(IN(), s_buf);
        ++input_line;
    }

    // чтение произвольного числа аргументов из одной строки потока
    template <typename... Ts>
    void ReadLine(Ts&&... args) {
        try {
            auto line = ReadLine();
            (ReadValue(line, args), ...);
        }
        catch (const std::exception& exc) {
            throw_abort("Error in line = " + std::to_string(input_line) + ". " + exc.what(), 2);
        }
    }
};

/* ******************************************************************************************************** */
/*                                   ОСНОВНОЙ КОД ВЫПОЛНЕНИЯ ЗАДАЧИ                                         */
/* ******************************************************************************************************** */

[[nodiscard]] int complete_task(TInOut& IO) {

    using TGraph = TAnnotatedGraph<float, asAll>;

    try {
        /* Ввод размеров графа */
        std::cout << "Entering sizes...\n";

        size_t NV, NE;
        IO.ReadLine(NV, NE);
        TGraph graph;
        graph.AddVertexes(NV);
        IO.IgnorLine();

        /* Ввод рёбер */
        std::cout << "Entering edges...\n";

        for (size_t i = 0; i < NE; i++) {
            size_t vi, vo;
            IO.ReadLine(vi, vo);

            try {
                graph.AddEdge(vi - 1, vo - 1);
            }
            catch (const std::exception& exc) {
                throw_abort(std::string("Incorrect data.\n") + exc.what(), 3);
            }
        }

#if 0
        /* Read rules of vertex */
        std::string s_buf;
        if (is_console) OUT << "Enter rules:\n";
        else            TRY_READ_LINE(std::string()); // пропуск строки

        std::vector<TRule> VertexRules(NV);
        for (auto& r : VertexRules)
        {
            r = ReadRule(IN);
            std::getline(IN, s_buf);
        }

        /* Read rules of edge */
        std::vector<TRuleBase*> EdgeRules(NE);
        for (auto& r : EdgeRules)
        {
            r = ReadRule(IN);
            std::getline(IN, s_buf);
        }

        /* Calculate the rules */
        std::vector<TState> VertexState(NV, sEmpty);
        std::vector<TState> EdgeState(NE, sEmpty);

        for (size_t i = 0; i < NV; i++) {
            if (VertexState[i] == sReady) continue;
            try {
                graph.vertex[i].atr = CalculateRules(VertexRules[i], graph, VertexState, EdgeState, VertexRules, EdgeRules);
            }
            catch (const std::exception& exc) {
                throw_abort("Attribute of vertex[" + std::to_string(i + 1) + "] cannot be calculated.\n" + exc.what(), 3);
            }
            VertexState[i] = sReady;
        }

        for (size_t i = 0; i < NE; i++) {
            if (EdgeState[i] == sReady) continue;
            try {
                graph.edge[i].atr = CalculateRules(EdgeRules[i], graph, VertexState, EdgeState, VertexRules, EdgeRules);
            }
            catch (const std::exception& exc) {
                throw_abort("Attribute of edge[" + std::to_string(i + 1) + "] cannot be calculated.\n" + exc.what(), 3);
            }
            EdgeState[i] = sReady;
        }
#else
        using TRules = TRules<TGraph>;
        TRules rules{};
        rules.RegFunc("+", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] + v[1]; });
        rules.RegFunc("*", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] * v[1]; });

        rules.Add_Value(7);
        rules.Add_Function("+");
        //rules.Add_Function("-");
        rules.Add_Function("*");
        rules.Add_VertexLink(12);
        rules.Add_VertexLink(10);
        rules.Add_EdgeLink(77);


#endif

        /* Print */
        std::cout << "\nOutputting results...\n";

        for (size_t i = 0; i < NV; i++) {
            IO << graph.vertex[i].attribute << std::endl;
        }
        for (size_t i = 0; i < NE; i++) {
            IO << graph.edge[i].attribute << std::endl;
        }
        
        std::cout << "...READY\n";
    }
    catch (const EAbort& exc) {
        std::cerr << std::endl << exc.what() << std::endl;
        return exc.exit_code();
    }

    return 0;
}

[[nodiscard]] int complete_task_by_file(std::string const& fin_name) {
    TInOut IO(fin_name);
    return complete_task(IO);
}

#ifdef TEST_MODE
/* ******************************************************************************************************** */
/*                                              TEST MAIN                                                   */
/* ******************************************************************************************************** */
#include <algorithm>
#include <set>
#include <list>

int c_count = 0;
int d_count = 0;

struct MyClass {
    static void print() {
        std::cout << "MyClass constr(" << c_count << "), destr(" << d_count << ")\n";
    }
    int i;
    int operator=(int val) { return i = val; }
    operator int() { return i; }
    MyClass() : i{} { ++c_count; }
    ~MyClass() { ++d_count; }
};

int foo(int a, int b) { return a + b; };

using Tfp = int(*)(int, int);
using Tf = int(int, int);

using AT = Tfp;
using IT = unsigned short;

using T = int&;

constexpr bool br = std::is_reference_v<T>;
constexpr bool bt = std::is_fundamental_v<T>;

int i = 1;
decltype(auto) up = std::make_unique<T>();

static inline int test_main()
{
    {
        TAnnotatedGraph<AT, asVert> G;
        std::cout << sizeof(decltype(G)::TVert) << "\tvert size\n";
        std::cout << sizeof(decltype(G)::TEdge) << "\tedge size\n";
        std::cout << sizeof(decltype(G)::TVertBase) << "\tbase vert size\n";
        std::cout << sizeof(decltype(G)::TEdgeBase) << "\tbase edge size\n";
        std::cout << sizeof(decltype(G)::TVertAttrBase) << "\tattr size\n";
        std::cout << sizeof(decltype(G)::TEdgeAttrBase) << "\tattr size\n";
        std::cout << typeid(decltype(G.vertex[1].attribute)).name() << "\tattr type\n";
        std::cout << std::endl;
        G.AddVertexes(4);
        G.AddEdge(1, 2); // 0
        G.AddEdge(2, 1); // 1
        G.AddEdge(2, 0); // 2
        G.AddEdge(0, 2); // 3
        //G.vertex[1].attribute() = 9999;
        G.vertex[1].attribute = foo;
        std::cout << "e[0] " << G.edge[0].from << "->" << G.edge[0].to << std::endl;
        std::cout << "v[2] " << G.vertex[2].first_output << ", " << G.vertex[2].first_input << std::endl;
        //std::cout << "v[1] = " << G.vertex[1].attribute() << std::endl;
        std::cout << "v[1](2, 3) = " << G.vertex[1].attribute(2, 3) << std::endl;
        G.AddVertexes(4000);
    }
    std::cout << std::endl;

    MyClass::print();
    std::cout << std::endl;

    try {
        std::ignore = complete_task_by_file("");
    }
    catch (const std::exception& exc) {
        std::cerr << exc.what() << std::endl;
    }
    std::cout << std::endl;

    std::cin.get();
    return 0;
}

int main() {
    setlocale(LC_ALL, "");
    return test_main();
}

#else // TEST_MODE

/* ******************************************************************************************************** */
/*                                                MAIN                                                      */
/* ******************************************************************************************************** */
constexpr char const* default_fin_name = "test.gar"; // gar - graph and rures

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");

    if (argc > 1) {
        /* Вывод справки */
        if (strcmp(argv[1], "-?") == 0) {
            std::cout
                << "Использование: " << argv[0] << " [<список файлов>] [] [-c] [-?]\n"
                << "\n"
                << "Параметры:\n"
                << "   <список файлов>   Имена файлов входных данных.\n"
                << "                     По умолчанию имя файла: \"" << default_fin_name << "\".\n"
                << "                     Имя файла результатов: \"<входной файл>.out\".\n"
                << "              [-c]   Ввод/вывод осуществляется через консоль.\n"
                << "              [-?]   Справка.\n"
                << "\n"
                << "Коды завершения:\n"
                << "                0    успешно\n"
                << "                1    ошибка доступа к файлу\n"
                << "                2    неверный формат входного файла/потока\n"
                << "                3    невалидные входные данные\n";
            return 0;
        }

        /* Ввод-вывод через консоль */
        if (strcmp(argv[1], "-c") == 0) {
            return complete_task_by_file("");
        }

        /* Ввод-вывод через список файлов */
        for (int i = 1; i < argc; ++i) {
            int res = complete_task_by_file(argv[i]);
            if (res != 0) return res;
        }
        return 0;
    }

    /* Ввод-вывод через файл по умолчанию */
    return complete_task_by_file(default_fin_name);
}

#endif // TEST_MODE