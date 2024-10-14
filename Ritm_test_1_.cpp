// Windows 10
// Visual Studio 2022
// C++20

//#define TEST_MODE

#include <iostream>
#include <utility>

#include "my_graph.h"
#include "agent_function.h"
#include "ritm_test_suppor.h"

/* ******************************************************************************************************** */
/*                                   ОСНОВНОЙ КОД ВЫПОЛНЕНИЯ ЗАДАЧИ                                         */
/* ******************************************************************************************************** */

[[nodiscard]] int complete_task(TInOut& IO) {

    using TGraph = TAnnotatedGraph<float, asAll>;
    using TRules = TRules<TGraph>;

    try {
        // Ввод размеров графа
        if (IO.IsConsole()) std::cout << "Entering sizes...\n";

        size_t NV, NE;

        try {
            IO.ReadLine(NV, NE);
        }
        catch (const std::exception& exc) { throw_abort(exc.what(), 2); }

        TGraph graph;
        graph.AddVertexes(NV);
        IO.IgnorLine();

        // Ввод рёбер
        if (IO.IsConsole()) std::cout << "Entering edges...\n";

        for (size_t i = 0; i < NE; ++i) {
            size_t vi, vo;
            try {
                IO.ReadLine(vi, vo);
            }
            catch (const std::exception& exc) { throw_abort(exc.what(), 2); }

            try {
                graph.AddEdge(vi - 1, vo - 1);
            }
            catch (const std::exception& exc) { throw_abort(exc.what(), 3); }
        }
        IO.IgnorLine();

        // Создаём агент-функцию и регестрируем функции
        TRules agent_func;
        agent_func.RegFunc("min", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] < v[1] ? v[0] : v[1]; });
        agent_func.RegFunc("max", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] > v[1] ? v[0] : v[1]; });
        agent_func.RegFunc("+", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] + v[1]; });
        agent_func.RegFunc("-", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] - v[1]; });
        agent_func.RegFunc("*", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] * v[1]; });
        agent_func.RegFunc("/", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] / v[1]; });

        // Читаем правила для агент-функции
        try {
            for (size_t i = 0; i < NV; ++i) {
                auto str = IO.ReadLine();
                agent_func.ReadMainRule(getVert, i, str);
            }
            for (size_t i = 0; i < NV; ++i) {
                auto str = IO.ReadLine();
                agent_func.ReadMainRule(getEdge, i, str);
            }
        }
        catch (const std::exception& exc) { throw_abort(exc.what(), 2); }

        // Вычисляем (применяем к графу)
        agent_func.SetOn(graph);

        // Заисываем результат
        if (IO.IsConsole()) std::cout << "\nOutputting results...\n";

        for (size_t i = 0; i < NV; i++) {
            IO << graph.vertex[i].attribute << std::endl;
        }
        for (size_t i = 0; i < NE; i++) {
            IO << graph.edge[i].attribute << std::endl;
        }
    }
    catch (const EAbort& exc) {
        std::string mes;
        if (exc.exit_code() == 2) mes = "Error in line #" + std::to_string(IO.CurInputLine()) + ". ";
        mes += exc.what();

        std::ostream& err_out = IO.IsConsole() ? std::cerr : IO.OUT();
        err_out << std::endl << exc.what() << std::endl;
        return exc.exit_code();
    }

    return 0;
}

[[nodiscard]] int complete_task_by_file(std::string const& fin_name) {
    bool f{ false };
    try {
        TInOut IO(fin_name);
        f = true;
        return complete_task(IO);
    }
    catch (const std::exception& exc) {
        if (f) throw;
        std::cerr << std::endl << exc.what() << std::endl;
        return 1;
    }
}

#ifndef TEST_MODE

/* ******************************************************************************************************** */
/*                                                MAIN                                                      */
/* ******************************************************************************************************** */
constexpr char const* default_fin_name = "test.gar"; // gar - graph and rures
//constexpr char const* default_fin_name = "c:/Users/Халтурин/source/repos/Ritm_test_1_/x64/Debug/test.gar";

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

#else // TEST_MODE

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

using T = float;
using i_t = unsigned short;

constexpr bool bs = std::is_fundamental_v<T>;
constexpr bool bf = std::is_function_v<T>;
constexpr bool br = std::is_reference_v<T>;

static inline int test_main()
{

    float num = 7.7;
    float& num_r = num;
    //std::unique_ptr<float&> up = std::make_unique<float&>(num_r);
    //std::vector<TAttribute<float&>> v;
    {
        //v.emplace_back(2.2);
    }
    //TAttribute<float&>a(num);
    constexpr auto sig = TAttributeSpec<float&>::signal;
    //TAttributeSpec<float&>::Spec<sig>::type st = num;
    //TAttributeSpec<float&>::type at = num;

    //decltype(auto) dfasdf = at;

    num_r = 1000;

    std::unordered_map<int, char> m;
    m[1] = '1';
    m[2] = '2';
    m[10] = '0';

    auto tmp = m.find(2);
    if (tmp == m.end()) throw std::runtime_error("Unknown");
    auto r = tmp->second;


#if 1
    {
        using TGraph = TAnnotatedGraph<float, asAll>;
        using TRules = TRules<TGraph>;

        TRules rules{};

        rules.RegFunc("+", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] + v[1]; });
        rules.RegFunc("*", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] * v[1]; });

        rules.Add_Value(7);
        //rules.Add_Function("+");
        //rules.Add_Function("-");
        //rules.Add_Function("*");
        rules.Add_Link(getVert, 12);
        rules.Add_Link(getVert, 10);
        rules.Add_Link(getEdge, 77);
    }
    {
        using TGraph = TAnnotatedGraph<float, asAll>;
        using TRules = TRules<TGraph>;

        TRules rules{};

        rules.RegFunc("min", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] < v[1] ? v[0] : v[1]; });
        rules.RegFunc("+", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] + v[1]; });
        rules.RegFunc("*", 2, [](TRules::TRuleFuncArgs const& v) {return v[0] * v[1]; });

        std::istringstream str;
        
        str.clear();
        str.str("min 1 0.2");
        rules.ReadMainRule(getVert, 1, str);

        str.clear();
        str.str("+ 1 0.2");
        rules.ReadMainRule(getVert, 1, str);
    }
#endif // 0

#if 1
    {
        TAnnotatedGraph<T, asVert, i_t> G;
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
        //G.vertex[1].attribute = foo;
        G.vertex[1].attribute = 43254;
        std::cout << "e[0] " << G.edge[0].from << "->" << G.edge[0].to << std::endl;
        std::cout << "v[2] (" << G.vertex[2].first_output << "...; " << G.vertex[2].first_input << "...)" << std::endl;
        //std::cout << "v[1] = " << G.vertex[1].attribute() << std::endl;
        //std::cout << "v[1](2, 3) = " << G.vertex[1].attribute(2, 3) << std::endl;
        std::cout << "v[1] = " << G.vertex[1].attribute << std::endl;
        G.AddVertexes(4000);
    }
    std::cout << std::endl;

    MyClass::print();
    std::cout << std::endl;
#endif // 0

#if 0
    try {
        std::ignore = complete_task_by_file("");
    }
    catch (const std::exception& exc) {
        std::cerr << exc.what() << std::endl;
    }
    std::cout << std::endl;
#endif // 0

    std::cin.get();
    return 0;
}

int main() {
    setlocale(LC_ALL, "");
    return test_main();
}

#endif // TEST_MODE
