#!/usr/bin/env lucicfg
"""
Open Screen's LUCI configuration for post-submit and pre-submit builders.
"""

REPO_URL = "https://chromium.googlesource.com/openscreen"
CHROMIUM_REPO_URL = "https://chromium.googlesource.com/chromium/src"
MAC_VERSION = "Mac-11"
REF = "refs/heads/main"

# Use LUCI Scheduler BBv2 names and add Scheduler realms configs.
lucicfg.enable_experiment("crbug.com/1182002")

luci.project(
    name = "openscreen",
    milo = "luci-milo.appspot.com",
    buildbucket = "cr-buildbucket.appspot.com",
    logdog = "luci-logdog.appspot.com",
    swarming = "chromium-swarm.appspot.com",
    scheduler = "luci-scheduler.appspot.com",
    acls = [
        acl.entry(
            roles = [
                acl.BUILDBUCKET_READER,
                acl.SCHEDULER_READER,
                acl.PROJECT_CONFIGS_READER,
                acl.LOGDOG_READER,
            ],
            groups = "all",
        ),
        acl.entry(
            roles = acl.SCHEDULER_OWNER,
            groups = "project-openscreen-admins",
        ),
        acl.entry(
            roles = acl.LOGDOG_WRITER,
            groups = "luci-logdog-chromium-writers",
        ),
        acl.entry(
            roles = acl.CQ_COMMITTER,
            groups = "project-openscreen-committers",
        ),
        acl.entry(
            roles = acl.CQ_DRY_RUNNER,
            groups = "project-openscreen-tryjob-access",
        ),
    ],
)

luci.milo(
    logo = ("https://storage.googleapis.com/chrome-infra-public/logo/" +
            "openscreen-logo.png"),
)

luci.logdog(gs_bucket = "chromium-luci-logdog")

# Gitiles pollers are used for triggering CI builders.
luci.gitiles_poller(
    name = "master-gitiles-trigger",
    bucket = "ci",
    repo = REPO_URL,
)
luci.gitiles_poller(
    name = "chromium-trigger",
    bucket = "ci",
    repo = CHROMIUM_REPO_URL,
)

# Whereas tryjob verifiers are used for triggering try builders.
luci.cq_group(
    name = "openscreen-build-config",
    watch = cq.refset(
        repo = REPO_URL,
        refs = ["refs/heads/.+"],
    ),
)
luci.cq(status_host = "chromium-cq-status.appspot.com")

luci.bucket(
    name = "try",
    acls = [
        acl.entry(
            roles = [acl.BUILDBUCKET_TRIGGERER, acl.CQ_COMMITTER],
            groups = [
                "project-openscreen-tryjob-access",
                "service-account-cq",
            ],
        ),
    ],
)
luci.bucket(
    name = "ci",
    acls = [
        acl.entry(
            roles = [acl.BUILDBUCKET_TRIGGERER],
            users = "luci-scheduler@appspot.gserviceaccount.com",
        ),
    ],
)

luci.console_view(
    name = "ci",
    title = "OpenScreen CI Builders",
    repo = REPO_URL,
)
luci.console_view(
    name = "try",
    title = "OpenScreen Try Builders",
    repo = REPO_URL,
)

def get_properties(
        is_release = False,
        is_gcc = False,
        is_asan = False,
        is_tsan = False,
        use_coverage = False,
        use_sysroot = False,
        target_cpu = "x64",
        cast_standalone = False,
        chromium = False,
        use_goma = True,
        use_ats = True,
        is_presubmit = False):
    """Returns a collection of GN properties for the build system."""
    properties = {
        "target_cpu": target_cpu,
        "$depot_tools/bot_update": {
            "apply_patch_on_gclient": True,
        },
        "$recipe_engine/swarming": {
            "server": "https://chromium-swarm.appspot.com",
        },
    }
    if not is_release:
        properties["is_debug"] = True
    if is_gcc:
        properties["is_gcc"] = True
    if is_asan:
        properties["is_asan"] = True
    if is_tsan:
        properties["is_tsan"] = True
    if use_coverage:
        properties["use_coverage"] = True
    if use_sysroot:
        properties["sysroot_platform"] = "stretch"
    if cast_standalone:
        properties["have_ffmpeg"] = True
        properties["have_libsdl2"] = True
        properties["have_libopus"] = True
        properties["have_libvpx"] = True
        properties["cast_allow_developer_certificate"] = True
    if chromium:
        properties["builder_group"] = "client.openscreen.chromium"
    if use_goma:
        properties["$build/goma"] = {
            "server_host": "goma.chromium.org",
            "rpc_extra_params": "?prod",
        }
        if use_ats:
            properties["$build/goma"]["enable_ats"] = True
    else:
        properties["use_goma"] = False
    if is_presubmit:
        properties["repo_name"] = "openscreen"
        properties["runhooks"] = "true"
    return properties

