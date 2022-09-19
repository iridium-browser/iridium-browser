#!/usr/bin/env python3

# Copyright (c) 2021, Arm Limited
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import tabulate as tbl
import argparse
import csv
import doctest


def quote_literal(val):
    if len(val) > 0:
        return f"``{val}``"
    return val


rst_header_levels = ['=', '~', '-', '_', '@']
__CSV_SECTION_PREFIX = '<SECTION>'
__CSV_COMMENT_PREFIX = '<COMMENT>'
__CSV_HEADER_PREFIX = '<HEADER>'
__intrinsic_table_header = ['Intrinsic', 'Argument preparation',
                            'AArch64 Instruction', 'Result', 'Supported architectures']

__INTRINSIC_TABLE_KEYWORD = '__intrinsic_table'
__SECTION_TEXT_KEYWORD = '__section_text'

__SECTION_KEYWORDS = [__INTRINSIC_TABLE_KEYWORD, __SECTION_TEXT_KEYWORD]


def rst_literal_quote(mapping):
    r"""
    >>> rst_literal_quote('a; b')
    ' ::\n\n    a \n    b \n\n'

    >>> rst_literal_quote('a b')
    ' ::\n\n    a b \n\n'

    >>> rst_literal_quote('')
    ''
    """
    if mapping == "":
        return ""

    lines = mapping.split(';')
    indented_lines = [f"    {line.strip()} " for line in lines]
    lines = [" ::", ""] + indented_lines + ["\n"]
    return '\n'.join(lines)


def quote_split_intrinsics(intrinsic):
    r"""
    >>> quote_split_intrinsics('int f(int x, float y)')
    '.. code:: c\n\n    int f(\n        int x,\n        float y)'

    >>> quote_split_intrinsics('int f(int x)')
    '.. code:: c\n\n    int f(int x)\n'
    """
    # Remove the suffix ')' from the intrinsic string.
    intrinsic_without_ending = intrinsic[:-1]
    ret_def, par, signature = intrinsic_without_ending.partition('(')
    split_signature = signature.split(',')
    if len(split_signature) > 1:
        return f".. code:: c\n\n    {ret_def}(\n        " + ',\n       '.join(split_signature) + ")"
    else:
        return f".. code:: c\n\n    {intrinsic}\n"


def get_intrinsic_name(signature):
    """
    Get the intrinsic name from the signature of the intrinsic.

    >>> get_intrinsic_name("int8x8_t vadd_s8(int8x8_t a, int8x8_t b)")
    'vadd_s8'

    >>> get_intrinsic_name("int32x4_t vaddl_high_s16(int16x8_t a, int16x8_t b)")
    'vaddl_high_s16'

    >>> get_intrinsic_name("float64x2_t vfmsq_lane_f64(float64x2_t a, float64x2_t b, float64x1_t v, __builtin_constant_p(lane))")
    'vfmsq_lane_f64'

    >>> get_intrinsic_name("poly16x8_t vsriq_n_p16(poly16x8_t a, poly16x8_t b, __builtin_constant_p(n))")
    'vsriq_n_p16'

    >>> get_intrinsic_name("uint8x16_t [__arm_]vddupq_m[_n_u8](uint8x16_t inactive, uint32_t a, const int imm, mve_pred16_t p)")
    '[__arm_]vddupq_m[_n_u8]'
    """
    tmp = signature.split(' ')

    tmp = tmp[1].split('(')
    return tmp[0]


def clear_builtin_constant(s):
    """
    >>> clear_builtin_constant("__builtin_constant_p(lane)")
    'const int lane'
    >>> clear_builtin_constant("__builtin_constant_p(n)")
    'const int n'
    >>> clear_builtin_constant("__builtin_constant_p(lane1)")
    'const int lane1'
    >>> clear_builtin_constant("__builtin_constant_p(lane2)")
    'const int lane2'
    >>> clear_builtin_constant("__builtin_constant_p(x), __builtin_constant_p(y)")
    'const int x, const int y'
    """
    import re
    return re.sub(r'__builtin_constant_p\(([a-zA-Z0-9]+)\)', r'const int \1', s)


