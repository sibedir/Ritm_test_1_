/* ******************************************************************************************************** */
/*                                               √–¿‘                                                       */
/* ******************************************************************************************************** */
#pragma once

#include <memory>
#include <type_traits>
#include <concepts>
#include <string>
#include <vector>

// TAttribute

template <typename value_type>
struct TAttributeSpec {
public:
    using rr_val_ptr = std::remove_reference_t<value_type>*;
private:
public:
    struct TAEmpty {};

    template <typename T>
    struct TAValue {
        T attribute;
        TAValue() = default;
        TAValue(T attribute) : attribute(attribute) {}
    };

    template <typename T>
    struct TAUPtr : public std::unique_ptr<T> {
        using TUniquePtr = std::unique_ptr<T>;
        TAUPtr() : TUniquePtr() {}
        template <typename... _T>
        TAUPtr(T&& args) : attribute(std::forward<_T>(args)...) {}
        T& attribute() {
            return TUniquePtr::operator*();
        };
    };

    enum atr_type { atEmpty = 0, atValue, atFunc, atUPtr };

    static constexpr atr_type signal = std::is_void_v<value_type>
        ? atEmpty
        : (std::is_fundamental_v<value_type> or std::is_pointer_v<value_type>
            ? atValue
            : (std::is_function_v<value_type>
                ? atFunc
                : atUPtr));

    template <atr_type> struct Spec          { using type = TAEmpty             ; };
    template <        > struct Spec<atValue> { using type = TAValue<value_type >; };
    template <        > struct Spec<atFunc > { using type = TAValue<rr_val_ptr >; };
    template <        > struct Spec<atUPtr > { using type = TAUPtr <value_type >; };
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

template <typename value_type, bool is_vert> struct TVertAnnotatedSpec { using arrt_value_type = void; };
template <typename value_type              > struct TVertAnnotatedSpec<value_type, true> { using arrt_value_type = value_type; };

template <typename value_type, bool is_edge> struct TEdgeAnnotatedSpec { using arrt_value_type = void; };
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
        idx_type const first_input;
        idx_type const first_output;
    };

    struct TVertBase {
        idx_type first_input{ BAD_IDX };
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
        idx_type to;
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
        idx_type& vtfi = vert_arr[to].first_input;
        idx_type next_from = vffo;
        idx_type next_to = vtfi;
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
