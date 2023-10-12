use tracing::{instrument, warn};
use tracing_subscriber::{layer::SubscriberExt, registry::Registry};
use tracing_tree::HierarchicalLayer;

fn main() {
    let layer = HierarchicalLayer::default()
        .with_writer(std::io::stdout)
        .with_indent_lines(true)
        .with_indent_amount(2)
        .with_thread_names(true)
        .with_thread_ids(true)
        .with_targets(true)
        .with_wraparound(5);

    let subscriber = Registry::default().with(layer);
    tracing::subscriber::set_global_default(subscriber).unwrap();

    recurse(0);
}

#[instrument]
fn recurse(i: usize) {
    warn!("boop");
    if i > 20 {
        warn!("bop");
        return;
    } else {
        recurse(i + 1);
    }
    warn!("bop");
}