class Intrinsic:

    def __init__(self, signature, parameter_mapping, asm, result_mapping, arch, classification):
        self.signature = clear_builtin_constant(signature.strip())
        self.parameter_mapping = parameter_mapping
        self.asm = asm.strip()
        self.result_mapping = result_mapping
        self.arch = arch.strip()
        self.asm_mnemonic = self.asm.split(' ')[0].strip()
        self.classification = classification

    def table_row(self):
        return [quote_split_intrinsics(self.signature), rst_literal_quote(self.parameter_mapping), rst_literal_quote(self.asm), rst_literal_quote(self.result_mapping), quote_literal(self.arch)]


def recurse_set(parent, section_levels, value, object_type_target):
    """Recursively fills the dictinoary `parent` with the keys provided in
    `section_levels`. The innermost key is mapped to a list object to which
    the value of `value` is appened.

    For example, given parent={}, section_levels=['a','b','c'] and value =
    'text', it creates the entry
    `parent['a']['b']['c']=['text']`. Another invocation with
    value='text2' updates the list as
    `parent['a']['b']['c']=['text', 'text2']`

    >>> import json
    >>> jprint = lambda x: print(json.dumps(x, indent = 2))

    >>> parent = {}
    >>> intrinsic_sample = lambda x : [f'intr{x}', f'arg_prep{x}', f'inst{x}', f'res{x}', f'sup_arch{x}']
    >>> recurse_set(parent, ['Section 1'], intrinsic_sample(0), '__intrinsic_table')
    >>> jprint(parent)
    {
      "Section 1": {
        "__intrinsic_table": [
          [
            "intr0",
            "arg_prep0",
            "inst0",
            "res0",
            "sup_arch0"
          ]
        ]
      }
    }
    >>> recurse_set(parent, ['Section 1'], intrinsic_sample(1), '__intrinsic_table')
    >>> jprint(parent)
    {
      "Section 1": {
        "__intrinsic_table": [
          [
            "intr0",
            "arg_prep0",
            "inst0",
            "res0",
            "sup_arch0"
          ],
          [
            "intr1",
            "arg_prep1",
            "inst1",
            "res1",
            "sup_arch1"
          ]
        ]
      }
    }

    >>> recurse_set(parent, ['Section 2', 'Section 2.1'], intrinsic_sample(3), '__intrinsic_table')
    >>> recurse_set(parent, ['Section 2', 'Section 2.1'], intrinsic_sample(4), '__intrinsic_table')
    >>> recurse_set(parent, ['Section 2', 'Section 2.1'], intrinsic_sample(5), '__intrinsic_table')
    >>> jprint(parent)
    {
      "Section 1": {
        "__intrinsic_table": [
          [
            "intr0",
            "arg_prep0",
            "inst0",
            "res0",
            "sup_arch0"
          ],
          [
            "intr1",
            "arg_prep1",
            "inst1",
            "res1",
            "sup_arch1"
          ]
        ]
      },
      "Section 2": {
        "Section 2.1": {
          "__intrinsic_table": [
            [
              "intr3",
              "arg_prep3",
              "inst3",
              "res3",
              "sup_arch3"
            ],
            [
              "intr4",
              "arg_prep4",
              "inst4",
              "res4",
              "sup_arch4"
            ],
            [
              "intr5",
              "arg_prep5",
              "inst5",
              "res5",
              "sup_arch5"
            ]
          ]
        }
      }
    }

    >>> recurse_set(parent, ['Section 2', 'Section 2.2'], intrinsic_sample(6), '__intrinsic_table')
    >>> jprint(parent)
    {
      "Section 1": {
        "__intrinsic_table": [
          [
            "intr0",
            "arg_prep0",
            "inst0",
            "res0",
            "sup_arch0"
          ],
          [
            "intr1",
            "arg_prep1",
            "inst1",
            "res1",
            "sup_arch1"
          ]
        ]
      },
      "Section 2": {
        "Section 2.1": {
          "__intrinsic_table": [
            [
              "intr3",
              "arg_prep3",
              "inst3",
              "res3",
              "sup_arch3"
            ],
            [
              "intr4",
              "arg_prep4",
              "inst4",
              "res4",
              "sup_arch4"
            ],
            [
              "intr5",
              "arg_prep5",
              "inst5",
              "res5",
              "sup_arch5"
            ]
          ]
        },
        "Section 2.2": {
          "__intrinsic_table": [
            [
              "intr6",
              "arg_prep6",
              "inst6",
              "res6",
              "sup_arch6"
            ]
          ]
        }
      }
    }

    >>> recurse_set(parent, ['Section 1', 'Section 1.1'], intrinsic_sample(7), '__intrinsic_table')
    >>> jprint(parent)
    {
      "Section 1": {
        "__intrinsic_table": [
          [
            "intr0",
            "arg_prep0",
            "inst0",
            "res0",
            "sup_arch0"
          ],
          [
            "intr1",
            "arg_prep1",
            "inst1",
            "res1",
            "sup_arch1"
          ]
        ],
        "Section 1.1": {
          "__intrinsic_table": [
            [
              "intr7",
              "arg_prep7",
              "inst7",
              "res7",
              "sup_arch7"
            ]
          ]
        }
      },
      "Section 2": {
        "Section 2.1": {
          "__intrinsic_table": [
            [
              "intr3",
              "arg_prep3",
              "inst3",
              "res3",
              "sup_arch3"
            ],
            [
              "intr4",
              "arg_prep4",
              "inst4",
              "res4",
              "sup_arch4"
            ],
            [
              "intr5",
              "arg_prep5",
              "inst5",
              "res5",
              "sup_arch5"
            ]
          ]
        },
        "Section 2.2": {
          "__intrinsic_table": [
            [
              "intr6",
              "arg_prep6",
              "inst6",
              "res6",
              "sup_arch6"
            ]
          ]
        }
      }
    }

    >>> recurse_set(parent, ['Section X', 'Section X.X'], 'somethign', '__unhandled_keyword')
    Traceback (most recent call last):
       ...
    Exception: Target is unsupported.

    >>> new = {}
    >>> recurse_set(new, ['Section X'], intrinsic_sample('001'), '__intrinsic_table')
    >>> recurse_set(new, ['Section X'], 'Description of Section X', '__section_text')
    >>> recurse_set(new, ['Section X', 'Section X.X'], 'somethign', '__section_text')
    >>> recurse_set(new, ['Section X', 'Section X.X'], 'Description of Section X.X', '__section_text')
    >>> jprint(new)
    {
      "Section X": {
        "__intrinsic_table": [
          [
            "intr001",
            "arg_prep001",
            "inst001",
            "res001",
            "sup_arch001"
          ]
        ],
        "__section_text": "Description of Section X",
        "Section X.X": {
          "__section_text": "Description of Section X.X"
        }
      }
    }
    """
    if object_type_target not in __SECTION_KEYWORDS:
        raise Exception("Target is unsupported.")
    current_section, *rest = section_levels

    # No more headers to traverse: insert the value under the
    # `object_type_target`.
    if not rest:
        if object_type_target == __INTRINSIC_TABLE_KEYWORD:
            parent[current_section] = parent.get(
                current_section, {__INTRINSIC_TABLE_KEYWORD: []})
            parent[current_section][__INTRINSIC_TABLE_KEYWORD].append(value)
            return

        if object_type_target == __SECTION_TEXT_KEYWORD:
            parent[current_section] = parent.get(current_section, {})
            parent[current_section][__SECTION_TEXT_KEYWORD] = value
            return

    # If we are still traversing the section headers, we are dealing
    # with a new section.
    parent.setdefault(current_section, {})
    recurse_set(parent[current_section], rest, value, object_type_target)


