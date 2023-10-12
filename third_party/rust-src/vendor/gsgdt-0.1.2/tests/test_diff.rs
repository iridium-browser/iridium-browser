use gsgdt;
mod helpers;
use helpers::*;

use gsgdt::*;

#[test]
fn test_diff_2() {
    let g1 = read_graph_from_file("tests/graph1.json");
    let g2 = read_graph_from_file("tests/graph2.json");

    let d1 = DiffGraph::new(&g1);
    let d2 = DiffGraph::new(&g2);
    let mapping = match_graphs(&d1, &d2);
    let expected = vec![
        Match::Full(Matching::new("bb0", "bb0")),
        Match::Full(Matching::new("bb1", "bb1")),
        Match::Full(Matching::new("bb10", "bb10")),
        Match::Full(Matching::new("bb11", "bb11")),
        Match::Full(Matching::new("bb12", "bb12")),
        Match::Full(Matching::new("bb13", "bb13")),
        Match::Full(Matching::new("bb14", "bb14")),
        Match::Full(Matching::new("bb18", "bb7")),
        Match::Full(Matching::new("bb2", "bb2")),
        Match::Full(Matching::new("bb26", "bb15")),
        Match::Full(Matching::new("bb3", "bb3")),
        Match::Full(Matching::new("bb4", "bb4")),
        Match::Full(Matching::new("bb5", "bb5")),
        Match::Full(Matching::new("bb6", "bb6")),
        Match::Full(Matching::new("bb8", "bb8")),
        Match::Full(Matching::new("bb9", "bb9")),
    ];

    // dbg!("{:#?}", mapping);
    assert_eq!(mapping, expected);

    let settings: GraphvizSettings = Default::default();
    let mut f1 = std::fs::File::create("test1.dot").expect("create failed");
    let mut f2 = std::fs::File::create("test2.dot").expect("create failed");
    g1.to_dot(&mut f1, &settings, false).expect("can't fail");
    g2.to_dot(&mut f2, &settings, false).expect("can't fail");
}

#[test]
fn test_diff_vis() {
    let g1 = read_graph_from_file("tests/graph1.json");
    let g2 = read_graph_from_file("tests/graph2.json");

    let d1 = DiffGraph::new(&g1);
    let d2 = DiffGraph::new(&g2);
    let settings: GraphvizSettings = Default::default();

    let mut f1 = std::fs::File::create("test1.dot").expect("create failed");
    let mg = visualize_diff(&d2, &d1);

    mg.to_dot(&mut f1, &settings).unwrap();
}
