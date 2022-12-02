# Lucicfg overview

The way our infrastructure for try jobs and continuous integration works is
currently based on
[lucicfg](https://chromium.googlesource.com/infra/luci/luci-go/+/refs/heads/main/lucicfg/doc/README.md),
which uses the singular [main.star](./main.star) file to generate what used
to be manual `*.cfg` files, such as `cr-buildbucket.cfg`.

## Editing main.star
### Validation
When making edits to the main.star file, after your edits are complete
validation can be done using:

```bash
lucicfg validate main.star
```

Note that this validation is done as part of `git cl presubmit` as well.

### Generation
Regeneration of the `generated` directory files can be done by directly
executing main.star, which essentially runs:

```bash
lucicfg generate main.star
```

### Formatting
Lucicfg has an autoformatter, that can be ran using:

```bash
lucicfg fmt
```