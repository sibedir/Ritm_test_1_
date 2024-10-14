/* ******************************************************************************************************** */
/*                                     АГЕНТ-ФУНКЦИЯ НАД ГРАФОМ                                             */
/* ******************************************************************************************************** */
#pragma once

#include <concepts>
#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <functional>

#include "my_graph.h"
#include "ritm_test_suppor.h"

template <
    typename TTargetGraph_,
    std::unsigned_integral rules_idx_t_ = typename TTargetGraph_::idx_type,
    std::unsigned_integral func_arg_idx_t_ = unsigned short
>
class TRules {
#ifdef TEST_MODE
    friend int test_main();
#endif // TEST_MODE
public:
    using TTargetGraph = TTargetGraph_; // тип целевого графа
    using value_type = TTargetGraph::attr_value_type; // тип атрибутов (результатов работы функций)
    using link_idx_t = TTargetGraph::idx_type; // тип для номеров элементов графа
    using rules_idx_t = rules_idx_t_; // тип для номеров правил агент функции
    using func_arg_idx_t = func_arg_idx_t_; // тип для номеров аргументов функций (например: "min a b" - 2 аргумента)

    using TRuleFuncArgs = std::vector<value_type>;
    using TRuleFunc = std::function<value_type(TRuleFuncArgs const&)>;
    using TAttribute = TAttribute<value_type>;

private:

    struct TRuleFuncSpec {
        TRuleFunc func;
        func_arg_idx_t arg_count{ 0 };
    };

    using TRuleFuncSpecMap = std::unordered_map<std::string, TRuleFuncSpec>;

    enum TRuleType { rtNone = 0, rtValue, rtVertLink, rtEdgeLink, rtFunc };
    //                          значение   ссылка на  ссылка на   функция
    //                                       узел       ребро

    // Класс "правило агент-функции"
    struct TRule {
        TRuleType rule_type{ rtNone }; // вид правила
        union {
            value_type value;
            link_idx_t idx;
            TRuleFuncSpec const* func{ nullptr };
        };

        TRule() = default;

        // конструктор для значения
        TRule(value_type val)
            : rule_type{ rtValue }
            , value{ val }
        {};

        // конструктор для ссылки
        TRule(Graph_Elem_Type type, link_idx_t idx)
            : rule_type{ type == getVert ? rtVertLink : rtEdgeLink }
            , idx{ idx }
        {};

        // конструктор для функции
        TRule(TRuleFuncSpec const* func_spec)
            : rule_type{ rtFunc }
            , func{ func_spec }
        {};

    };

private:
    using TLinker = std::unordered_map<link_idx_t, rules_idx_t>;
    using TRuleGraph = TAnnotatedGraph<TRule, asVert, rules_idx_t>;
    using TRuleIterator = typename TRuleGraph::TIterator;

    TLinker vert_linker{}; // линкер узлов (хранит номера правил, ссылающихся на узлы)
    TLinker edge_linker{}; // линкер рёбер (хранит номера правил, ссылающихся на рёбра)
    TRuleFuncSpecMap functions_specification{}; // спецификация функций (хранит указатели функций и количества аргументов)
    TRuleGraph rules{}; // ГРАФ АГЕНТ-ФУНКЦИИ
    bool ready = false; // отсортирован ли граф

private:
    // добавляет правило/узел типа "значение"
    rules_idx_t Add_Value(value_type val) {
        rules.AddVertex(val);
        return rules.vertex.size() - 1;
    }

    // добавляет узел типа "функция" (ссылка на спецификацию функции)
    // ! НЕ создаёт зависимости/рёбра
    rules_idx_t Add_Function(TRuleFuncSpec const* func_spec) {
        rules.AddVertex(func_spec);
        return rules.vertex.size() - 1;
    }

    /*
    // добавляет узел типа "функция" (ссылка на спецификацию функции) по имени
    // ! НЕ создаёт зависимости/рёбра
    rules_idx_t Add_Function(std::string const& func_name) {
        rules.AddVertex(functions_specification, func_name);
        return rules.vertex.size() - 1;
    }
    */

    // добавляет правило/узел типа "ссылка"
    rules_idx_t Add_Link(Graph_Elem_Type elem_type, link_idx_t idx) {
        TLinker& linker = (elem_type == getVert) ? vert_linker : edge_linker;

        // ищем индекс елемента в линкере
        auto it = linker.find(idx);

        // если найден, то возвращаем его
        if (it != linker.end()) return it->second;

        // иначе, создаём новое правило "ссылка"
        rules.AddVertex(elem_type, idx);
        // и сохраняем в линкере номер правила, которое соответствует индексу элемента
        rules_idx_t ri = rules.vertex.size() - 1;
        linker.insert({ idx, ri });
        return ri;
    }

