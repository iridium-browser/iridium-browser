#!/usr/bin/env lucicfg
#
# This is the LUCI configuration for the 'chromium-website' project,
# the set of machines that build and test changes for the static website
# deployed to serve www.chromium.org.
#
# The chromium website needs basically the simplest possible LUCI project:
# one presubmit (aka "try") builder and one postsubmit (aka "CI" or
# continuous integration) builder, with the following conventions:
#
# - The project is called PROJECT_NAME.
# - The repo containing the LUCI configuration is found in PROJECT_REPO.
# - The recipes are found in the same repo as the rest of the LUCI config
#   (which is likely also the main repo for the project source code).
# - The builders will run in LUCI's public flex pools.
# - The builders will be configured into two different buckets, called "try"
# - and "ci"
# - The builders will be named $PROJECT_NAME-$BUCKET-builder
# - The builders will use the same recipe, found in
#   //infra/config/recipes/$PROJECT_NAME.py

# This should match the LUCI project name.
#
# In order to aid with grepping for occurrences of the project name
# in various other names, we repeat the string, below in some places,
# rather than using string interpolation. This means that if you change
# the project name, you should also do a search-and-replace for other
# occurrences of the string in this file.
PROJECT_NAME = "chromium-website"

PROJECT_REPO = "https://chromium.googlesource.com/website"

# TODO(crbug.com/1457690): Update this when/if you get a custom logo.
PROJECT_LOGO = "https://storage.googleapis.com/chrome-infra-public/logo/chromium.svg"

RECIPE_CIPD_PACKAGE = "infra/recipe_bundles/chromium.googlesource.com/chromium/tools/build"
RECIPE_NAME = "chromium_website"

#
# Everything below this comment should be almost completely generic; however,
# in some places we explicitly expand $PROJECT_NAME rather than using
# string interpolation to aid in grepping for strings / code search.
# See the note above for more.
#

lucicfg.check_version("1.30.9", "Please update depot_tools")

_LINUX_OS = "Ubuntu-18.04"

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
        "tricium-prod.cfg",
    ],
    fail_on_warnings = True,
)

luci.project(
    name = PROJECT_NAME,
    buildbucket = "cr-buildbucket.appspot.com",
    logdog = "luci-logdog",
    milo = "luci-milo",
    scheduler = "luci-scheduler",
    swarming = "chromium-swarm.appspot.com",
    tricium = "tricium-prod.appspot.com",
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
        acl.entry([acl.SCHEDULER_OWNER], groups = ["project-chromium-website-committers"]),
        acl.entry([acl.LOGDOG_WRITER], groups = ["luci-logdog-chromium-website-writers"]),
    ],
)

luci.logdog(
    gs_bucket = "chromium-luci-logdog",
)

luci.milo(
    logo = PROJECT_LOGO,
)

luci.console_view(
    name = PROJECT_NAME,
    title = PROJECT_NAME,
    repo = PROJECT_REPO,
    refs = ["refs/heads/main"],
    favicon = "https://storage.googleapis.com/chrome-infra-public/logo/favicon.ico",
)

luci.gitiles_poller(
    name = "chromium-website-trigger",
    bucket = "ci",
    repo = PROJECT_REPO,
    refs = ["refs/heads/main"],
)

luci.bucket(name = "ci", acls = [
    acl.entry(
        [acl.BUILDBUCKET_TRIGGERER],
    ),
])

luci.binding(
    realm = "ci",
    roles = "role/swarming.taskTriggerer",
    groups = "flex-ci-led-users",
)

luci.recipe(
    name = RECIPE_NAME,
    cipd_package = RECIPE_CIPD_PACKAGE,
    cipd_version = "refs/heads/main",
    use_bbagent = True,
    use_python3 = True,
)

luci.builder(
    name = "chromium-website-ci-builder",
    bucket = "ci",
    executable = RECIPE_NAME,
    service_account = "chromium-website-ci-builder@chops-service-accounts.iam.gserviceaccount.com",
    execution_timeout = 1 * time.hour,
    dimensions = {"cpu": "x86-64", "os": _LINUX_OS, "pool": "luci.flex.ci"},
    triggered_by = ["chromium-website-trigger"],
    build_numbers = True,
)

luci.console_view_entry(
    console_view = PROJECT_NAME,
    builder = "chromium-website-ci-builder",
    short_name = "ci",
)

luci.cq(
    submit_max_burst = 4,
    submit_burst_delay = 8 * time.minute,
)

luci.cq_group(
    name = PROJECT_NAME,
    watch = cq.refset(
        repo = PROJECT_REPO,
        refs = ["refs/heads/main"],
    ),
    acls = [
        acl.entry(
            [acl.CQ_COMMITTER],
            groups = ["project-chromium-website-committers"],
        ),
        acl.entry(
            [acl.CQ_DRY_RUNNER],
            groups = ["project-chromium-website-tryjob-access"],
        ),
    ],
    retry_config = cq.retry_config(
        single_quota = 1,
        global_quota = 2,
        failure_weight = 1,
        transient_failure_weight = 1,
        timeout_weight = 2,
    ),
    verifiers = [
        luci.cq_tryjob_verifier(
            builder = "chromium-website-try-builder",
            mode_allowlist = [
                cq.MODE_ANALYZER_RUN,
                cq.MODE_DRY_RUN,
                cq.MODE_FULL_RUN,
            ],
        ),
    ],
)

luci.bucket(name = "try", acls = [
    acl.entry(
        [acl.BUILDBUCKET_TRIGGERER],
        groups = ["project-chromium-website-tryjob-access", "service-account-cq"],
        users = ["tricium-prod@appspot.gserviceaccount.com"],
    ),
])

luci.binding(
    realm = "try",
    roles = "role/swarming.taskTriggerer",
    groups = "flex-try-led-users",
)

luci.builder(
    name = "chromium-website-try-builder",
    bucket = "try",
    executable = RECIPE_NAME,
    service_account = "chromium-website-try-builder@chops-service-accounts.iam.gserviceaccount.com",
    execution_timeout = 1 * time.hour,
    dimensions = {"cpu": "x86-64", "os": _LINUX_OS, "pool": "luci.flex.try"},
    build_numbers = True,
)
