#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>

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
    std::istream& IN() { return is_console ? std::cin : *fin; };
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

    bool IsConsole() const { return is_console; };

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

#include <iostream>

#include "my_graph.h"
#include "agent_function.h"

[[nodiscard]] int complete_task(TInOut& IO) {

    using TGraph = TAnnotatedGraph<float, asAll>;

    try {
        /* Ввод размеров графа */
        if (IO.IsConsole()) std::cout << "Entering sizes...\n";

        size_t NV, NE;
        IO.ReadLine(NV, NE);
        TGraph graph;
        graph.AddVertexes(NV);
        IO.IgnorLine();

        /* Ввод рёбер */
        if (IO.IsConsole()) std::cout << "Entering edges...\n";

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

#endif

        /* Print */
        if (IO.IsConsole()) std::cout << "\nOutputting results...\n";

        for (size_t i = 0; i < NV; i++) {
            IO << graph.vertex[i].attribute << std::endl;
        }
        for (size_t i = 0; i < NE; i++) {
            IO << graph.edge[i].attribute << std::endl;
        }

        std::cout << "...READY\n";
    }
    catch (const EAbort& exc) {
        std::ostream& err_out = IO.IsConsole() ? std::cerr : IO.OUT();
        err_out << std::endl << exc.what() << std::endl;
        return exc.exit_code();
    }

    return 0;
}

[[nodiscard]] int complete_task_by_file(std::string const& fin_name) {
    TInOut IO(fin_name);
    return complete_task(IO);
}
