#!/usr/bin/env python3
# Copyright 2023 The Khronos Group Inc.
# Copyright 2023 Valve Corporation
# Copyright 2023 LunarG, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys
from xml.etree import ElementTree

def RunGenerators(api: str, registry: str, targetFilter: str) -> None:

    # These live in the Vulkan-Docs repo, but are pulled in via the
    # Vulkan-Headers/registry folder
    # At runtime we inject python path to find these helper scripts
    scripts = os.path.dirname(registry)
    scripts_directory_path = os.path.dirname(os.path.abspath(__file__))
    registry_headers_path = os.path.join(scripts_directory_path, scripts)
    sys.path.insert(0, registry_headers_path)
    from reg import Registry

    from generators.base_generator import BaseGeneratorOptions
    from generators.dispatch_table_generator import DispatchTableOutputGenerator
    from generators.enum_string_helper_generator import EnumStringHelperOutputGenerator

    # Build up a list of all generators and custom options
    generators = {
        'vul_dispatch_table.h' : {
           'generator' : DispatchTableOutputGenerator,
           'directory' : 'include/vulkan/utility',
        },
        'vk_enum_string_helper.h' : {
            'generator' : EnumStringHelperOutputGenerator,
            'directory' : 'include/vulkan',
        },
    }

    if (targetFilter and targetFilter not in generators.keys()):
        print(f'ERROR: No generator options for unknown target: {targetFilter}', file=sys.stderr)
        sys.exit(1)

    # Filter if --target is passed in
    targets = [x for x in generators.keys() if not targetFilter or x == targetFilter]

    for index, target in enumerate(targets, start=1):
        print(f'[{index}|{len(targets)}] Generating {target}')

        # First grab a class contructor object and create an instance
        generator = generators[target]['generator']
        gen = generator()

        outDirectory = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', generators[target]['directory']))
        options = BaseGeneratorOptions(
            customFileName  = target,
            customDirectory = outDirectory,
            customApiName   = 'vulkan')

        # Create the registry object with the specified generator and generator
        # options. The options are set before XML loading as they may affect it.
        reg = Registry(gen, options)

        # Parse the specified registry XML into an ElementTree object
        tree = ElementTree.parse(registry)

        # Filter out non-Vulkan extensions
        if api == 'vulkan':
            [exts.remove(e) for exts in tree.findall('extensions') for e in exts.findall('extension') if (sup := e.get('supported')) is not None and options.apiname not in sup.split(',')]

        # Load the XML tree into the registry object
        reg.loadElementTree(tree)

        # Finally, use the output generator to create the requested target
        reg.apiGen()


def main(argv):
    parser = argparse.ArgumentParser(description='Generate source code for this repository')
    parser.add_argument('--api',
                        default='vulkan',
                        choices=['vulkan'],
                        help='Specify API name to generate')
    parser.add_argument('registry', metavar='REGISTRY_PATH', help='path to the Vulkan-Headers registry directory')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--target', help='only generate file name passed in')
    args = parser.parse_args(argv)

    registry = os.path.abspath(os.path.join(args.registry,  'vk.xml'))
    RunGenerators(args.api, registry, args.target)

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

