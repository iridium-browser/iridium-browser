use crate::*;
use std::collections::{HashMap, HashSet, VecDeque};

/// A wrapper around [Graph](struct.Graph.html) to assist diffing.
pub struct DiffGraph<'a> {
    pub(crate) graph: &'a Graph,
    pub(crate) dist_start: HashMap<&'a str, usize>,
    pub(crate) dist_end: HashMap<&'a str, usize>,
}

impl<'a> DiffGraph<'a> {
    pub fn new(graph: &'a Graph) -> Self {
        let adj_list = graph.adj_list();
        let rev_adj_list = graph.rev_adj_list();
        let start_nodes = Self::get_source_labels(&adj_list);
        let end_nodes = Self::get_source_labels(&rev_adj_list);
        DiffGraph {
            graph,
            dist_start: Self::bfs_shortest_dist(rev_adj_list, start_nodes),
            dist_end: Self::bfs_shortest_dist(adj_list, end_nodes),
        }
    }

    /// Calculate the shortest distance to the end from the given sources nodes using bfs.
    fn bfs_shortest_dist(adj_list: AdjList<'a>, source: Vec<&'a str>) -> HashMap<&'a str, usize> {
        let mut dist = HashMap::new();
        for k in source.iter() {
            dist.insert(*k, 0);
        }
        let mut visited = HashSet::new();
        let mut queue: VecDeque<&str> = source.into();
        while let Some(node) = queue.pop_front() {
            let neighbours = adj_list.get(node).unwrap();
            let curr_dist = *dist.get(&node).unwrap();

            for neighbour in neighbours {
                if !visited.contains(neighbour) {
                    dist.insert(neighbour, curr_dist + 1);
                    queue.push_back(neighbour);
                    visited.insert(neighbour);
                }
            }
        }

        dist
    }

    /// Get the source labels for a given adjacency list. The source labels will the
    // TODO: This is sink labels, not source labels
    fn get_source_labels(adj_list: &AdjList<'a>) -> Vec<&'a str> {
        adj_list
            .iter()
            .filter(|(_, v)| v.is_empty())
            .map(|(k, _)| *k)
            .collect()
    }
}