    // чтение правила из строкового потока
    rules_idx_t ReadRule(std::istringstream& IN) {
        std::string str;
        ReadValue(IN, str); // читаем первое слово

        // Если это ссылка на елемент графа (зарезервированные слова "v" и "e")
        if (str.size() == 1 and (str[0] == 'v' or str[0] == 'e')) {
            Graph_Elem_Type et = str[0] == 'v' ? getVert : getEdge;
            link_idx_t li;
            ReadValue(IN, li);
            return Add_Link(et, li - 1);
        }
        // Если это функция
        TRuleFuncSpec const* fs = FunctionsSpec(str);
        if (fs) {
            // Считываем все аргументы во временный вектор
            bool args_all_value = true;
            std::vector<rules_idx_t> arg_idxs(fs->arg_count);
            for (func_arg_idx_t i = 0; i < fs->arg_count; ++i) {
                arg_idxs[i] = ReadRule(IN);
                args_all_value &= rules.vertex[arg_idxs[i]].attribute().rule_type == rtValue;
            }

            // Если все аргументы - это значения, то расчет функции выполняется на месте
            if (args_all_value) {
                std::vector<value_type> args(fs->arg_count);
                for (func_arg_idx_t i = 0; i < fs->arg_count; ++i) {
                    args[i] = rules.vertex[arg_idxs[i]].attribute().value;
                }
                return Add_Value(fs->func(args));
            }

            // иначе, в графф агент-функции записываются соответствующие правила 
            rules_idx_t ri = Add_Function(fs);
            for (func_arg_idx_t i = fs->arg_count; i > 0 ; --i) {
                // аргументы добавляются в обратном порядке,
                // так как новые рёбра добавляются в начало списка узла
                rules.AddEdge(ri, arg_idxs[i-1]);
            }
            return ri;
        }

        // То это значение
        value_type val;
        std::stringstream(str) >> val;
        return Add_Value(val);
    }

public:

    // регистрация новой функции
    void RegFunc(std::string name, func_arg_idx_t arg_count, TRuleFunc const& func) {
        // лексемы "v" и "e" зарезервированы под ссылки
        if (name == "v" or name == "e") throw std::runtime_error("Invalid function name: \"" + name + "\"");

        TRuleFuncSpec& rfs = functions_specification[name];
        rfs.func = func;
        rfs.arg_count = arg_count;
    }

    // получить ссылку на спецификацию функции по имени
    TRuleFuncSpec const* FunctionsSpec(std::string name) const {
        auto it = functions_specification.find(name);
        return (it == functions_specification.end()) ? nullptr : &(it->second);
    }

    // чтение строки с правилом агент-функции
    rules_idx_t ReadMainRule(Graph_Elem_Type elem_type, link_idx_t idx, std::istringstream& IN) {
        ready = false;

        rules_idx_t li = Add_Link(elem_type, idx);
        rules_idx_t ri = ReadRule(IN);
        rules.AddEdge(li, ri);
        return li;
    }

    // подготовка агент-функции к применению на графы (топологическая сортировка)
    void GetReady() {
        rules.TopSort();
    }

    // применению агент-функции на граф
    void SetOn(TTargetGraph& graph) {
        if (!ready) {
            rules.TopSort();
            ready = true;
        }

        std::vector<value_type> res(rules.vertex.size());

        // для каждого правила последовательно выполняется
        for (rules_idx_t ri = 0; ri < rules.vertex.size(); ++ri) {
            TRuleIterator iter{ rules, ri };
            TRule& r = rules.vertex[ri].attribute();

            // если правило - это значение
            if (r.rule_type == rtValue) {
                res[ri] = r.value; // сохраняем значение
                continue;
            }

            // если правило - это ссылка на узел
            if (r.rule_type == rtVertLink) {
                if (iter.end_e()) { // если ссылка ни от чего не зависит
                    res[ri] = graph.vertex[r.idx].attribute; // читаем атрибут из целевого графа
                }
                else { // иначе
                    res[ri] = res[iter.look_e()]; // сохраняем значение, от которого зависит ссылка
                    graph.vertex[r.idx].attribute = res[ri]; // и записываем его в атрибут целевого графа
                }
                continue;
            }

            // аналогично для правила ссылка на ребро 
            if (r.rule_type == rtEdgeLink) {
                if (iter.end_e()) {
                    res[ri] = graph.edge[r.idx].attribute;
                }
                else {
                    res[ri] = res[iter.look_e()];
                    graph.edge[r.idx].attribute = res[ri];
                }
                continue;
            }

            // если правило - это функция
            //if (r.rule_type == rtFunc) {
                std::vector<value_type> args(r.func->arg_count);
                for (func_arg_idx_t i = 0; i < args.size(); ++i) { // последовательно читаем значения аргументов из уже готовых результатов
                    args[i] = res[iter.look_e()]; // читаем аргумент
                    iter.next_e(); // следующее ребро
                }
                res[ri] = r.func->func(args); // выполняем функцию и записываем результат
            //    continue;
            //}
        }
    }
};
