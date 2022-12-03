#!/usr/bin/env lucicfg

lucicfg.check_version("1.30.9", "Please update depot_tools")

# Use LUCI Scheduler BBv2 names and add Scheduler realms configs.
lucicfg.enable_experiment("crbug.com/1182002")

lucicfg.config(
    config_dir = "generated",
    tracked_files = [
        "commit-queue.cfg",
        "cr-buildbucket.cfg",
        "project.cfg",
        "luci-logdog.cfg",
        "luci-milo.cfg",
        "luci-scheduler.cfg",
        "realms.cfg",
    ],
    fail_on_warnings = True,
)

luci.project(
    name = "gn",
    buildbucket = "cr-buildbucket.appspot.com",
    logdog = "luci-logdog",
    milo = "luci-milo",
    scheduler = "luci-scheduler",
    swarming = "chromium-swarm.appspot.com",
    acls = [
        acl.entry(
            [
                acl.BUILDBUCKET_READER,
                acl.LOGDOG_READER,
                acl.PROJECT_CONFIGS_READER,
                acl.SCHEDULER_READER,
            ],
            groups = ["all"],
        ),
        acl.entry([acl.SCHEDULER_OWNER], groups = ["project-gn-committers"]),
        acl.entry([acl.LOGDOG_WRITER], groups = ["luci-logdog-chromium-writers"]),
    ],
)

def builder(name, bucket, os, caches = None, triggered_by = None):
    luci.builder(
        name = name,
        bucket = bucket,
        executable = luci.recipe(
            name = "gn",
            cipd_package = "infra/recipe_bundles/gn.googlesource.com/gn",
            cipd_version = "refs/heads/main",
        ),
        caches = caches,
        service_account = "gn-%s-builder@chops-service-accounts.iam.gserviceaccount.com" % bucket,
        execution_timeout = 1 * time.hour,
        dimensions = {"cpu": "x86-64", "os": os, "pool": "luci.flex.%s" % bucket},
        triggered_by = triggered_by,
        experiments = {
            "luci.recipes.use_python3": 100,
        },
    )

luci.logdog(
    gs_bucket = "chromium-luci-logdog",
)

luci.milo(
    logo = "https://storage.googleapis.com/chrome-infra-public/logo/gn-logo.png",
)

luci.console_view(
    name = "gn",
    title = "gn",
    repo = "https://gn.googlesource.com/gn",
    refs = ["refs/heads/main"],
    favicon = "https://storage.googleapis.com/chrome-infra-public/logo/favicon.ico",
)

luci.gitiles_poller(
    name = "gn-trigger",
    bucket = "ci",
    repo = "https://gn.googlesource.com/gn",
    refs = ["refs/heads/main"],
)

luci.bucket(name = "ci", acls = [
    acl.entry(
        [acl.BUILDBUCKET_TRIGGERER],
    ),
])

def ci_builder(name, os, caches = None):
    builder(name, "ci", os, caches, triggered_by = ["gn-trigger"])
    luci.console_view_entry(
        console_view = "gn",
        builder = "ci/" + name,
        short_name = name,
    )

ci_builder("linux", "Ubuntu-18.04")
ci_builder("mac", "Mac-10.15", caches = [swarming.cache("macos_sdk")])
ci_builder("win", "Windows-10", caches = [swarming.cache("windows_sdk")])

luci.cq(
    submit_max_burst = 4,
    submit_burst_delay = 8 * time.minute,
)

luci.cq_group(
    name = "gn",
    watch = cq.refset(
        repo = "https://gn.googlesource.com/gn",
        refs = ["refs/heads/main"],
    ),
    acls = [
        acl.entry(
            [acl.CQ_COMMITTER],
            groups = ["project-gn-committers"],
        ),
        acl.entry(
            [acl.CQ_DRY_RUNNER],
            groups = ["project-gn-tryjob-access"],
        ),
    ],
    retry_config = cq.retry_config(
        single_quota = 1,
        global_quota = 2,
        failure_weight = 1,
        transient_failure_weight = 1,
        timeout_weight = 2,
    ),
)

luci.bucket(name = "try", acls = [
    acl.entry(
        [acl.BUILDBUCKET_TRIGGERER],
        groups = ["project-gn-tryjob-access", "service-account-cq"],
    ),
])

luci.binding(
    realm = "try",
    roles = "role/swarming.taskTriggerer",
    groups = "flex-try-led-users",
)

def try_builder(name, os, caches = None):
    builder(name, "try", os, caches)
    luci.cq_tryjob_verifier(
        builder = "try/" + name,
        cq_group = "gn",
    )

try_builder("linux", "Ubuntu-18.04")
try_builder("mac", "Mac-10.15", caches = [swarming.cache("macos_sdk")])
try_builder("win", "Windows-10", caches = [swarming.cache("windows_sdk")])
