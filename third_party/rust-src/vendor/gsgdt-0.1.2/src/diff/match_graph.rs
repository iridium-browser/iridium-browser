use crate::diff::*;
use crate::levenshtein::distance;
use std::collections::{BTreeMap, HashSet};

pub type Mapping<'a> = BTreeMap<&'a str, &'a str>;

// TODO: Is it better to return a list of matches or a hashmap
// It might be better to do former because we may need to distinguise
// b/w full and partial match
#[derive(Debug, PartialEq)]
pub struct Matching<'a> {
    pub from: &'a str,
    pub to: &'a str,
}

impl<'a> Matching<'a> {
    pub fn new(from: &'a str, to: &'a str) -> Matching<'a> {
        Matching { from, to }
    }
}

#[derive(Debug, PartialEq)]
pub enum Match<'a> {
    Full(Matching<'a>),
    Partial(Matching<'a>),
}
//
// impl<'a> Match<'a> {
//     fn is_full(&self) -> bool {
//         match self {
//             Match::Full(_) => true,
//             _ => false
//         }
//     }
// }

/// Matches both graphs and returns the mapping of nodes from g1 to g2
pub fn match_graphs<'a>(d1: &'a DiffGraph<'_>, d2: &'a DiffGraph<'_>) -> Vec<Match<'a>> {
    let mut mapping: BTreeMap<&str, &str> = get_initial_mapping(d1, d2);

    // TODO: This mapping may have duplicate mappings, remove them

    let mut matches: Vec<Match> = mapping
        .iter()
        .map(|(from, to)| Match::Full(Matching { from, to }))
        .collect();

    // we use rev adjacency list because we are going to compare the parents
    let rev_adj_list1 = d1.graph.rev_adj_list();
    let rev_adj_list2 = d2.graph.rev_adj_list();

    let mut matched_labels_in_2: HashSet<&str> = mapping.iter().map(|(_, v)| *v).collect();

    let mut prev_mapping = mapping.clone();
    loop {
        let mut new_mapping: Mapping = BTreeMap::new();
        for (k, v) in prev_mapping.iter() {
            let parents_in_1 = rev_adj_list1.get(k).unwrap();
            let parents_in_2 = rev_adj_list2.get(v).unwrap();

            for n in parents_in_1.iter() {
                // don't bother selecting if the node in 1 is already matched
                // as we use a stricter selection for the initial matching
                if mapping.contains_key(n) {
                    continue;
                }
                if let Some(lab) = select(d1, d2, n, &parents_in_2, false) {
                    // if the matched label is already matched to some node in 1, skip
                    if !matched_labels_in_2.contains(lab) {
                        matched_labels_in_2.insert(lab);
                        new_mapping.insert(n, lab);
                    }
                }
            }
        }
        // println!("{:?}", new_mapping);
        // merge
        let new_matches = get_new_matches(&mapping, &new_mapping);
        if new_matches.len() == 0 {
            break;
        }
        for mch in new_matches {
            mapping.insert(mch.from, mch.to);
            matches.push(Match::Partial(mch));
        }
        prev_mapping = new_mapping;
    }

    matches
}

fn get_initial_mapping<'a>(d1: &'a DiffGraph<'_>, d2: &'a DiffGraph<'_>) -> Mapping<'a> {
    let mut mapping: BTreeMap<&str, &str> = BTreeMap::new();
    let mut reverse_mapping: BTreeMap<&str, &str> = BTreeMap::new();
    let g2_labels: Vec<&str> = d2.graph.nodes.iter().map(|n| n.label.as_str()).collect();

    // TODO: this can be functional
    for node in d1.graph.nodes.iter() {
        if let Some(matched_lab) = select(d1, d2, &node.label, &g2_labels, true) {
            if let Some(lab_in_1) = reverse_mapping.get(matched_lab) {
                // matched_lab was already matched
                // select the one with the lowest cost
                // this is done so that no duplicate matching will occur
                let dist_already = dist_bw_nodes(d1, d2, lab_in_1, matched_lab);
                let dist_new = dist_bw_nodes(d1, d2, &node.label, matched_lab);
                if dist_new > dist_already {
                    continue;
                }
                mapping.remove(lab_in_1);
            }
            mapping.insert(&node.label, matched_lab);
            reverse_mapping.insert(matched_lab, &node.label);
        }
    }

    mapping
}

fn dist_bw_nodes(d1: &DiffGraph<'_>, d2: &DiffGraph<'_>, n1: &str, n2: &str) -> Option<usize> {
    let node1 = d1.graph.get_node_by_label(n1).unwrap();
    let node2 = d2.graph.get_node_by_label(n2).unwrap();

    let tup1 = (
        d1.dist_start[n1] as i64,
        d1.dist_end[n1] as i64,
        node1.stmts.len() as i64,
    );
    let tup2 = (
        d2.dist_start[n2] as i64,
        d2.dist_end[n2] as i64,
        node2.stmts.len() as i64,
    );

    let s1 = node1.stmts.join("");
    let s2 = node2.stmts.join("");
    let dist = distance(&s1, &s2);

    Some(
        ((tup1.0 - tup2.0).pow(2) + (tup1.1 - tup2.1).pow(2) + (tup1.2 - tup2.2).pow(2)) as usize
            + dist,
    )
}

/// Selects the most suitable match for n from L
fn select<'a>(
    d1: &'a DiffGraph<'_>,
    d2: &'a DiffGraph<'_>,
    n: &'a str,
    list_of_labs: &[&'a str],
    use_text_dist_filter: bool,
) -> Option<&'a str> {
    let node = d1.graph.get_node_by_label(n).unwrap();
    let node_stmt_len = node.stmts.len();
    let node_stmts = node.stmts.join("");
    list_of_labs
        .iter()
        .filter(|lab| {
            let other_node = d2.graph.get_node_by_label(lab).unwrap();
            // filter out nodes that may differ by more than 2 lines
            (other_node.stmts.len() as i64 - node.stmts.len() as i64).abs() <= 2
        })
        .filter(|lab| {
            if !use_text_dist_filter {
                return true;
            }
            let other_node_stmts = d2.graph.get_node_by_label(lab).unwrap().stmts.join("");
            // allow bigger basic blocks have more edits
            // 2 here is arbitary
            distance(&node_stmts, &other_node_stmts) < 2 * node_stmt_len
        })
        .min_by_key(|x| dist_bw_nodes(d1, d2, n, x))
        .map(|x| *x)
}

fn get_new_matches<'a>(mapping: &Mapping<'a>, new_mapping: &Mapping<'a>) -> Vec<Matching<'a>> {
    let mut changed = Vec::new();
    for (k, v) in new_mapping.iter() {
        if !mapping.contains_key(k) {
            changed.push(Matching { from: k, to: v })
        }
    }

    changed
}