def start_new_section(item):
    """We start a new section when `key` is not handled in
    `__SECTION_KEYWORDS` and `value` is a `dict` object.
    """
    key, value = item
    return isinstance(value, dict) and key not in __SECTION_KEYWORDS


def is_intrinsic_table(item):
    """
    Determines whether a key/value pair is a table of intrinsics.

    >>> is_intrinsic_table(('__intrinsic_table', [[1,2,3,4,5],['a', 'b','c', 'd', 'f']]))
    True

    >>> is_intrinsic_table(('__intrinsic_table', [[1,2,3,4,5],['a', 'b','c', 'd']]))
    False

    >>> is_intrinsic_table(('__intrinsic_tabl', [[1,2,3,4,5],['a', 'b','c', 'd', 'f']]))
    False

    """
    key, value = item
    return key == __INTRINSIC_TABLE_KEYWORD and isinstance(value, list) and all(len(row) == 5 and isinstance(row, list) for row in value)


def is_section_text(item):
    """
    Determines whether a key/value pair is a table of intrinsics.

    >>> is_section_text(('__intrinsic_table', [[1,2,3,4,5],['a', 'b','c', 'd', 'f']]))
    False

    >>> is_section_text(('__section_text', ""))
    True


    """
    key, value = item
    return key == __SECTION_TEXT_KEYWORD


