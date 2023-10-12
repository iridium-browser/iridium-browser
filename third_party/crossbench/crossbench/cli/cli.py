# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import datetime as dt
import json
import logging
import pathlib
import sys
import tempfile
import traceback
from typing import (TYPE_CHECKING, Any, Dict, List, Optional, Sequence, Tuple,
                    Type)

from tabulate import tabulate

import crossbench.benchmarks.all as benchmarks
from crossbench import cli_helper, helper
from crossbench.browsers import splash_screen, viewport
from crossbench.browsers.browser_helper import BROWSERS_CACHE
from crossbench.env import (HostEnvironment, HostEnvironmentConfig,
                            ValidationMode)
from crossbench.probes.all import GENERAL_PURPOSE_PROBES, DebuggerProbe
from crossbench.probes.internal import ErrorsProbe
from crossbench.runner.runner import Runner
from crossbench.runner.timing import Timing
from crossbench.benchmarks.benchmark import Benchmark

from . import cli_config
from .devtools_recorder_proxy import CrossbenchDevToolsRecorderProxy

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.probes.probe import Probe
  from crossbench.runner.run import Run
  BenchmarkClsT = Type[Benchmark]
  BrowserLookupTableT = Dict[str, Tuple[Type[Browser], pathlib.Path]]

argparse.ArgumentError = cli_helper.CrossBenchArgumentError


class EnableDebuggingAction(argparse.Action):
  """Custom action to set both --throw and -vvv."""

  def __call__(self, parser, namespace, values, option_string=None):
    setattr(namespace, "throw", True)
    setattr(namespace, "verbosity", 3)


class AppendDebuggerProbeAction(argparse.Action):
  """Custom action to set multiple args when --gdb or --lldb are set:
  - Add a DebuggerProbe config.
  - Increase --timeout-unit to a large value to keep debug session alive for a
    longer time.
  """

  def __call__(self, parser, namespace, values, option_string=None):
    probes: List[cli_config.SingleProbeConfig] = getattr(
        namespace, self.dest, [])
    probe_settings = {"debugger": "gdb"}
    if option_string and "lldb" in option_string:
      probe_settings["debugger"] = "lldb"
    probes.append(cli_config.SingleProbeConfig(DebuggerProbe, probe_settings))
    if not getattr(namespace, "timeout_unit", None):
      # Set a very large --timeout-unit to allow for very slow debugging without
      # causing timeouts (for instance when waiting on a breakpoint).
      setattr(namespace, "timeout_unit", dt.timedelta.max)


