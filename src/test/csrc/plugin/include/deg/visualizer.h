/* Author: baichen.bai@alibaba-inc.com */


#ifndef __VISUALIZER_H__
#define __VISUALIZER_H__


#include "o3.h"
#include "utils.h"
#include "graph_utils.h"
#include <boost/graph/graphviz.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/property_map/dynamic_property_map.hpp>


struct vertex_property {
    struct graphviz_property {
        std::pair<int, int> pos;

        graphviz_property() = default;
        explicit graphviz_property(std::pair<int, int> pos): pos(pos) {}
        ~graphviz_property() = default;
    };

    int type;
    count_t inst_seq;
    std::string inst;
    graphviz_property property;

    vertex_property() = default;
    vertex_property(
        int type,
        count_t inst_seq,
        std::string inst,
        graphviz_property property
    ): type(type), inst_seq(inst_seq), inst(inst), property(property) {}
    ~vertex_property() = default;
};


struct edge_property {
    struct graphviz_property {
        bool dashed;

        graphviz_property() = default;
        explicit graphviz_property(bool dashed): dashed(dashed) {}
        ~graphviz_property() = default;
    };

    latency weight;
    Bottleneck bottleneck;
    bool critical;
    bool _virtual;
    graphviz_property property;

    edge_property() = default;
    explicit edge_property(
        latency weight,
        Bottleneck bottleneck,
        bool critical,
        bool _virtual,
        graphviz_property property
    ):
        weight(weight),
        bottleneck(bottleneck),
        critical(critical),
        _virtual(_virtual),
        property(property) {}
    ~edge_property() = default;
};


class Visualizer: public O3Graph {
public:
    using bg_t = boost::adjacency_list<
        boost::vecS, boost::vecS, boost::directedS, vertex_property, edge_property,
        boost::property<boost::graph_name_t, std::string>
    >;

public:
    Visualizer() = delete;

    Visualizer(typename O3Graph::deg_t deg, count_t start, count_t end) {
        if (start == 0 && end == 0) {
            graph = deg2bg(deg, 0, ULONG_MAX);
        } else {
            graph = deg2bg(deg, start, end);
        }
    }

    ~Visualizer() = default;

public:
    bg_t deg2bg(typename O3Graph::deg_t, count_t start, count_t end);
    void dot(const std::string&);

private:
    bg_t graph;
};


std::string graphviz_encode(std::string s) noexcept {
    // boost::algorithm::replace_all(s, ",", "$$$COMMA$$$");
    // boost::algorithm::replace_all(s, " ", "$$$SPACE$$$");
    // boost::algorithm::replace_all(s, "\"", "$$$QUOTE$$$");
    return s;
}


template <typename graph>
class VertexWriter {
public:
    VertexWriter() = delete;
    explicit VertexWriter(graph _g): _g(_g) {}
    ~VertexWriter() = default;

public:
    template <class vertex_descriptor>
    void operator()(std::ostream& out, const vertex_descriptor& vd) const noexcept {
        out << "[label=\"";
        if (name_of_vertex_type.at(_g[vd].type) == "F1") {
            out << _g[vd].inst << "\\n";
        }
        out << graphviz_encode(
            std::to_string(_g[vd].inst_seq) + ' ' + (name_of_vertex_type.at(_g[vd].type))
        );
        out << "\",pos=\"" << _g[vd].property.pos.first << "," << _g[vd].property.pos.second << "!\"]";
    }

private:
    graph _g;
};


template <typename graph>
class EdgeWriter {
public:
    EdgeWriter() = delete;
    explicit EdgeWriter(graph _g): _g(_g) {}
    ~EdgeWriter() = default;

public:
    template <class edge_descriptor>
    void operator()(std::ostream& out, const edge_descriptor& ed) const noexcept {
        std::stringstream label_stream;
        label_stream << _g[ed].weight << ", ";
        auto label = label_stream.str();
        out << "[label=\"" << _g[ed].bottleneck.__repr__() << " (" << \
            graphviz_encode(label.substr(0, label.size() - 2)) << ")\"";
        if (_g[ed].critical) {
            out << ", color=\"red\"" << "]";
        } else if (_g[ed]._virtual) {
            out << ", style=\"dashed\"" << ",color=\"blue\"" << "]";
        } else {
            out << "]";
        }
        // if (_g[ed].property.dashed) {
        //  out << ",style=\"dashed\"" << ",splines=\"curved\",color=\"red\"" << "]";
        // } else {
        //  out << "]";
        // }
    }

private:
    graph _g;
};


typename Visualizer::bg_t create_empty_directed_graph() noexcept {
    return {};
}


template <typename graph>
std::string get_graph_name(const graph& g) noexcept {
    return get_property(g, boost::graph_name);
}


template <typename graph>
void set_graph_name(const std::string& name, graph& g) noexcept {
    PPK_ASSERT(!std::is_const<graph>::value, "graph cannot be const");
    get_property(g, boost::graph_name) = name;
}


template <typename graph>
std::vector<vertex_property> get_all_vertices(const graph& g) noexcept {
    std::vector<vertex_property> v(boost::num_vertices(g));
    const auto vp = vertices(g);
    std::transform(vp.first, vp.second, std::begin(v), [&g](auto& _v) {
            return g[_v];
        }
    );
    return v;
}


