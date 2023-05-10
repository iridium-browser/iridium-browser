# liblouis Braille Translation and Back-Translation Library
#
# Copyright (C) 2017 Bert Frees
#
# This file is part of liblouis.
#
# liblouis is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 2.1 of the License, or
# (at your option) any later version.
#
# liblouis is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with liblouis. If not, see <http://www.gnu.org/licenses/>.
#

import argparse
import fileinput
import re
from utils import *

def main():
    parser = argparse.ArgumentParser(description="Add contraction rules to contraction table")
    parser.add_argument('FILE', default=['-'],
                        help="file with unprocessed contraction rules")
    parser.add_argument('CONTRACTION_TABLE', help="contraction table")
    parser.add_argument('-t', '--table', required=True,
                        help="translation table for converting to dot patterns", dest="TABLE")
    args = parser.parse_args()
    load_table(args.TABLE)
    p = re.compile(r"^#.*|[ \t]*([+-])?(nocross[ \t]+)?(always|word|begword|endword|midword|begmidword|midendword|prfword|sufword)[ \t]+([^ \t]+)[ \t]+([^ \t\n]+)([ \t]+(.*))?\n?$")
    rules = []
    for line in fileinput.FileInput(args.CONTRACTION_TABLE, openhook=fileinput.hook_encoded("utf-8")):
        m = p.match(line)
        exit_if_not(m and not m.group(1))
        _, nocross, opcode, text, braille, _, comment = m.groups()
        nocross = nocross or ""
        if opcode:
            rule = {"opcode": opcode, "nocross": nocross, "text": text, "braille": braille, "comment": comment}
            rules.append(rule)
    for line in fileinput.FileInput(args.FILE, openhook=fileinput.hook_encoded("utf-8")):
        m = p.match(line)
        if not m:
            printerrln("%s: rule is not valid" % (line,))
            exit(1)
        exit_if_not(m)
        add_or_delete, nocross, opcode, text, braille, _, _ = m.groups()
        nocross = nocross or ""
        if opcode:
            comment = braille
            if comment.endswith('\\'):
                comment = comment + " "
            braille = to_dot_pattern(braille)
            rules, rule = partition(lambda rule: rule["opcode"] == opcode and
                                                 rule["text"] == text and
                                                 rule["braille"] == braille,
                                    rules)
            rule = list(rule)
            if add_or_delete == '-':
                if not rule:
                    printerrln("%s%s %s %s: rule can not be deleted because not in %s" % (nocross,opcode,text,braille,args.CONTRACTION_TABLE))
                    exit(1)
            else:
                if rule:
                    printerrln("%s%s %s %s: rule can not be added because already in %s" % (nocross,opcode,text,braille,args.CONTRACTION_TABLE))
                    exit(1)
                rule = {"opcode": opcode, "nocross": nocross, "text": text, "braille": braille, "comment": comment}
                rules.append(rule)
    opcode_order = {"word": 1, "always": 2, "begword": 3, "endword": 4, "midword": 5, "begmidword": 6, "midendword": 7, "prfword": 8, "sufword": 9}
    for rule in sorted(rules, key=lambda rule: (rule["text"], rule["nocross"], opcode_order[rule["opcode"]])):
        println(u"{nocross:<9}{opcode:<10} {text:<10} {braille:<30} {comment}".format(**rule))

if __name__ == "__main__": main()
