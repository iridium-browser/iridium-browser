# Third-Party Dependencies

The Open Screen library includes its dependencies as DEPS in the source
tree under the `//third_party/` directory.  They are structured as follows:

```
  //third_party/<library>
   BUILD.gn
   LICENSE
   README.chromium
   ...other necessary adapter files...
   src/
     <library>'s source
```

## Adding a new dependency

When adding a new dependency to the project, you should first add an entry
to the DEPS file. For example, let's say we want to add a
new library called `alpha`. Opening up DEPS, you would add

``` python
  deps = {
    ...
    'src/third_party/alpha/src': 'https://repo.com/path/to/alpha.git'
        + '@' + '<revision>'
```

Then you need to add `BUILD.gn` and `README.chromium` file for it under
`//third_party/alpha`, assuming it doesn't already provide its own `BUILD.gn`.
You will also need to provide a `LICENSE` for it if it does not have its own.

You will also need to add a gitmodule entry for the new dependency.  Follow the
[Chromium instructions](https://chromium.googlesource.com/chromium/src/+/main/docs/dependencies.md#regenerating-git-submodules)
to synchronize gitmodules with DEPS.

Finally, add a new entry for the `src` directory of your dependency to
the `//third_party/.gitignore`.

Commit all of these changes and upload the CL for review.  New dependencies have
impacts to binary size, code health, portability, and security so expect some
discussion in the review before the new dependency is approved.

## Roll a dependency to a new version

See [roll_deps.md](../docs/roll_deps.md) for instructions.

## Removing a dependency

See [Chromium documentation](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/dependencies.md#Deleting-dependencies)
for instructions.
