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
from utils import *

def update_counters(total, init=False):
    if not init:
        sys.stderr.write("\033[1A\033[K")
        sys.stderr.write("\033[1A\033[K")
    sys.stderr.write(("%d words processed\n" % total))
    sys.stderr.flush()

def main():
    parser = argparse.ArgumentParser(description="Suggest dictionary rows and liblouis rules")
    parser.add_argument('-d', '--dictionary', required=True, dest="DICTIONARY",
                        help="dictionary file")
    parser.add_argument('-t', '--table', required=True, dest="TABLE",
                        help="translation table including hyphenation patterns")
    parser.add_argument('-n', type=int, default=10, dest="MAX_PROBLEMS",
                        help="maximal number of suggestions")
    parser.add_argument('--non-interactive', type=bool, default=False, dest="NON_INTERACTIVE",
                        help="commit all suggestions without providing details")
    parser.add_argument('--print-total-rate', type=bool, default=False, dest="TOTAL_RATE",
                        help="print the total number of correctly translated words (takes longer)")
    args = parser.parse_args()
    println("# -*- tab-width: 30; -*-")
    conn, c = open_dictionary(args.DICTIONARY)
    c.execute("SELECT text,braille FROM dictionary WHERE braille IS NOT NULL ORDER BY frequency DESC")
    load_table(args.TABLE)
    problems = 0
    correct = 0
    total = 0
    update_counters(total, init=True)
    for text, braille in c.fetchall():
        problem = False
        if braille:
            actual_braille, applied_rules = translate(text)
            if actual_braille != braille:
                problem = True
                if problems < args.MAX_PROBLEMS:
                    comments = []
                    comments.append("wrong braille\t\t" + actual_braille)
                    suggest_rows = []
                    suggest_rules = []
                    non_letters = False
                    if not is_letter(text):
                        non_letters = True
                        comments.append("word has non-letters")
                    comments.append("applied rules")
                    applied_rules = [get_rule(x) for x in applied_rules if x is not None]
                    for rule in applied_rules:
                        comments.append("> " + "\t".join(rule))
                    other_relevant_rules = set(find_relevant_rules(text)) - set(applied_rules)
                    if other_relevant_rules:
                        comments.append("other relevant rules")
                        for rule in other_relevant_rules:
                            comments.append("> " + "\t".join(rule))
                    suggest_rules.append({"opcode": "word", "text": text, "braille": braille})
            if problem and problems < args.MAX_PROBLEMS:
                if args.NON_INTERACTIVE and suggest_rules:
                    for rule in suggest_rules:
                        println("%(opcode)s\t%(text)s\t%(braille)s" % rule)
                else:
                    println("# >>>\t%s\t%s" % (text, braille or ""))
                    for comment in comments:
                        println("# | " + comment)
                    println("# |__")
                    for row in suggest_rows:
                        println("#\t%(text)s\t%(braille)s" % row)
                    for rule in suggest_rules:
                        println("# %(opcode)s\t%(text)s\t%(braille)s" % rule)
                    println()
                    problems += 1
                if not args.TOTAL_RATE and problems >= args.MAX_PROBLEMS:
                    break
            else:
                correct += 1
            total += 1
            update_counters(total)
    if args.TOTAL_RATE:
        println("### %d out of %d (%.1f %%) words translated correctly" % (correct, total, math.floor(1000 * correct / total) / 10))
    elif correct > 0:
        println("### %d words translated correctly" % correct)
    conn.close()

if __name__ == "__main__": main()