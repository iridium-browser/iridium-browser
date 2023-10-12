use crate::util::escape_html;
use std::io::{self, Write};
use serde::{Deserialize, Serialize};

/// NodeStyle defines some style of [Node](struct.Node.html)
#[derive(Clone, Serialize, Deserialize)]
pub struct NodeStyle {
    /// Override the title color of the title
    /// To color the title of the node differently in graphviz
    pub title_bg: Option<String>,

    /// Print a seperator b/w the rest of the statements and the last one
    pub last_stmt_sep: bool,
}

impl Default for NodeStyle {
    fn default() -> NodeStyle {
        NodeStyle {
            title_bg: None,
            last_stmt_sep: false,
        }
    }
}

/// A graph node
#[derive(Clone, Serialize, Deserialize)]
pub struct Node {
    /// A list of statements.
    pub stmts: Vec<String>,

    /// A unique identifier for the given node.
    pub label: String,

    /// The title is printed on the top of BB, the index of the basic block
    pub(crate) title: String,

    /// Can be used to override the default styles
    pub(crate) style: NodeStyle,
}

impl Node {
    pub fn new(stmts: Vec<String>, label: String, title: String, style: NodeStyle) -> Node {
        Node {
            stmts,
            label,
            title,
            style,
        }
    }

    pub fn to_dot<W: Write>(&self, w: &mut W) -> io::Result<()> {
        write!(w, r#"<table border="0" cellborder="1" cellspacing="0">"#)?;

        let bg_attr = match &self.style.title_bg {
            Some(color) => format!(r#"bgcolor="{}""#, color),
            None => "".into(),
        };
        write!(
            w,
            r#"<tr><td {bg_attr} {attrs} colspan="{colspan}">{blk}</td></tr>"#,
            attrs = r#"align="center""#,
            // TODO: Not sure what this is for
            colspan = 1,
            blk = self.title,
            bg_attr = bg_attr
        )?;

        let stmts_len = self.stmts.len();
        if !self.stmts.is_empty() {
            if self.stmts.len() > 1 {
                write!(w, r#"<tr><td align="left" balign="left">"#)?;
                for statement in &self.stmts[..stmts_len - 1] {
                    write!(w, "{}<br/>", escape_html(statement))?;
                }
                write!(w, "</td></tr>")?;
            }

            if !self.style.last_stmt_sep {
                write!(w, r#"<tr><td align="left">"#)?;
                write!(w, "{}", escape_html(&self.stmts[stmts_len - 1]))?;
            } else {
                write!(w, r#"<tr><td align="left" balign="left">"#)?;
                write!(w, "{}", escape_html(&self.stmts[stmts_len - 1]))?;
            }
            write!(w, "</td></tr>")?;
        }

        write!(w, "</table>")
    }
}

/// A directed graph edge
#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct Edge {
    /// The label of the source node of the edge.
    pub from: String,

    /// The label of the target node of the edge.
    pub to: String,

    /// The label (title) of the edge. This doesn't have to unique.
    // TODO: Rename this to title?
    pub label: String,
}

impl Edge {
    pub fn new(from: String, to: String, label: String) -> Edge {
        Edge { from, to, label }
    }

    pub fn to_dot<W: Write>(&self, w: &mut W) -> io::Result<()> {
        writeln!(
            w,
            r#"    {} -> {} [label="{}"];"#,
            self.from, self.to, self.label
        )
    }
}