def builder(builder_type, name, properties, os, cpu):
    """Defines a builder.
       Args:
        builder_type: "ci" or "try".
        name: name of the builder to define.
        properties: configuration to be passed to GN.
        os: the target operating system.
        cpu: the target central processing unit.
    """
    recipe_id = "openscreen"
    use_python3 = True
    if properties:
        if "builder_group" in properties:
            recipe_id = "chromium"
        elif "runhooks" in properties:
            recipe_id = "run_presubmit"

    caches = []
    if os == MAC_VERSION:
        caches.append(swarming.cache("osx_sdk"))

    triggers = None
    if builder_type == "ci":
        triggers = ["chromium-trigger" if recipe_id == "chromium" else "master-gitiles-trigger"]

    luci.builder(
        name = name,
        bucket = builder_type,
        executable = luci.recipe(
            name = recipe_id,
            recipe = recipe_id,
            cipd_package =
                "infra/recipe_bundles/chromium.googlesource.com/chromium/tools/build",
            cipd_version = "refs/heads/master",
            use_bbagent = True,
            use_python3 = use_python3,
        ),
        dimensions = {
            "pool": "luci.flex." + builder_type,
            "os": os,
            "cpu": cpu,
        },
        caches = caches,
        properties = properties,
        service_account =
            "openscreen-{}-builder@chops-service-accounts.iam.gserviceaccount.com"
                .format(builder_type),
        triggered_by = triggers,
    )

    # CI jobs get triggered by |triggers|, try jobs get trigged by the commit
    # queue instead.
    if builder_type == "try":
        # We mark some bots as experimental to not block the build.
        experiment_percentage = None
        if name in ["linux64_cast_e2e", "linux64_coverage_debug"]:
            experiment_percentage = 100

        luci.cq_tryjob_verifier(
            builder = "try/" + name,
            cq_group = "openscreen-build-config",
            experiment_percentage = experiment_percentage,
        )

    luci.console_view_entry(
        builder = "{}/{}".format(builder_type, name),
        console_view = builder_type,
        category = "{}|{}".format(os, cpu),
        short_name = name,
    )

def ci_builder(name, properties, os = "Ubuntu-18.04", cpu = "x86-64"):
    """Defines a post submit builder.
       Args:
        name: name of the builder to define.
        properties: configuration to be passed to GN.
        os: the target operating system.
        cpu: the target central processing unit.
    """
    builder("ci", name, properties, os, cpu)

def try_builder(name, properties, os = "Ubuntu-18.04", cpu = "x86-64"):
    """Defines a pre submit builder.
       Args:
        name: name of the builder to define.
        properties: configuration to be passed to GN.
        os: the target operating system.
        cpu: the target central processing unit.
    """
    builder("try", name, properties, os, cpu)

def try_and_ci_builders(name, properties, os = "Ubuntu-18.04", cpu = "x86-64"):
    """Defines a similarly configured try and ci bulider pair.
       Args:
        name: name of the builder to define.
        properties: configuration to be passed to GN.
        os: the target operating system.
        cpu: the target central processing unit.
    """
    try_builder(name, properties, os, cpu)

    # NOTE: properties is immutable at this point and must be copied.
    ci_properties = dict(properties)
    ci_properties["is_ci"] = True
    ci_builder(name, ci_properties, os, cpu)

# BUILDER CONFIGURATIONS
try_builder(
    "openscreen_presubmit",
    get_properties(is_presubmit = True, is_release = True, use_goma = False),
)
try_and_ci_builders("linux64_cast_e2e", get_properties(cast_standalone = True))
try_and_ci_builders("linux64_coverage_debug", get_properties(use_coverage = True))
try_and_ci_builders("linux64_debug", get_properties(is_asan = True))
try_and_ci_builders(
    "linux64_gcc_debug",
    get_properties(is_gcc = True, use_goma = False),
)
try_and_ci_builders(
    "linux64_tsan",
    get_properties(is_release = True, is_tsan = True),
)
try_and_ci_builders(
    "linux_arm64_debug",
    get_properties(target_cpu = "arm64", use_sysroot = True),
)
try_and_ci_builders("mac_debug", get_properties(use_ats = False), os = MAC_VERSION)
try_and_ci_builders("chromium_linux64_debug", get_properties(chromium = True))
try_and_ci_builders(
    "chromium_mac_debug",
    get_properties(chromium = True, use_ats = False),
    os = MAC_VERSION,
)
