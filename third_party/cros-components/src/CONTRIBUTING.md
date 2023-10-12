
# How to Contribute

Cros components is not accepting any external contributions at the moment.

## Readiness checklist

A component that is ready to be used by clients has implemented the following

- [ ] Renders and behaves according to the spec.
- [ ] The various states from the spec are testable (and therefore tested) with scuba.
- [ ] Emits events for interesting state transitions. The target of the event should be the top level component.
- [ ] Is selectable in queries using its state. E.g. `document.querySelector('cros-jellybean[flavour="broccoli"]')`
- [ ] Handles programmatic clicks in tests
  - [ ] `.click()` and `.dispatchEvent('click', event)` should both work.