template <typename graph, typename property>
typename boost::graph_traits<graph>::vertex_iterator
get_vertex(property&v, const graph& g) noexcept {
    const auto vp = vertices(g);
    return std::find_if(vp.first, vp.second, [&v, &g](auto _v) {
            return v.type == g[_v].type && v.inst_seq == g[_v].inst_seq;
        }
    );
}


template <typename graph, typename property>
typename boost::graph_traits<graph>::vertex_descriptor
add_vertex_with_property(const property& v, graph& g) noexcept {
    PPK_ASSERT(!std::is_const<graph>::value, "graph cannot be const.");
    auto _v = get_vertex(v, g);
    if (_v == vertices(g).second)
        return boost::add_vertex(v, g);
    else
        return *_v;
}


template <typename graph>
bool if_edge_exists_between_vertices(
    const typename boost::graph_traits<graph>::vertex_descriptor& v1,
    const typename boost::graph_traits<graph>::vertex_descriptor& v2,
    const graph& g
) noexcept {
    return edge(v1, v2, g).second;
}


template <typename graph, typename property>
typename boost::graph_traits<graph>::edge_descriptor add_edge_with_property(
    const typename boost::graph_traits<graph>::vertex_descriptor& from,
    const typename boost::graph_traits<graph>::vertex_descriptor& to,
    const property& edge,
    graph& g,
    bool allow_multiple_edge = true
) {
    PPK_ASSERT(!std::is_const<graph>::value, "graph cannot be const.");
    if (!allow_multiple_edge && if_edge_exists_between_vertices(from, to, g)) {
        std::stringstream msg;
        msg << "[ERROR]: " << __func__ << ": already has an edge";
        throw std::invalid_argument(msg.str());
    }
    const auto e = boost::add_edge(from, to, g);
    PPK_ASSERT(e.second);
    g[e.first] = edge;
    return e.first;
}


template <typename graph>
inline VertexWriter<graph> make_vertex_writer(const graph& g) {
    return VertexWriter<graph>(g);
}


template <typename graph>
inline EdgeWriter<graph> make_edge_writer(const graph& g) {
    return EdgeWriter<graph>(g);
}


typename Visualizer::bg_t Visualizer::deg2bg(typename O3Graph::deg_t deg, count_t start, count_t end) {
    auto bg = create_empty_directed_graph();
    std::for_each(deg.begin(), deg.end(), [&bg, &start, &end](auto node) {
            auto parent = node.first;
            std::for_each(node.second.begin(), node.second.end(), [&parent, &bg, &start, &end](auto edge) {
                    if (
                        (parent.inst_seq >= start && parent.inst_seq <= end) || \
                        (edge.child.inst_seq >= start && edge.child.inst_seq <= end)
                    ) {
                        auto v1 = add_vertex_with_property(
                            vertex_property(
                                parent.type,
                                parent.inst_seq,
                                parent.inst->inst,
                                vertex_property::graphviz_property(
                                    std::pair<int, int>(parent.timing, parent.inst_seq)
                                )
                            ),
                            bg
                        );
                        auto v2 = add_vertex_with_property(
                            vertex_property(
                                edge.child.type,
                                edge.child.inst_seq,
                                edge.child.inst->inst,
                                vertex_property::graphviz_property(
                                    std::pair<int, int>(edge.child.timing, edge.child.inst_seq)
                                )
                            ),
                            bg
                        );
                        // auto dashed = false;
                        // if (parent.type == edge.child.type && \
                        //  edge.child.inst_seq - parent.inst_seq > 1) {
                        //  // vertices are on the same x-axis
                        //  dashed = true;
                        // }
                        add_edge_with_property(
                            v1,
                            v2,
                            edge_property(
                                edge.weight,
                                edge.bottleneck,
                                edge.critical,
                                edge._virtual,
                                edge_property::graphviz_property(false)
                            ),
                            bg
                        );
                    }
                }
            );
        }
    );
    set_graph_name("New DEG", bg);
    return bg;
}


void Visualizer::dot(const std::string& dot_path) {
    std::ofstream f(dot_path);
    boost::write_graphviz(
        f, graph,
        make_vertex_writer(graph), make_edge_writer(graph),
        [&](std::ostream& os) {
            os << "name=\"" << get_graph_name(this->graph) << "\";\n";
            os << "graph [bgcolor=\"#FFFFFF\" " \
                "color=\"#000000\" " \
                "fontcolor=\"#000000\" " \
                "fontname=Times " \
                "fontsize=10 " \
                "margin=\"0,0\" " \
                "pad=\"1.0,0.5\" " \
                "rankdir=LR]\n";
            os << "node [color=\"#000000\" " \
                "fillcolor=\"#E8E8E8\" " \
                "fontcolor=\"#000000\" " \
                "fontname=Times " \
                "fontsize=10 " \
                "margin=\"0,0\" " \
                "shape=ellipse " \
                "style=filled]\n";
            os << "edge [color=\"#000000\" " \
                "fontcolor=\"#000000\" " \
                "fontname=Times " \
                "fontsize=10 " \
                "style=solid]\n";
        }
    );
}

#endif