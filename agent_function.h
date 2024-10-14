/* ******************************************************************************************************** */
/*                                     �����-������� ��� ������                                             */
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
    using TTargetGraph = TTargetGraph_; // ��� �������� �����
    using value_type = TTargetGraph::attr_value_type; // ��� ��������� (����������� ������ �������)
    using link_idx_t = TTargetGraph::idx_type; // ��� ��� ������� ��������� �����
    using rules_idx_t = rules_idx_t_; // ��� ��� ������� ������ ����� �������
    using func_arg_idx_t = func_arg_idx_t_; // ��� ��� ������� ���������� ������� (��������: "min a b" - 2 ���������)

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
    //                          ��������   ������ ��  ������ ��   �������
    //                                       ����       �����

    // ����� "������� �����-�������"
    struct TRule {
        TRuleType rule_type{ rtNone }; // ��� �������
        union {
            value_type value;
            link_idx_t idx;
            TRuleFuncSpec const* func{ nullptr };
        };

        TRule() = default;

        // ����������� ��� ��������
        TRule(value_type val)
            : rule_type{ rtValue }
            , value{ val }
        {};

        // ����������� ��� ������
        TRule(Graph_Elem_Type type, link_idx_t idx)
            : rule_type{ type == getVert ? rtVertLink : rtEdgeLink }
            , idx{ idx }
        {};

        // ����������� ��� �������
        TRule(TRuleFuncSpec const* func_spec)
            : rule_type{ rtFunc }
            , func{ func_spec }
        {};

    };

private:
    using TLinker = std::unordered_map<link_idx_t, rules_idx_t>;
    using TRuleGraph = TAnnotatedGraph<TRule, asVert, rules_idx_t>;
    using TRuleIterator = typename TRuleGraph::TIterator;

    TLinker vert_linker{}; // ������ ����� (������ ������ ������, ����������� �� ����)
    TLinker edge_linker{}; // ������ ���� (������ ������ ������, ����������� �� ����)
    TRuleFuncSpecMap functions_specification{}; // ������������ ������� (������ ��������� ������� � ���������� ����������)
    TRuleGraph rules{}; // ���� �����-�������
    bool ready = false; // ������������ �� ����

private:
    // ��������� �������/���� ���� "��������"
    rules_idx_t Add_Value(value_type val) {
        rules.AddVertex(val);
        return rules.vertex.size() - 1;
    }

    // ��������� ���� ���� "�������" (������ �� ������������ �������)
    // ! �� ������ �����������/����
    rules_idx_t Add_Function(TRuleFuncSpec const* func_spec) {
        rules.AddVertex(func_spec);
        return rules.vertex.size() - 1;
    }

    /*
    // ��������� ���� ���� "�������" (������ �� ������������ �������) �� �����
    // ! �� ������ �����������/����
    rules_idx_t Add_Function(std::string const& func_name) {
        rules.AddVertex(functions_specification, func_name);
        return rules.vertex.size() - 1;
    }
    */

    // ��������� �������/���� ���� "������"
    rules_idx_t Add_Link(Graph_Elem_Type elem_type, link_idx_t idx) {
        TLinker& linker = (elem_type == getVert) ? vert_linker : edge_linker;

        // ���� ������ �������� � �������
        auto it = linker.find(idx);

        // ���� ������, �� ���������� ���
        if (it != linker.end()) return it->second;

        // �����, ������ ����� ������� "������"
        rules.AddVertex(elem_type, idx);
        // � ��������� � ������� ����� �������, ������� ������������� ������� ��������
        rules_idx_t ri = rules.vertex.size() - 1;
        linker.insert({ idx, ri });
        return ri;
    }

    // ������ ������� �� ���������� ������
    rules_idx_t ReadRule(std::istringstream& IN) {
        std::string str;
        ReadValue(IN, str); // ������ ������ �����

        // ���� ��� ������ �� ������� ����� (����������������� ����� "v" � "e")
        if (str.size() == 1 and (str[0] == 'v' or str[0] == 'e')) {
            Graph_Elem_Type et = str[0] == 'v' ? getVert : getEdge;
            link_idx_t li;
            ReadValue(IN, li);
            return Add_Link(et, li - 1);
        }
        // ���� ��� �������
        TRuleFuncSpec const* fs = FunctionsSpec(str);
        if (fs) {
            // ��������� ��� ��������� �� ��������� ������
            bool args_all_value = true;
            std::vector<rules_idx_t> arg_idxs(fs->arg_count);
            for (func_arg_idx_t i = 0; i < fs->arg_count; ++i) {
                arg_idxs[i] = ReadRule(IN);
                args_all_value &= rules.vertex[arg_idxs[i]].attribute().rule_type == rtValue;
            }

            // ���� ��� ��������� - ��� ��������, �� ������ ������� ����������� �� �����
            if (args_all_value) {
                std::vector<value_type> args(fs->arg_count);
                for (func_arg_idx_t i = 0; i < fs->arg_count; ++i) {
                    args[i] = rules.vertex[arg_idxs[i]].attribute().value;
                }
                return Add_Value(fs->func(args));
            }

            // �����, � ����� �����-������� ������������ ��������������� ������� 
            rules_idx_t ri = Add_Function(fs);
            for (func_arg_idx_t i = fs->arg_count; i > 0 ; --i) {
                // ��������� ����������� � �������� �������,
                // ��� ��� ����� ���� ����������� � ������ ������ ����
                rules.AddEdge(ri, arg_idxs[i-1]);
            }
            return ri;
        }

        // �� ��� ��������
        value_type val;
        std::stringstream(str) >> val;
        return Add_Value(val);
    }