def recurse_print_to_rst(item, section_level_list, headers=__intrinsic_table_header, tablefmt="rst"):
    """
    >>> table_item = ('__intrinsic_table', [[1,2,3,4,5], [6,7,8,9, 10]])
    >>> print(recurse_print_to_rst(table_item, ['=']))
    <BLANKLINE>
    ===========  ======================  =====================  ========  =========================
      Intrinsic    Argument preparation    AArch64 Instruction    Result    Supported architectures
    ===========  ======================  =====================  ========  =========================
              1                       2                      3         4                          5
              6                       7                      8         9                         10
    ===========  ======================  =====================  ========  =========================

    >>> item = ('New section', {'__intrinsic_table': [[1,2,3,4,5], [6,7,8,9, 10]]})
    >>> print(recurse_print_to_rst(item, ['=']))
    <BLANKLINE>
    New section
    ===========
    <BLANKLINE>
    ===========  ======================  =====================  ========  =========================
      Intrinsic    Argument preparation    AArch64 Instruction    Result    Supported architectures
    ===========  ======================  =====================  ========  =========================
              1                       2                      3         4                          5
              6                       7                      8         9                         10
    ===========  ======================  =====================  ========  =========================

    >>> item = ('Section 1', {'Section 1.1': {'__intrinsic_table': [[1,2,3,4,5], [6,7,8,9, 10]]}})
    >>> print(recurse_print_to_rst(item, ['=','~']))
    <BLANKLINE>
    Section 1
    =========
    <BLANKLINE>
    Section 1.1
    ~~~~~~~~~~~
    <BLANKLINE>
    ===========  ======================  =====================  ========  =========================
      Intrinsic    Argument preparation    AArch64 Instruction    Result    Supported architectures
    ===========  ======================  =====================  ========  =========================
              1                       2                      3         4                          5
              6                       7                      8         9                         10
    ===========  ======================  =====================  ========  =========================

    >>> item = ('Section 1', {'Section 1.1': { '__section_text': "Text for Section 1.1", '__intrinsic_table': [[1,2,3,4,5]]}})
    >>> print(recurse_print_to_rst(item, ['=','~']))
    <BLANKLINE>
    Section 1
    =========
    <BLANKLINE>
    Section 1.1
    ~~~~~~~~~~~
    <BLANKLINE>
    Text for Section 1.1
    <BLANKLINE>
    ===========  ======================  =====================  ========  =========================
      Intrinsic    Argument preparation    AArch64 Instruction    Result    Supported architectures
    ===========  ======================  =====================  ========  =========================
              1                       2                      3         4                          5
    ===========  ======================  =====================  ========  =========================

    >>> item = ('Section 1', {'Section 1.1': { '__intrinsic_table': [[1,2,3,4,5]], '__section_text': "Text for Section 1.1" }})
    >>> print(recurse_print_to_rst(item, ['=','~']))
    <BLANKLINE>
    Section 1
    =========
    <BLANKLINE>
    Section 1.1
    ~~~~~~~~~~~
    <BLANKLINE>
    Text for Section 1.1
    <BLANKLINE>
    ===========  ======================  =====================  ========  =========================
      Intrinsic    Argument preparation    AArch64 Instruction    Result    Supported architectures
    ===========  ======================  =====================  ========  =========================
              1                       2                      3         4                          5
    ===========  ======================  =====================  ========  =========================
    """
    key, value = item
    if is_intrinsic_table(item):
        return "\n" + str(tbl.tabulate(value, headers=headers, tablefmt=tablefmt))

    body = ""
    if start_new_section(item):
        title, *rest = section_level_list
        body += f"\n{key}"
        body += "\n" + title * len(key)+""
        # Print the value of the text for the section right after the
        # section title.
        if __SECTION_TEXT_KEYWORD in value:
            body += "\n\n" + value[__SECTION_TEXT_KEYWORD]
    for k, v in value.items():
        # Skip the section text because it has already been processed right
        # after the section title.
        if k == __SECTION_TEXT_KEYWORD:
            continue
        body += "\n"+recurse_print_to_rst((k, v), rest, headers, tablefmt)
    return body


