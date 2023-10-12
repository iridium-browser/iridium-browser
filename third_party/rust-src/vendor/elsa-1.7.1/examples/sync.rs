use elsa::sync::*;

use std::sync::Arc;
use std::thread;
use std::time::Duration;

fn main() {
    let a = Arc::new(FrozenMap::new());
    for i in 1..10 {
        let b = a.clone();
        thread::spawn(move || {
            b.insert(i, i.to_string());
            thread::sleep(Duration::from_millis(300));
            loop {
                if let Some(opposite) = b.get(&(10 - i)) {
                    assert!(opposite.parse::<i32>().unwrap() == 10 - i);
                    break;
                } else {
                    thread::sleep(Duration::from_millis(200));
                }
            }
        });
    }

    thread::sleep(Duration::from_millis(1000));
}
