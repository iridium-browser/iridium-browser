use tracing::{debug, error, info, instrument, span, warn, Level};
use tracing_subscriber::{layer::SubscriberExt, registry::Registry};
use tracing_tree::HierarchicalLayer;

fn main() {
    let layer = HierarchicalLayer::default()
        .with_writer(std::io::stdout)
        .with_indent_lines(true)
        .with_indent_amount(2)
        .with_thread_names(true)
        .with_thread_ids(true)
        .with_verbose_exit(false)
        .with_verbose_entry(false)
        .with_targets(true);

    let subscriber = Registry::default().with(layer);
    tracing::subscriber::set_global_default(subscriber).unwrap();

    let app_span = span!(Level::TRACE, "hierarchical-example", version = %0.1);
    let _e = app_span.enter();

    let server_span = span!(Level::TRACE, "server", host = "localhost", port = 8080);
    let _e2 = server_span.enter();
    info!("starting");
    std::thread::sleep(std::time::Duration::from_millis(300));
    info!("listening");
    let peer1 = span!(Level::TRACE, "conn", peer_addr = "82.9.9.9", port = 42381);
    peer1.in_scope(|| {
        debug!("connected");
        std::thread::sleep(std::time::Duration::from_millis(300));
        debug!(length = 2, "message received");
    });
    drop(peer1);
    let peer2 = span!(Level::TRACE, "conn", peer_addr = "8.8.8.8", port = 18230);
    peer2.in_scope(|| {
        std::thread::sleep(std::time::Duration::from_millis(300));
        debug!("connected");
    });
    drop(peer2);
    let peer3 = span!(
        Level::TRACE,
        "foomp",
        normal_var = 43,
        "{} <- format string",
        42
    );
    peer3.in_scope(|| {
        error!("hello");
    });
    drop(peer3);
    let peer1 = span!(Level::TRACE, "conn", peer_addr = "82.9.9.9", port = 42381);
    peer1.in_scope(|| {
        warn!(algo = "xor", "weak encryption requested");
        std::thread::sleep(std::time::Duration::from_millis(300));
        debug!(length = 8, "response sent");
        debug!("disconnected");
    });
    drop(peer1);
    let peer2 = span!(Level::TRACE, "conn", peer_addr = "8.8.8.8", port = 18230);
    peer2.in_scope(|| {
        debug!(length = 5, "message received");
        std::thread::sleep(std::time::Duration::from_millis(300));
        debug!(length = 8, "response sent");
        debug!("disconnected");
    });
    drop(peer2);
    warn!("internal error");
    info!("exit");
}

#[instrument]
fn call_a(name: &str) {
    info!(name, "got a name");
    call_b(name)
}

#[instrument]
fn call_b(name: &str) {
    info!(name, "got a name");
}