def is_section_command(row):
    """CSV rows are cosidered new section commands if they start with
    <SECTION> and consist of at least two columns column.

    >>> is_section_command('<SECTION>\tSection name'.split('\t'))
    True

    >>> is_section_command('<other>\tSection name'.split('\t'))
    False

    >>> is_section_command(['<SECTION>', 'Section name', 'some more'])
    True

    """
    return len(row) >= 2 and row[0] == __CSV_SECTION_PREFIX


def is_comment_line(row):
    """CSV rows are cosidered new section commands if they start with
    <COMMENT>

    >>> is_comment_line('<COMMENT>\tSome text'.split('\t'))
    True

    >>> is_comment_line('<other>\tSome text'.split('\t'))
    False

    >>> is_comment_line('Some text'.split('\t'))
    False

    >>> is_comment_line('<COMMENT>\tSome text\tsome more'.split('\t'))
    True
    """
    return len(row) >= 2 and row[0] == __CSV_COMMENT_PREFIX


def is_table_header(row):
    """Table headers are 6-columns with the first one being
    __CSV_HEADER_PREFIX.

    >>> is_table_header(['<HEADER>', '', '', '', '', ''])
    True

    >>> is_table_header(['<EADER>', '', '', '', '', ''])
    False
    """
    return len(row) == 6 and row[0] == __CSV_HEADER_PREFIX


def is_intrinsic_definition(row):
    return len(row) == 5


def get_section_data(row):
    section_text = row[2] if len(row) == 3 else None
    return [row[1], section_text]


def process_db(db, classification_db):
    """Processes a list of intrinsics and their mappings to the
    classification into a sequence of sections and RST tables.

    >>> intrinsics = [
    ... ['<HEADER>', 'T1', 'T2', 'T3', 'T4', 'T5'],
    ... ['<SECTION>','Section 1 title', 'Section 1 description.'],
    ... ['a A01()','a','aa','aaa','aaaa'],
    ... ['b B01()','b','bb','bbb','bbbb'],
    ... ['<SECTION>','Section 2 title', 'Section 2 description.'],
    ... ['c C01()','c','cc','ccc','cccc'],
    ... ]
    >>> classification = {
    ... 'B01': 'Section 1.1|Section 1.1.1',
    ... 'C01': 'classX|subclassY'
    ... }
    >>> print(process_db(intrinsics, classification))
    <BLANKLINE>
    <BLANKLINE>
    Section 1 title
    ===============
    <BLANKLINE>
    Section 1 description.
    <BLANKLINE>
    No category
    ~~~~~~~~~~~
    <BLANKLINE>
    +-------------+-------+--------+---------+----------+
    | T1          | T2    | T3     | T4      | T5       |
    +=============+=======+========+=========+==========+
    | .. code:: c | ::    | ::     | ::      | ``aaaa`` |
    |             |       |        |         |          |
    |     a A01() |     a |     aa |     aaa |          |
    +-------------+-------+--------+---------+----------+
    <BLANKLINE>
    Section 1.1
    ~~~~~~~~~~~
    <BLANKLINE>
    Section 1.1.1
    -------------
    <BLANKLINE>
    +-------------+-------+--------+---------+----------+
    | T1          | T2    | T3     | T4      | T5       |
    +=============+=======+========+=========+==========+
    | .. code:: c | ::    | ::     | ::      | ``bbbb`` |
    |             |       |        |         |          |
    |     b B01() |     b |     bb |     bbb |          |
    +-------------+-------+--------+---------+----------+
    <BLANKLINE>
    Section 2 title
    ===============
    <BLANKLINE>
    Section 2 description.
    <BLANKLINE>
    classX
    ~~~~~~
    <BLANKLINE>
    subclassY
    ---------
    <BLANKLINE>
    +-------------+-------+--------+---------+----------+
    | T1          | T2    | T3     | T4      | T5       |
    +=============+=======+========+=========+==========+
    | .. code:: c | ::    | ::     | ::      | ``cccc`` |
    |             |       |        |         |          |
    |     c C01() |     c |     cc |     ccc |          |
    +-------------+-------+--------+---------+----------+
    """
    filtered = {}
    section, section_text, table_header = None, None, None
    line_num = 0
    for row in db:
        line_num += 1
        # Set section if the line in the file is starting with __CSV_SECTION_PREFIX.
        if is_section_command(row):
            section, section_text = get_section_data(row)
            if section_text:
                recurse_set(filtered, [section],
                            section_text, __SECTION_TEXT_KEYWORD)

            print(
                f"Activating {__CSV_SECTION_PREFIX} command at line {line_num}, '{section}'", file=sys.stderr)
            continue

        # Skip comment lines.
        if is_comment_line(row):
            continue

        if is_table_header(row):
            # Only one <HEADER> command is allowed.
            assert(table_header is None)
            # Set the header.
            table_header = row[1:]
            assert(len(table_header) == 5)
            continue

        if is_intrinsic_definition(row):
            classification = classification_db.get(
                get_intrinsic_name(row[0]), "No category")
            intrinsic = Intrinsic(
                row[0], row[1], row[2], row[3], row[4], classification)
            classification_list = intrinsic.classification.split('|')
            if section:
                classification_list = [section]+classification_list
            recurse_set(filtered, classification_list,
                        intrinsic.table_row(), __INTRINSIC_TABLE_KEYWORD)

            continue

        print(f"Skipping line {line_num}: row = {row}", file=sys.stderr)
    # Make sure that the CSV has at a row that have set the tale
    # header.
    assert(table_header is not None)
    body = ""
    for k, v in filtered.items():
        body += "\n" + \
            recurse_print_to_rst((k, v), rst_header_levels,
                                 headers=table_header, tablefmt="grid")
    return body


