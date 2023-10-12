# gsgdt

**G**eneric **S**tringly typed **G**raph **D**ata**T**ype

```rust
fn main() {
        let label1: String = "bb0__0_3".into();
        let label2: String = "bb0__1_3".into();
        let style: NodeStyle = Default::default();

        let nodes = vec![
            Node::new(
                vec!["_1 = const 1_i32".into(), "_2 = const 2_i32".into()],
                label1.clone(),
                "0".into(),
                style.clone(),
            ),
            Node::new(
                vec!["return".into()],
                label2.clone(),
                "1".into(),
                style.clone(),
            ),
        ];

        let g = Graph::new(
            "Mir_0_3".into(),
            GraphKind::Digraph,
            nodes,
            vec![Edge::new(label1, label2, "return".into())],
        );
}
```
