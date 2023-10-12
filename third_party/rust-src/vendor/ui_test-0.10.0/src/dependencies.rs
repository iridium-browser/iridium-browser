use cargo_metadata::{camino::Utf8PathBuf, DependencyKind};
use cargo_platform::Cfg;
use color_eyre::eyre::{bail, Result};
use std::{
    collections::{HashMap, HashSet},
    path::PathBuf,
    process::Command,
    str::FromStr,
};

use crate::{Config, Mode, OutputConflictHandling};

#[derive(Default, Debug)]
pub struct Dependencies {
    /// All paths that must be imported with `-L dependency=`. This is for
    /// finding proc macros run on the host and dependencies for the target.
    pub import_paths: Vec<PathBuf>,
    /// The name as chosen in the `Cargo.toml` and its corresponding rmeta file.
    pub dependencies: Vec<(String, Vec<Utf8PathBuf>)>,
}

fn cfgs(config: &Config) -> Result<Vec<Cfg>> {
    let mut cmd = config.cfgs.build();
    cmd.arg("--target").arg(config.target.as_ref().unwrap());
    let output = cmd.output()?;
    let stdout = String::from_utf8(output.stdout)?;

    if !output.status.success() {
        let stderr = String::from_utf8(output.stderr)?;
        bail!(
            "failed to obtain `cfg` information from {cmd:?}:\nstderr:\n{stderr}\n\nstdout:{stdout}"
        );
    }
    let mut cfgs = vec![];

    for line in stdout.lines() {
        cfgs.push(Cfg::from_str(line)?);
    }

    Ok(cfgs)
}

/// Compiles dependencies and returns the crate names and corresponding rmeta files.
pub fn build_dependencies(config: &mut Config) -> Result<Dependencies> {
    let manifest_path = match &config.dependencies_crate_manifest_path {
        Some(path) => path.to_owned(),
        None => return Ok(Default::default()),
    };
    let manifest_path = &manifest_path;
    config.fill_host_and_target()?;
    eprintln!("   Building test dependencies...");
    let mut build = config.dependency_builder.build();

    if let Some(target) = &config.target {
        build.arg(format!("--target={target}"));
    }

    // Reusable closure for setting up the environment both for artifact generation and `cargo_metadata`
    let setup_command = |cmd: &mut Command| {
        cmd.arg("--manifest-path").arg(manifest_path);
        match (&config.output_conflict_handling, &config.mode) {
            (_, Mode::Yolo) => {}
            (OutputConflictHandling::Error, _) => {
                cmd.arg("--locked");
            }
            _ => {}
        }
    };

    setup_command(&mut build);
    build.arg("--message-format=json");

    let output = build.output()?;

    if !output.status.success() {
        let stdout = String::from_utf8(output.stdout)?;
        let stderr = String::from_utf8(output.stderr)?;
        bail!("failed to compile dependencies:\nstderr:\n{stderr}\n\nstdout:{stdout}");
    }

    // Collect all artifacts generated
    let artifact_output = output.stdout;
    let artifact_output = String::from_utf8(artifact_output)?;
    let mut import_paths: HashSet<PathBuf> = HashSet::new();
    let mut artifacts: HashMap<_, _> = artifact_output
        .lines()
        .filter_map(|line| {
            let message = serde_json::from_str::<cargo_metadata::Message>(line).ok()?;
            if let cargo_metadata::Message::CompilerArtifact(artifact) = message {
                for filename in &artifact.filenames {
                    import_paths.insert(filename.parent().unwrap().into());
                }
                Some((artifact.package_id, artifact.filenames))
            } else {
                None
            }
        })
        .collect();

    // Check which crates are mentioned in the crate itself
    let mut metadata = cargo_metadata::MetadataCommand::new().cargo_command();
    config.dependency_builder.apply_env(&mut metadata);
    setup_command(&mut metadata);
    let output = metadata.output()?;

    if !output.status.success() {
        let stdout = String::from_utf8(output.stdout)?;
        let stderr = String::from_utf8(output.stderr)?;
        bail!("failed to run cargo-metadata:\nstderr:\n{stderr}\n\nstdout:{stdout}");
    }

    let output = output.stdout;
    let output = String::from_utf8(output)?;

    let cfg = cfgs(config)?;

    for line in output.lines() {
        if !line.starts_with('{') {
            continue;
        }
        let metadata: cargo_metadata::Metadata = serde_json::from_str(line)?;
        // Only take artifacts that are defined in the Cargo.toml

        // First, find the root artifact
        let root = metadata
            .packages
            .iter()
            .find(|package| {
                package.manifest_path.as_std_path().canonicalize().unwrap()
                    == manifest_path.canonicalize().unwrap()
            })
            .unwrap();

        // Then go over all of its dependencies
        let dependencies = root
            .dependencies
            .iter()
            .filter(|dep| matches!(dep.kind, DependencyKind::Normal))
            // Only consider dependencies that are enabled on the current target
            .filter(|dep| match &dep.target {
                Some(platform) => platform.matches(config.target.as_ref().unwrap(), &cfg),
                None => true,
            })
            .map(|dep| {
                let package = metadata
                    .packages
                    .iter()
                    .find(|&p| p.name == dep.name && dep.req.matches(&p.version))
                    .expect("dependency does not exist");
                (
                    package,
                    dep.rename.clone().unwrap_or_else(|| package.name.clone()),
                )
            })
            // Also expose the root crate
            .chain(std::iter::once((root, root.name.clone())))
            .filter_map(|(package, name)| {
                // Get the id for the package matching the version requirement of the dep
                let id = &package.id;
                // Return the name chosen in `Cargo.toml` and the path to the corresponding artifact
                match artifacts.remove(id) {
                    Some(artifacts) => Some((name.replace('-', "_"), artifacts)),
                    None => {
                        if name == root.name {
                            // If there are no artifacts, this is the root crate and it is being built as a binary/test
                            // instead of a library. We simply add no artifacts, meaning you can't depend on functions
                            // and types declared in the root crate.
                            None
                        } else {
                            panic!("no artifact found for `{name}`(`{id}`):`\n{artifact_output}")
                        }
                    }
                }
            })
            .collect();
        let import_paths = import_paths.into_iter().collect();
        return Ok(Dependencies {
            dependencies,
            import_paths,
        });
    }

    bail!("no json found in cargo-metadata output")
}
