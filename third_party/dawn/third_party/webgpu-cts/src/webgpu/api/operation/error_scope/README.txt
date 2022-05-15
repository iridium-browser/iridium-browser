TODO: plan and implement
- test very deeply nested error scopes, make sure errors go to the right place, e.g.
    - validation, ..., validation, out-of-memory
    - out-of-memory, validation, ..., validation
    - out-of-memory, ..., out-of-memory, validation
    - validation, out-of-memory, ..., out-of-memory
- use error scopes on two different threads and make sure errors go to the right place
- unhandled errors always go to the "original" device object
    - test they go nowhere if the original was dropped (attemptGarbageCollection to make sure)
