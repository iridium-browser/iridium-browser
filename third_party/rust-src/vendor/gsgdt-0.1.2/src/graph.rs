use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::io::{self, Write};

use crate::node::*;

#[derive(Clone, Serialize, Deserialize)]
pub enum GraphKind {
    Digraph,
    Subgraph,
}

pub type AdjList<'a> = HashMap<&'a str, Vec<&'a str>>;

/// Graph represents a directed graph as a list of nodes and list of edges.
#[derive(Serialize, Deserialize)]
pub struct Graph {
    /// Identifier for the graph
    pub name: String,

    /// The Vector containing the Nodes
    pub nodes: Vec<Node>,

    /// The Vector containing the Edges
    pub edges: Vec<Edge>,
}

#[derive(Clone)]
pub struct GraphvizSettings {
    /// The attributes of the graph in graphviz.
    pub graph_attrs: Option<String>,

    /// The attributes of the nodes in graphviz.
    pub node_attrs: Option<String>,

    /// The attributes of the edges in graphviz.
    pub edge_attrs: Option<String>,

    /// Label of the graph
    pub graph_label: Option<String>,
}

impl Default for GraphvizSettings {
    fn default() -> GraphvizSettings {
        GraphvizSettings {
            graph_attrs: None,
            node_attrs: None,
            edge_attrs: None,
            graph_label: None,
        }
    }
}

impl Graph {
    pub fn new(name: String, nodes: Vec<Node>, edges: Vec<Edge>) -> Graph {
        Graph { name, nodes, edges }
    }

