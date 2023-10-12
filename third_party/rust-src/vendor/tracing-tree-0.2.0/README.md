# tracing-tree

Instrument your application with [tracing](https://github.com/tokio-rs/tracing)
and get tree-structured summaries of your application activity with timing
information on the console:

<pre> 
 <b>server</b>{host=&quot;localhost&quot;, port=8080<b>}</b>
    0ms <b> INFO</b> starting
    300ms <b> INFO</b> listening
    <b>conn</b>{peer_addr=&quot;82.9.9.9&quot;, port=42381<b>}</b>
      0ms <b>DEBUG</b> connected
      300ms <b>DEBUG</b> message received, length=2
    <b>conn</b>{peer_addr=&quot;8.8.8.8&quot;, port=18230<b>}</b>
      300ms <b>DEBUG</b> connected
    <b>conn</b>{peer_addr=&quot;82.9.9.9&quot;, port=42381<b>}</b>
      600ms <b> WARN</b> weak encryption requested, algo=&quot;xor&quot;
      901ms <b>DEBUG</b> response sent, length=8
      901ms <b>DEBUG</b> disconnected
    <b>conn</b>{peer_addr=&quot;8.8.8.8&quot;, port=18230<b>}</b>
      600ms <b>DEBUG</b> message received, length=5
      901ms <b>DEBUG</b> response sent, length=8
      901ms <b>DEBUG</b> disconnected
    1502ms <b> WARN</b> internal error
    1502ms <b> INFO</b> exit
</pre>

(Format inspired by [slog-term](https://github.com/slog-rs/slog#terminal-output-example))

## Setup

After instrumenting your app with
[tracing](https://github.com/tokio-rs/tracing), add this subscriber like this:

```rust
let subscriber = Registry::default().with(HierarchicalLayer::new(2));
tracing::subscriber::set_global_default(subscriber).unwrap();
```