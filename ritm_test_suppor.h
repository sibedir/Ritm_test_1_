/* ******************************************************************************************************** */
/*                                   ВСПОМОГАТЕЛЬНЫЕ ТИПЫ И МЕТОДЫ                                          */
/* ******************************************************************************************************** */
#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

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

// Класс яля "поток ввода-вывода"
class TInOut {
private:
    bool is_console{ true };
    std::ifstream* fin{ nullptr };
    std::ofstream* fout{ nullptr };
    size_t input_line{ 0 }; // номер текущей строки в потоке ввода
public:
    TInOut(std::string const& input_file) {
        is_console = (input_file == "");
        if (!is_console) {
            try {
                fin = new std::ifstream;
                fin->open(input_file);
                if (fin->fail()) throw std::runtime_error("Ошибка при открытия файла: \"" + input_file + "\"");

                fout = new std::ofstream;
                fout->open(input_file + ".out");
                if (fout->fail()) throw std::runtime_error("Ошибка при открытия файла: \"" + input_file + ".out\"");
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

    std::istream& IN() { return is_console ? std::cin : *fin; };
    std::ostream& OUT() { return is_console ? std::cout : *fout; };
    bool IsConsole() const { return is_console; };
    size_t CurInputLine() const { return input_line; };

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
        auto line = ReadLine();
        (ReadValue(line, args), ...);
    }
};