class CrossBenchCLI:
  BENCHMARKS: Tuple[BenchmarkClsT, ...] = (
      benchmarks.Speedometer30Benchmark,
      benchmarks.Speedometer21Benchmark,
      benchmarks.Speedometer20Benchmark,
      benchmarks.JetStream21Benchmark,
      benchmarks.JetStream20Benchmark,
      benchmarks.MotionMark12Benchmark,
      benchmarks.PageLoadBenchmark,
      benchmarks.PowerBenchmark,
  )

  RUNNER_CLS: Type[Runner] = Runner

  def __init__(self, enable_logging: bool = True) -> None:
    self._enable_logging = enable_logging
    self._console_handler: Optional[logging.StreamHandler] = None
    self._subparsers: Dict[BenchmarkClsT,
                           cli_helper.CrossBenchArgumentParser] = {}
    self.parser = cli_helper.CrossBenchArgumentParser(
        description=("A cross browser and cross benchmark runner "
                     "with configurable measurement probes."))
    self.help_parser = cli_helper.CrossBenchArgumentParser()
    self.describe_parser = cli_helper.CrossBenchArgumentParser()
    self.recorder_parser = cli_helper.CrossBenchArgumentParser()
    self.args = argparse.Namespace()
    self._setup_parser()
    self._setup_subparser()

  def _setup_parser(self) -> None:
    self._add_verbosity_argument(self.parser)
    # Disable colors by default when piped to a file.
    has_color = hasattr(sys.stdout, "isatty") and sys.stdout.isatty()
    self.parser.add_argument(
        "--no-color",
        dest="color",
        action="store_false",
        default=has_color,
        help="Disable colored output")

  def _add_verbosity_argument(self, parser: argparse.ArgumentParser) -> None:
    debug_group = parser.add_argument_group("Verbosity / Debugging Options")
    verbosity_group = debug_group.add_mutually_exclusive_group()
    verbosity_group.add_argument(
        "--quiet",
        "-q",
        dest="verbosity",
        default=0,
        action="store_const",
        const=-1,
        help="Disable most output printing.")
    verbosity_group.add_argument(
        "--verbose",
        "-v",
        dest="verbosity",
        action="count",
        default=0,
        help=("Increase output verbosity. "
              "Repeat for more verbose output (0..2)."))
    debug_group.add_argument(
        "--throw",
        action="store_true",
        default=False,
        help=("Directly throw exceptions instead. "
              "Note that this prevents merging of probe results if only "
              "a subset of the runs failed."))
    debug_group.add_argument(
        "--debug",
        action=EnableDebuggingAction,
        nargs=0,
        help="Enable debug output, equivalent to --throw -vvv")

    debugger_group = debug_group.add_mutually_exclusive_group()
    debugger_group.add_argument(
        "--gdb",
        action=AppendDebuggerProbeAction,
        nargs=0,
        dest="probe",
        help=("Launch chrome with gdb or lldb attached to all processes. "
              " See 'describe probe debugger' for more options."))
    debugger_group.add_argument(
        "--lldb",
        action=AppendDebuggerProbeAction,
        nargs=0,
        dest="probe",
        help=("Launch chrome with lldb attached to all processes."
              " See 'describe probe debugger' for more options."))

  def _setup_subparser(self) -> None:
    self.subparsers = self.parser.add_subparsers(
        title="Subcommands",
        dest="subcommand",
        required=True,
        parser_class=cli_helper.CrossBenchArgumentParser)
    for benchmark_cls in self.BENCHMARKS:
      self._setup_benchmark_subparser(benchmark_cls)
    self._setup_help_subparser()
    self._setup_describe_subparser()
    self._setup_recorder_subparser()

  def _setup_recorder_subparser(self) -> None:
    self.recorder_parser = CrossbenchDevToolsRecorderProxy.add_subcommand(
        self.subparsers)
    assert isinstance(self.recorder_parser, cli_helper.CrossBenchArgumentParser)
    self._add_verbosity_argument(self.recorder_parser)

  def _setup_describe_subparser(self) -> None:
    self.describe_parser = self.subparsers.add_parser(
        "describe", aliases=["desc"], help="Print all benchmarks and stories")
    assert isinstance(self.describe_parser, cli_helper.CrossBenchArgumentParser)
    self.describe_parser.add_argument(
        "category",
        nargs="?",
        choices=["all", "benchmark", "benchmarks", "probe", "probes"],
        default="all",
        help="Limit output to the given category, defaults to 'all'")
    self.describe_parser.add_argument(
        "filter",
        nargs="?",
        help=("Only display the given item from the provided category. "
              "By default all items are displayed. "
              "Example: describe probes v8.log"))
    self.describe_parser.add_argument(
        "--json",
        default=False,
        action="store_true",
        help="Print the data as json data")
    self.describe_parser.set_defaults(subcommand_fn=self.describe_subcommand)
    self._add_verbosity_argument(self.describe_parser)

  def _setup_help_subparser(self) -> None:
    # Just for completeness we want to support "--help" and "help"
    self.help_parser = self.subparsers.add_parser(
        "help", help="Print the top-level --help")
    assert isinstance(self.describe_parser, cli_helper.CrossBenchArgumentParser)
    self.help_parser.set_defaults(subcommand_fn=self.help_subcommand)
    self._add_verbosity_argument(self.describe_parser)

  def describe_subcommand(self, args: argparse.Namespace) -> None:
    benchmarks_data: Dict[str, Any] = {}
    for benchmark_cls in self.BENCHMARKS:
      aliases: Tuple[str, ...] = benchmark_cls.aliases()
      if args.filter:
        if benchmark_cls.NAME != args.filter and args.filter not in aliases:
          continue
      benchmark_info = benchmark_cls.describe()
      benchmark_info["aliases"] = aliases or "None"
      benchmark_info["help"] = f"See `{benchmark_cls.NAME} --help`"
      benchmarks_data[benchmark_cls.NAME] = benchmark_info
    data: Dict[str, Dict[str, Any]] = {
        "benchmarks": benchmarks_data,
        "probes": {
            str(probe_cls.NAME): probe_cls.help_text()
            for probe_cls in GENERAL_PURPOSE_PROBES
            if not args.filter or probe_cls.NAME == args.filter
        }
    }
    if args.json:
      if args.category in ("probe", "probes"):
        data = data["probes"]
        if not data:
          self.error(f"No matching probe found: '{args.filter}'")
      elif args.category in ("benchmark", "benchmarks"):
        data = data["benchmarks"]
        if not data:
          self.error(f"No matching benchmark found: '{args.filter}'")
      else:
        assert args.category == "all"
        if not data["benchmarks"] and not data["probes"]:
          self.error(f"No matching benchmarks or probes found: '{args.filter}'")
      print(json.dumps(data, indent=2))
      return
    # Create tabular format
    printed_any = False
    if args.category in ("all", "benchmark", "benchmarks"):
      table: List[List[Optional[str]]] = [["Benchmark", "Property", "Value"]]
      for benchmark_name, values in data["benchmarks"].items():
        table.append([
            benchmark_name,
        ])
        for name, value in values.items():
          if isinstance(value, (tuple, list)):
            value = "\n".join(value)
          elif isinstance(value, dict):
            if not value.items():
              value = "[]"
            else:
              kwargs = {"maxcolwidths": 60}
              value = tabulate(value.items(), tablefmt="plain", **kwargs)
          table.append([None, name, value])
      if len(table) <= 1:
        if args.category != "all":
          self.error(f"No matching benchmark found: '{args.filter}'")
      else:
        printed_any = True
        print(tabulate(table, tablefmt="grid"))

    if args.category in ("all", "probe", "probes"):
      table = [["Probe", "Help"]]
      for probe_name, probe_desc in data["probes"].items():
        table.append([probe_name, probe_desc])
      if len(table) <= 1:
        if args.category != "all":
          self.error(f"No matching probe found: '{args.filter}'")
      else:
        printed_any = True
        print(tabulate(table, tablefmt="grid"))

    if not printed_any:
      self.error(f"No matching benchmarks or probes found: '{args.filter}'")

  def help_subcommand(self, args: argparse.Namespace) -> None:
    del args
    self.parser.print_help()
    sys.exit(0)

  def _setup_benchmark_subparser(self, benchmark_cls: Type[Benchmark]) -> None:
    subparser = benchmark_cls.add_cli_parser(self.subparsers,
                                             benchmark_cls.aliases())
    self.RUNNER_CLS.add_cli_parser(benchmark_cls, subparser)
    assert isinstance(subparser, argparse.ArgumentParser), (
        f"Benchmark class {benchmark_cls}.add_cli_parser did not return "
        f"an ArgumentParser: {subparser}")
    self._subparsers[benchmark_cls] = subparser

    runner_group = subparser.add_argument_group("Runner Options", "")
    runner_group.add_argument(
        "--cache-dir",
        type=pathlib.Path,
        default=BROWSERS_CACHE,
        help="Used for caching browser binaries and archives. "
        "Defaults to .browser_cache")

    cooldown_group = runner_group.add_mutually_exclusive_group()
    cooldown_group.add_argument(
        "--cool-down-time",
        "--cool-down",
        type=cli_helper.Duration.parse_zero,
        default=dt.timedelta(seconds=2),
        help="Time the runner waits between different runs or repetitions. "
        "Increase this to let the CPU cool down between runs. "
        f"Format: {cli_helper.Duration.help()}")
    cooldown_group.add_argument(
        "--no-cool-down",
        action="store_const",
        dest="cool_down_time",
        const=dt.timedelta(seconds=0),
        help="Disable cool-down between runs (might cause CPU throttling), "
        "equivalent to --cool-down=0.")

    runner_group.add_argument(
        "--time-unit",
        type=cli_helper.Duration.parse,
        default=dt.timedelta(seconds=1),
        help="Absolute duration of 1 time unit in the runner. "
        "Increase this for slow builds or machines. "
        f"Format: {cli_helper.Duration.help()}")
    runner_group.add_argument(
        "--timeout-unit",
        type=cli_helper.Duration.parse,
        default=dt.timedelta(),
        help="Absolute duration of 1 time unit for timeouts in the runner. "
        "Unlike --time-unit, this does only apply for timeouts, "
        "as opposed to say initial wait times or sleeps."
        f"Format: {cli_helper.Duration.help()}")
    runner_group.add_argument(
        "--run-timeout",
        type=cli_helper.Duration.parse_zero,
        default=dt.timedelta(),
        help="Sets the same timeout per run on all browsers. "
        "Runs will be aborted after the given timeout. "
        f"Format: {cli_helper.Duration.help()}")

    env_group = subparser.add_argument_group("Environment Options", "")
    env_settings_group = env_group.add_mutually_exclusive_group()
    env_settings_group.add_argument(
        "--env",
        type=cli_config.parse_inline_env_config,
        help="Set default runner environment settings. "
        f"Possible values: {', '.join(HostEnvironment.CONFIGS)}"
        "or an inline hjson configuration (see --env-config). "
        "Mutually exclusive with --env-config")
    env_settings_group.add_argument(
        "--env-config",
        type=cli_config.parse_env_config_file,
        help="Path to an env.config.hjson file that specifies detailed "
        "runner environment settings and requirements. "
        "See config/env.config.hjson for more details."
        "Mutually exclusive with --env")

    env_group.add_argument(
        "--env-validation",
        default=ValidationMode.PROMPT,
        type=ValidationMode,
        help=(
            "Set how runner env is validated (see als --env-config/--env):\n" +
            ValidationMode.help_text(indent=2)))
    env_group.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Don't run any browsers or probes")

    browser_group = subparser.add_argument_group(
        "Browser Options", "Any other browser option can be passed "
        "after the '--' arguments separator.")
    browser_config_group = browser_group.add_mutually_exclusive_group()
    browser_config_group.add_argument(
        "--browser",
        "-b",
        type=cli_config.BrowserConfig.parse,
        action="append",
        default=[],
        help="Browser binary. Use this to test a simple browser variant. "
        "Use [chrome, stable, dev, canary, safari] "
        "for system default browsers or a full path. "
        "Repeat for adding multiple browsers. "
        "Defaults to 'chrome-stable'. "
        "Use --browser=chrome-M107 to download the latest milestone, "
        "--browser=chrome-100.0.4896.168 to download a specific chrome version "
        "(macOS and linux for googlers and chrome only). "
        "Use --browser=path/to/archive.dmg on macOS or "
        "--browser=path/to/archive.rpm on linux "
        "for locally cached versions (chrome only)."
        "Cannot be used with --browser-config")
    browser_config_group.add_argument(
        "--browser-config",
        type=cli_helper.parse_hjson_file_path,
        help="Browser configuration.json file. "
        "Use this to run multiple browsers and/or multiple flag configurations."
        "See config/browser.config.example.hjson on how to set up a complex "
        "configuration file. "
        "Cannot be used together with --browser.")
    browser_group.add_argument(
        "--driver-path",
        type=cli_helper.parse_file_path,
        help="Use the same custom driver path for all specified browsers. "
        "Version mismatches might cause crashes.")
    browser_group.add_argument(
        "--config",
        type=cli_helper.parse_hjson_file_path,
        help="Specify a common config for "
        "--probe-config, --browser-config and --env-config.")

    splashscreen_group = browser_group.add_mutually_exclusive_group()
    splashscreen_group.add_argument(
        "--splash-screen",
        "--splashscreen",
        "--splash",
        type=splash_screen.SplashScreen.parse,
        default=splash_screen.SplashScreen.DETAILED,
        help=("Set the splashscreen shown before each run. "
              "Choices: 'default', 'none', 'minimal', 'detailed,' or "
              "a path or a URL."))
    splashscreen_group.add_argument(
        "--no-splash",
        "--nosplash",
        dest="splash_screen",
        const=splash_screen.SplashScreen.NONE,
        action="store_const",
        help="Shortcut for --splash-screen=none")

    viewport_group = browser_group.add_mutually_exclusive_group()
    # pytype: disable=missing-parameter
    viewport_group.add_argument(
        "--viewport",
        default=viewport.Viewport.DEFAULT,
        type=viewport.Viewport.parse,
        help=("Set the browser window position."
              "Options: size and position, "
              f"{', '.join(str(e.value) for e in viewport.ViewportMode)}. "
              "Examples: --viewport=1550x300 --viewport=fullscreen. "
              f"Default: {viewport.Viewport.DEFAULT}"))
    # pytype: enable=missing-parameter
    viewport_group.add_argument(
        "--headless",
        dest="viewport",
        const=viewport.Viewport.HEADLESS,
        action="store_const",
        help="Start the browser in headless if supported. "
        "Equivalent to --viewport=headless.")

    chrome_args = subparser.add_argument_group(
        "Browsers Options: Chrome/Chromium",
        "For convenience these arguments are directly are forwarded "
        "directly to chrome. ")
    chrome_args.add_argument("--js-flags", dest="js_flags")

    doc_str = "See chrome's base/feature_list.h source file for more details"
    chrome_args.add_argument(
        "--enable-features",
        help="Comma-separated list of enabled chrome features. " + doc_str,
        default="")
    chrome_args.add_argument(
        "--disable-features",
        help="Command-separated list of disabled chrome features. " + doc_str,
        default="")

    probe_group = subparser.add_argument_group("Probe Options", "")
    probe_config_group = probe_group.add_mutually_exclusive_group()
    probe_config_group.add_argument(
        "--probe",
        action="append",
        type=cli_config.SingleProbeConfig.parse,
        default=[],
        help="Enable general purpose probes to measure data on all cb.stories. "
        "This argument can be specified multiple times to add more probes. "
        "Use inline hjson (e.g. --probe=\"$NAME{$CONFIG}\") "
        "to configure probes. "
        "Individual probe configs can be specified in files as well: "
        "--probe='path/to/config.hjson'."
        "Use 'describe probes' or 'describe probe $NAME' for probe "
        "configuration details."
        "Cannot be used together with --probe-config."
        f"\n\nChoices: {', '.join(cli_config.PROBE_LOOKUP.keys())}")
    probe_config_group.add_argument(
        "--probe-config",
        type=cli_helper.parse_hjson_file_path,
        help="Browser configuration.json file. "
        "Use this config file to specify more complex Probe settings."
        "See config/probe.config.example.hjson on how to set up a complex "
        "configuration file. "
        "Cannot be used together with --probe.")
    subparser.set_defaults(
        subcommand_fn=self.benchmark_subcommand, benchmark_cls=benchmark_cls)
    self._add_verbosity_argument(subparser)
    subparser.add_argument("other_browser_args", nargs="*")

  def benchmark_subcommand(self, args: argparse.Namespace) -> None:
    benchmark = None
    runner = None
    self._benchmark_subcommand_helper(args)
    try:
      self._benchmark_subcommand_process_args(args)
      benchmark = self._get_benchmark(args)
      with tempfile.TemporaryDirectory(prefix="crossbench") as tmp_dirname:
        if args.dry_run:
          args.out_dir = pathlib.Path(tmp_dirname) / "results"
        args.browser = self._get_browsers(args)
        probes = self._get_probes(args)
        env_config = self._get_env_config(args)
        env_validation_mode = self._get_env_validation_mode(args)
        timing = self._get_timing(args)
        runner = self._get_runner(args, benchmark, env_config,
                                  env_validation_mode, timing)

        # We prevent running multiple stories in repetition OR if multiple
        # browsers are open when 'power' probes are used since it might distort
        # the data.
        if len(args.browser) > 1 or args.repetitions > 1:
          probe_names = [probe.name for probe in probes if probe.BATTERY_ONLY]
          if probe_names:
            names_str = ",".join(probe_names)
            raise argparse.ArgumentTypeError(
                f"Cannot use [{names_str}] probe(s) "
                "with repeat > 1 and/or with multiple browsers. We need to "
                "always start at the same battery level, and by running "
                "stories on multiple browsers or multiples time will create "
                "erroneous data.")

        for probe in probes:
          runner.attach_probe(probe, matching_browser_only=True)

        self._run_benchmark(args, runner)
    except KeyboardInterrupt:
      sys.exit(2)
    except cli_helper.LateArgumentError as e:
      if args.throw:
        raise
      self.handle_late_argument_error(e)
    except Exception as e:  # pylint: disable=broad-except
      if args.throw:
        raise
      self._log_benchmark_subcommand_failure(benchmark, runner, e)
      sys.exit(3)

  def _benchmark_subcommand_helper(self, args: argparse.Namespace) -> None:
    """Handle common subcommand mistakes that are not easily implementable
    with argparse.
    run: => just run the benchmark
    help => use --help
    describe => use describe benchmark NAME
    """
    if not args.other_browser_args:
      return
    maybe_command = args.other_browser_args[0]
    if maybe_command == "run":
      args.other_browser_args.pop()
      return
    if maybe_command == "help":
      self._subparsers[args.benchmark_cls].print_help()
      sys.exit(0)
    if maybe_command == "describe":
      logging.warning("See `describe benchmark %s` for more options",
                      args.benchmark_cls.NAME)
      # Patch args to simulate: describe benchmark BENCHMARK_NAME
      args.category = "benchmarks"
      args.filter = args.benchmark_cls.NAME
      args.json = False
      self.describe_subcommand(args)
      sys.exit(0)

  def _benchmark_subcommand_process_args(self, args) -> None:
    if args.config:
      if args.env_config:
        raise argparse.ArgumentTypeError(
            "--config cannot be used together with --env-config")
      if args.browser_config:
        raise argparse.ArgumentTypeError(
            "--config cannot be used together with --browser-config")
      if args.probe_config:
        raise argparse.ArgumentTypeError(
            "--config cannot be used together with --probe-config")
      # Propagate the global config file to all sub-configs
      args.env_config = cli_config.parse_env_config_file(args.config)
      args.browser_config = args.config
      args.probe_config = args.config

  def _log_benchmark_subcommand_failure(self, benchmark: Optional[Benchmark],
                                        runner: Optional[Runner],
                                        e: Exception) -> None:
    logging.debug(e)
    logging.error("")
    logging.error("#" * 80)
    logging.error("SUBCOMMAND UNSUCCESSFUL got %s:", e.__class__.__name__)
    logging.error("-" * 80)
    self._log_benchmark_subcommand_exception(e)
    logging.error("-" * 80)
    if benchmark:
      logging.error("Running '%s' was not successful:", benchmark.NAME)
    logging.error(
        "- Use --debug for very verbose output (equivalent to --throw -vvv)")
    if runner and runner.runs:
      self._log_runner_debug_hints(runner)
    else:
      logging.error("- Check %s.json detailed backtraces", ErrorsProbe.NAME)
    logging.error("#" * 80)
    sys.exit(3)

  def _log_benchmark_subcommand_exception(self, e: Exception) -> None:
    message = str(e)
    if message:
      logging.error(message)
      return
    if isinstance(e, AssertionError):
      self._log_assertion_error_statement(e)

  def _log_assertion_error_statement(self, e: AssertionError) -> None:
    _, exception, tb = sys.exc_info()
    if exception is not e:
      return
    tb_info = traceback.extract_tb(tb)
    filename, line, _, text = tb_info[-1]
    logging.info("%s:%s: %s", filename, line, text)

  def _log_runner_debug_hints(self, runner: Runner) -> None:
    failed_runs = [run for run in runner.runs if not run.is_success]
    if not failed_runs:
      return
    candidates: List[pathlib.Path] = [
        *runner.out_dir.glob(f"{ErrorsProbe.NAME}*"),
    ]
    for failed_run in failed_runs:
      candidates.extend(failed_run.out_dir.glob(f"{ErrorsProbe.NAME}*"))
      candidates.extend(failed_run.out_dir.glob("*.log"))

    failed_run = failed_runs[0]
    logging.error("- Check log outputs (1 of %d failed runs):",
                  len(failed_runs))
    limit = 3
    for log_file in candidates[:limit]:
      try:
        log_file = log_file.relative_to(pathlib.Path.cwd())
      except Exception as e:
        logging.debug("Could not create relative log_file: %s", e)
      logging.error("  - %s", log_file)
    if (pending := len(candidates) - limit) > 0:
      logging.error("  - ... and %d more interesting %s.json or *.log files",
                    pending, ErrorsProbe.NAME)

  def _run_benchmark(self, args: argparse.Namespace, runner: Runner) -> None:
    try:
      runner.run(is_dry_run=args.dry_run)
      logging.info("")
      self._log_results(args, runner, is_success=runner.is_success)
    except:  # pylint disable=broad-except
      self._log_results(args, runner, is_success=False)
      raise
    finally:
      if not args.out_dir:
        self._update_results_symlinks(runner)

  def _update_results_symlinks(self, runner: Runner) -> None:
    results_root = runner.out_dir.parent
    latest_link = results_root / "latest"
    if latest_link.is_symlink():
      latest_link.unlink()
    if not latest_link.exists():
      latest_link.symlink_to(
          runner.out_dir.relative_to(results_root), target_is_directory=True)
    else:
      logging.error("Could not create %s", latest_link)
    if not runner.runs:
      return
    out_dir = runner.out_dir
    first_run_dir = out_dir / "first_run"
    last_run_dir = out_dir / "last_run"
    runs: List[Run] = list(runner.runs)
    if first_run_dir.exists():
      logging.error("Cannot create first_run symlink: %s", first_run_dir)
    else:
      first_run_dir.symlink_to(runs[0].out_dir.relative_to(out_dir))
    if last_run_dir.exists():
      logging.error("Cannot create last_run symlink: %s", last_run_dir)
    else:
      last_run_dir.symlink_to(runs[-1].out_dir.relative_to(out_dir))
    runs_dir = out_dir / "runs"
    runs_dir.mkdir()
    for run in runs:
      if not run.out_dir.exists():
        continue
      relative = pathlib.Path("..") / run.out_dir.relative_to(out_dir)
      (runs_dir / str(run.index)).symlink_to(relative)

  def _log_results(self, args: argparse.Namespace, runner: Runner,
                   is_success: bool) -> None:
    logging.info("=" * 80)
    if is_success:
      logging.critical("RESULTS: %s", runner.out_dir)
    else:
      logging.critical("RESULTS (maybe incomplete/broken): %s", runner.out_dir)
    logging.info("=" * 80)
    if not runner.has_browser_group:
      logging.debug("No browser group in %s", runner)
      return
    browser_group = runner.browser_group
    for probe in runner.probes:
      try:
        probe.log_browsers_result(browser_group)
      except Exception as e:  # pylint disable=broad-except
        if args.throw:
          raise
        logging.debug("log_result_summary failed: %s", e)

  def _get_browsers(self, args: argparse.Namespace) -> Sequence[Browser]:
    args.browser_config = cli_config.BrowserVariantsConfig.from_cli_args(args)
    return args.browser_config.variants

  def _get_probes(self, args: argparse.Namespace) -> Sequence[Probe]:
    args.probe_config = cli_config.ProbeConfig.from_cli_args(args)
    return args.probe_config.probes

  def _get_benchmark(self, args: argparse.Namespace) -> Benchmark:
    benchmark_cls = self._get_benchmark_cls(args)
    assert (issubclass(benchmark_cls, Benchmark)), (
        f"benchmark_cls={benchmark_cls} is not subclass of Runner")
    return benchmark_cls.from_cli_args(args)

  def _get_benchmark_cls(self, args: argparse.Namespace) -> Type[Benchmark]:
    return args.benchmark_cls

  def _get_env_validation_mode(self,
                               args: argparse.Namespace) -> ValidationMode:
    return args.env_validation

  def _get_env_config(self, args: argparse.Namespace) -> HostEnvironmentConfig:
    if args.env:
      return args.env
    if args.env_config:
      return args.env_config
    return HostEnvironmentConfig()

  def _get_timing(self, args: argparse.Namespace) -> Timing:
    timeout_unit: dt.timedelta = args.timeout_unit or args.time_unit
    return Timing(args.cool_down_time, args.time_unit, timeout_unit,
                  args.run_timeout)

  def _get_runner(self, args: argparse.Namespace, benchmark: Benchmark,
                  env_config: HostEnvironmentConfig,
                  env_validation_mode: ValidationMode,
                  timing: Timing) -> Runner:
    runner_kwargs = self.RUNNER_CLS.kwargs_from_cli(args)
    return self.RUNNER_CLS(
        benchmark=benchmark,
        env_config=env_config,
        env_validation_mode=env_validation_mode,
        timing=timing,
        **runner_kwargs)

  def run(self, argv: Sequence[str]) -> None:
    self._init_logging(argv)
    unprocessed_argv: List[str] = []
    try:
      # Manually check for unprocessed_argv to print nicer error messages.
      self.args, unprocessed_argv = self.parser.parse_known_args(argv)
    except argparse.ArgumentError as e:
      # args is not set at this point, as parsing might have failed before
      # handling --throw or --debug.
      if "--throw" in argv or "--debug" in argv:
        raise e
      self.error(str(e))
    if unprocessed_argv:
      self.error(f"unrecognized arguments: {unprocessed_argv}\n"
                 f"Use `{self.parser.prog} {self.args.subcommand} --help` "
                 "for more details.")
    # Properly initialize logging after having parsed all args
    self._setup_logging()
    try:
      self.args.subcommand_fn(self.args)
    finally:
      self._teardown_logging()

  def handle_late_argument_error(self, e: cli_helper.LateArgumentError) -> None:
    self.error(f"error argument {e.flag}: {e.message}")

  def error(self, message: str) -> None:
    parser: cli_helper.CrossBenchArgumentParser = self.parser
    # Try to use the subparser to print nicer usage help on errors.
    # ArgumentParser tends to default to the toplevel parser instead of the
    # current subcommand, which in turn prints the wrong usage text.
    subcommand: str = getattr(self.args, "subcommand", "")
    if subcommand == "describe":
      parser = self.describe_parser
    else:
      maybe_benchmark_cls = getattr(self.args, "benchmark_cls", None)
      if maybe_benchmark_cls:
        parser = self._subparsers[maybe_benchmark_cls]
    if subcommand:
      parser.fail(f"{subcommand}: {message}")
    else:
      parser.fail(message)

  def _init_logging(self, argv: Sequence[str]) -> None:
    assert self._console_handler is None
    if not self._enable_logging:
      logging.getLogger().setLevel(logging.CRITICAL)
      return
    self._console_handler = logging.StreamHandler(sys.stderr)
    self._console_handler.addFilter(logging.Filter("root"))
    self._console_handler.setLevel(logging.INFO)
    logging.getLogger().setLevel(logging.INFO)
    logging.getLogger().addHandler(self._console_handler)

    # Manually extract values to allow logging for failing arguments.
    if "-v" in argv or "-vv" in argv or "-vvv" in argv:
      self._console_handler.setLevel(logging.DEBUG)
      logging.getLogger().setLevel(logging.DEBUG)

  def _setup_logging(self) -> None:
    if not self._enable_logging:
      return
    assert self._console_handler
    if self.args.verbosity == -1:
      self._console_handler.setLevel(logging.ERROR)
    elif self.args.verbosity == 0:
      self._console_handler.setLevel(logging.INFO)
    elif self.args.verbosity >= 1:
      self._console_handler.setLevel(logging.DEBUG)
      logging.getLogger().setLevel(logging.DEBUG)
    if self.args.color:
      self._console_handler.setFormatter(helper.ColoredLogFormatter())

  def _teardown_logging(self) -> None:
    if not self._enable_logging:
      assert self._console_handler is None
      return
    assert self._console_handler
    self._console_handler.flush()
    logging.getLogger().removeHandler(self._console_handler)
    self._console_handler = None
