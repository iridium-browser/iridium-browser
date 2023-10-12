use crate::diff::{match_graphs, DiffGraph, Match};
use crate::{MultiGraph, Edge, Graph, NodeStyle};
use std::collections::HashSet;

/// Returns a MultiGraph containing the diff of the two graphs.
/// To be visualized with dot.
pub fn visualize_diff(d1: &DiffGraph, d2: &DiffGraph) -> MultiGraph {
    let matches = match_graphs(d1, d2);

    let mut matched1 = HashSet::new();
    let mut matched2 = HashSet::new();
    let mut partial1 = HashSet::new();
    let mut partial2 = HashSet::new();

    for mch in matches {
        match mch {
            Match::Full(m) => {
                matched1.insert(m.from);
                matched2.insert(m.to);
            }
            Match::Partial(m) => {
                partial1.insert(m.from);
                partial2.insert(m.to);
            }
        }
    }

    let added_style = NodeStyle {
        title_bg: Some("green".into()),
        ..Default::default()
    };
    let removed_style = NodeStyle {
        title_bg: Some("red".into()),
        ..Default::default()
    };
    let changed_style = NodeStyle {
        title_bg: Some("yellow".into()),
        ..Default::default()
    };
    let default_style = NodeStyle {
        ..Default::default()
    };

    let edges1: Vec<Edge> = d1
        .graph
        .edges
        .iter()
        .map(|e| {
            Edge::new(
                format!("{}_diff1", e.from),
                format!("{}_diff1", e.to),
                e.label.clone(),
            )
        })
        .collect();
    let edges2: Vec<Edge> = d2
        .graph
        .edges
        .iter()
        .map(|e| {
            Edge::new(
                format!("{}_diff2", e.from),
                format!("{}_diff2", e.to),
                e.label.clone(),
            )
        })
        .collect();

    let mut nodes1 = Vec::new();
    for node in &d1.graph.nodes {
        let label = node.label.as_str();
        let mut node_cloned = node.clone();
        node_cloned.label = format!("{}_diff1", node.label);
        if matched1.contains(label) {
            node_cloned.style = default_style.clone();
            nodes1.push(node_cloned);
        } else if partial1.contains(label) {
            node_cloned.style = changed_style.clone();
            nodes1.push(node_cloned);
        } else {
            node_cloned.style = removed_style.clone();
            nodes1.push(node_cloned);
        }
    }

    let mut nodes2 = Vec::new();
    for node in &d2.graph.nodes {
        let label = node.label.as_str();
        let mut node_cloned = node.clone();
        node_cloned.label = format!("{}_diff2", node.label);
        if matched2.contains(label) {
            node_cloned.style = default_style.clone();
            nodes2.push(node_cloned);
        } else if partial2.contains(label) {
            node_cloned.style = changed_style.clone();
            nodes2.push(node_cloned);
        } else {
            node_cloned.style = added_style.clone();
            nodes2.push(node_cloned);
        }
    }
    let newg1 = Graph::new("diff1".to_owned(), nodes1, edges1);
    let newg2 = Graph::new("diff2".to_owned(), nodes2, edges2);

    MultiGraph::new("diff".to_owned(), vec![newg1, newg2])
}
