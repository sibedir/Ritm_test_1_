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

template <
    typename target_graph_type,
    std::unsigned_integral rules_idx_t_ = typename target_graph_type::idx_type,
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

    template<Graph_Elem_Type type> struct TRuleLinkSpec {                                               };
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
            : type{ rtValue }
            , value{ val }
        {};

        template <typename graph_type>
        explicit TRule(Graph_Elem_Type type, link_idx_t idx)
            : type{ TRuleLinkSpec<type>::type }
            , idx{ idx }
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
    void print_t() {
        std::cout << typeid(rules.vert_[0].attribute()).name() << std::endl;
    }

    TRules() : functions_specification(), rules() {};

    void RegFunc(std::string name, func_arg_idx_t arg_count, TRuleFunc const& func) {
        if (name == "v" or name == "e") throw std::runtime_error("Invalid function name: \"" + name + "\"");
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
        r.attribute().idx = idx;
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