def get_classification_map(classification_file):
    classification_map = {}
    with open(classification_file) as csvclassificationfile:
        classification_db = csv.reader(csvclassificationfile, delimiter='\t')
        classification_map = {}
        for row in classification_db:
            if len(row) == 2:
                classification_map[row[0]] = row[1]
                continue

            # Skip comment lines.
            if is_comment_line(row):
                continue

            print(
                f"{classification_file}: skipping line {classification_db.line_num}: row = {row}", file=sys.stderr)

    return classification_map


def read_template(path):
    with open(path) as f:
        return f.read()


def get_intrinsics_db(path):
    with open(path) as csvfile:
        return list(csv.reader(csvfile, delimiter='\t'))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate an RST file for the intrinsics specifications.")
    parser.add_argument("--intrinsic-defs", metavar="<path>", type=str,
                        help="CSV file with the database of the intrinsics.", required=True)
    parser.add_argument("--template", metavar="<path>", type=str,
                        help="Template file for generating the RST of the specificaion.", required=True)
    parser.add_argument("--classification", metavar="<path>", type=str,
                        help="CSV file that map the intrinsics to their classification.", required=True)
    parser.add_argument("--outfile", metavar="<path>", type=str,
                        help="Output file where the RST of the specs is written.", required=True)
    cli_args = parser.parse_args()


    # We require version 0.8.6 to be able to print multi-line records
    # in tables.
    if tbl.__version__ < "0.8.6":
        print(f"Your version of package tabulate is too old ({tbl.__version__}). "
              "Update it to be greater or equal to 0.8.6.", file=sys.stderr)
        exit(1)

    classification_map = get_classification_map(cli_args.classification)
    intrinsics_db = get_intrinsics_db(cli_args.intrinsic_defs)
    doc_template = read_template(cli_args.template)
    intrinsic_table = process_db(intrinsics_db, classification_map)
    rst_output = doc_template.format(intrinsic_table=intrinsic_table)
    with (open(cli_args.outfile, 'w')) as f:
        f.write(rst_output)
    # Always run the unit tests.
    doctest.NORMALIZE_WHITESPACE
    doctest.testmod()
