#!/usr/bin/env python3
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A simple wrapper for cast_core_proto_generator.

Script that invokes cast_core_proto_generator to generate C++ <ServiceName>Stub
and <ServiceName>Handler classes as part of
//third_party/cast_core/build/chromium/proto.gni in Chromium repository.
"""

from __future__ import print_function
import argparse
import os.path
import subprocess
import sys


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "--generator",
      required=True,
      help="Relative path to Cast Core gRPC generator.")
  parser.add_argument(
      "--proto", required=True, help="Path to the gRPC proto file.")
  parser.add_argument(
      "--source-root", required=True, help="Source root directory.")
  parser.add_argument("--out-dir", required=True, help="Output directory.")
  parser.add_argument(
      "--proto-in-dir", required=True, help="Proto-in-dir directory.")
  parser.add_argument(
      "--includes",
      required=True,
      help="Comma-separated list of includes to add to generated output.")
  options = parser.parse_args(argv)

  if not options.proto_in_dir.startswith("//"):
    raise RuntimeError(
        "Proto-in-dir must be a root path starting with //: {0}".format(
            options.proto_in_dir))
  proto_in_dir = options.proto_in_dir[2:]

  # Get proto path relative to its proto-in-dir.
  proto = os.path.relpath(options.proto, options.source_root)
  proto = os.path.relpath(proto, proto_in_dir)

  # Get full path to the source directory with proto-in-dir.
  source_dir = os.path.join(os.path.realpath(options.source_root), proto_in_dir)

  # Get the full path to the output directory with proto-in-dir.
  out_dir = os.path.join(os.path.realpath(options.out_dir), proto_in_dir)

  generator_cmd = [options.generator]
  generator_cmd += ["--proto", proto]
  generator_cmd += ["--source-dir", source_dir]
  generator_cmd += ["--out-dir", out_dir]
  for option in options.includes.split(","):
    generator_cmd += ["--include", option]

  ret = subprocess.call(generator_cmd)
  if ret != 0:
    if ret <= -100:
      # Windows error codes such as 0xC0000005 and 0xC0000409 are much easier to
      # recognize and differentiate in hex. In order to print them as unsigned
      # hex we need to add 4 Gig to them.
      error_number = "0x%08X" % (ret + (1 << 32))
    else:
      error_number = "%d" % ret
    raise RuntimeError("Protoc has returned non-zero status: "
                       "{0}".format(error_number))


if __name__ == "__main__":
  try:
    main(sys.argv[1:])
  except RuntimeError as e:
    print(e, file=sys.stderr)
    sys.exit(1)