    /// Returns the adjacency list representation of the graph.
    /// Adjacency list can be used to easily find the childern of a given node.
    /// If the a node does not have any childern, then the list correspoding to that node
    /// will be empty.
    pub fn adj_list(&self) -> AdjList<'_> {
        let mut m: AdjList<'_> = HashMap::new();
        for node in &self.nodes {
            m.insert(&node.label, Vec::new());
        }
        for edge in &self.edges {
            m.entry(&edge.from).or_insert_with(Vec::new).push(&edge.to);
        }
        m
    }

    /// Returns the reverse adjacency list representation of the graph.
    /// Reverse adjacency list represents the adjacency list of a directed graph where
    /// the edges have been reversed.
    /// Reverse adjacency list can be used to easily find the parents of a given node.
    /// If the a node does not have any childern, then the list correspoding to that node
    /// will be empty.
    pub fn rev_adj_list(&self) -> AdjList<'_> {
        let mut m: AdjList<'_> = HashMap::new();
        for node in &self.nodes {
            m.insert(&node.label, Vec::new());
        }
        for edge in &self.edges {
            m.entry(&edge.to).or_insert_with(Vec::new).push(&edge.from);
        }
        m
    }

    /// Returns the node with the given label, if found.
    pub fn get_node_by_label(&self, label: &str) -> Option<&Node> {
        self.nodes.iter().find(|node| node.label == *label)
    }

    /// Returns the dot representation of the given graph.
    /// This can rendered using the graphviz program.
    pub fn to_dot<W: Write>(
        &self,
        w: &mut W,
        settings: &GraphvizSettings,
        as_subgraph: bool,
    ) -> io::Result<()> {
        if as_subgraph {
            write!(w, "subgraph cluster_{}", self.name)?;
        } else {
            write!(w, "digraph {}", self.name)?;
        }

        writeln!(w, " {{")?;

        if let Some(graph_attrs) = &settings.graph_attrs {
            writeln!(w, r#"    graph [{}];"#, graph_attrs)?;
        }
        if let Some(node_attrs) = &settings.node_attrs {
            writeln!(w, r#"    node [{}];"#, node_attrs)?;
        }
        if let Some(edge_attrs) = &settings.edge_attrs {
            writeln!(w, r#"    edge [{}];"#, edge_attrs)?;
        }
        if let Some(label) = &settings.graph_label {
            writeln!(w, r#"    label=<{}>;"#, label)?;
        }

        for node in self.nodes.iter() {
            write!(w, r#"    {} [shape="none", label=<"#, node.label)?;
            node.to_dot(w)?;
            writeln!(w, ">];")?;
        }

        for edge in self.edges.iter() {
            edge.to_dot(w)?;
        }

        writeln!(w, "}}")
    }
}

#[cfg(test)]
mod tests {
    use crate::*;
    fn get_test_graph() -> Graph {
        let stmts: Vec<String> = vec!["hi".into(), "hell".into()];
        let label1: String = "bb0__0_3".into();
        let style: NodeStyle = Default::default();
        let node1 = Node::new(stmts, label1.clone(), "0".into(), style.clone());

        let stmts: Vec<String> = vec!["_1 = const 1_i32".into(), "_2 = const 2_i32".into()];
        let label2: String = "bb0__1_3".into();
        let node2 = Node::new(stmts, label2.clone(), "1".into(), style.clone());

        Graph::new(
            "Mir_0_3".into(),
            vec![node1, node2],
            vec![Edge::new(label1, label2, "return".into())],
        )
    }

    #[test]
    fn test_adj_list() {
        let g = get_test_graph();
        let adj_list = g.adj_list();
        let expected: AdjList = [("bb0__0_3", vec!["bb0__1_3"]), (&"bb0__1_3", vec![])]
            .iter()
            .cloned()
            .collect();
        assert_eq!(adj_list, expected);
    }

    #[test]
    fn test_json_ser() {
        let g = get_test_graph();
        let json = serde_json::to_string(&g).unwrap();
        let expected_json: String = "\
        {\
            \"name\":\"Mir_0_3\",\
            \"nodes\":[\
            {\
                \"stmts\":[\
                    \"hi\",\
                    \"hell\"\
                ],\
                \"label\":\"bb0__0_3\",\
                \"title\":\"0\",\
                \"style\":{\
                    \"title_bg\":null,\
                    \"last_stmt_sep\":false\
                }\
            },\
            {\
                \"stmts\":[\
                    \"_1 = const 1_i32\",\
                    \"_2 = const 2_i32\"\
                ],\
                \"label\":\"bb0__1_3\",\
                \"title\":\"1\",\
                \"style\":{\
                    \"title_bg\":null,\
                    \"last_stmt_sep\":false\
                }\
            }\
            ],\
            \"edges\":[\
            {\
                \"from\":\"bb0__0_3\",\
                \"to\":\"bb0__1_3\",\
                \"label\":\"return\"\
            }\
            ]\
        }"
        .into();
        assert_eq!(json, expected_json)
    }

    #[test]
    fn test_json_deser() {
        let expected = get_test_graph();
        let struct_json: String = "\
        {\
            \"name\":\"Mir_0_3\",\
            \"nodes\":[\
            {\
                \"stmts\":[\
                    \"hi\",\
                    \"hell\"\
                ],\
                \"label\":\"bb0__0_3\",\
                \"title\":\"0\",\
                \"style\":{\
                    \"title_bg\":null,\
                    \"last_stmt_sep\":false\
                }\
            },\
            {\
                \"stmts\":[\
                    \"_1 = const 1_i32\",\
                    \"_2 = const 2_i32\"\
                ],\
                \"label\":\"bb0__1_3\",\
                \"title\":\"1\",\
                \"style\":{\
                    \"title_bg\":null,\
                    \"last_stmt_sep\":false\
                }\
            }\
            ],\
            \"edges\":[\
            {\
                \"from\":\"bb0__0_3\",\
                \"to\":\"bb0__1_3\",\
                \"label\":\"return\"\
            }\
            ]\
        }"
        .into();
        let got: Graph = serde_json::from_str(&struct_json).unwrap();

        assert_eq!(expected.nodes.len(), got.nodes.len());
        assert_eq!(expected.edges.len(), got.edges.len());

        for (n1, n2) in expected.nodes.iter().zip(got.nodes.iter()) {
            assert_eq!(n1.stmts, n2.stmts);
            assert_eq!(n1.label, n2.label);
            assert_eq!(n1.title, n2.title);
        }

        for (e1, e2) in expected.edges.iter().zip(got.edges.iter()) {
            assert_eq!(e1.from, e2.from);
            assert_eq!(e1.to, e2.to);
            assert_eq!(e1.label, e2.label);
        }
    }
}