public:

    // ����������� ����� �������
    void RegFunc(std::string name, func_arg_idx_t arg_count, TRuleFunc const& func) {
        // ������� "v" � "e" ��������������� ��� ������
        if (name == "v" or name == "e") throw std::runtime_error("Invalid function name: \"" + name + "\"");

        TRuleFuncSpec& rfs = functions_specification[name];
        rfs.func = func;
        rfs.arg_count = arg_count;
    }

    // �������� ������ �� ������������ ������� �� �����
    TRuleFuncSpec const* FunctionsSpec(std::string name) const {
        auto it = functions_specification.find(name);
        return (it == functions_specification.end()) ? nullptr : &(it->second);
    }

    // ������ ������ � �������� �����-�������
    rules_idx_t ReadMainRule(Graph_Elem_Type elem_type, link_idx_t idx, std::istringstream& IN) {
        ready = false;

        rules_idx_t li = Add_Link(elem_type, idx);
        rules_idx_t ri = ReadRule(IN);
        rules.AddEdge(li, ri);
        return li;
    }

    // ���������� �����-������� � ���������� �� ����� (�������������� ����������)
    void GetReady() {
        rules.TopSort();
    }

    // ���������� �����-������� �� ����
    void SetOn(TTargetGraph& graph) {
        if (!ready) {
            rules.TopSort();
            ready = true;
        }

        std::vector<value_type> res(rules.vertex.size());

        // ��� ������� ������� ��������������� �����������
        for (rules_idx_t ri = 0; ri < rules.vertex.size(); ++ri) {
            TRuleIterator iter{ rules, ri };
            TRule& r = rules.vertex[ri].attribute();

            // ���� ������� - ��� ��������
            if (r.rule_type == rtValue) {
                res[ri] = r.value; // ��������� ��������
                continue;
            }

            // ���� ������� - ��� ������ �� ����
            if (r.rule_type == rtVertLink) {
                if (iter.end_e()) { // ���� ������ �� �� ���� �� �������
                    res[ri] = graph.vertex[r.idx].attribute; // ������ ������� �� �������� �����
                }
                else { // �����
                    res[ri] = res[iter.look_e()]; // ��������� ��������, �� �������� ������� ������
                    graph.vertex[r.idx].attribute = res[ri]; // � ���������� ��� � ������� �������� �����
                }
                continue;
            }

            // ���������� ��� ������� ������ �� ����� 
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

            // ���� ������� - ��� �������
            //if (r.rule_type == rtFunc) {
                std::vector<value_type> args(r.func->arg_count);
                for (func_arg_idx_t i = 0; i < args.size(); ++i) { // ��������������� ������ �������� ���������� �� ��� ������� �����������
                    args[i] = res[iter.look_e()]; // ������ ��������
                    iter.next_e(); // ��������� �����
                }
                res[ri] = r.func->func(args); // ��������� ������� � ���������� ���������
            //    continue;
            //}
        }
    }
};
