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
from itertools import groupby
from utils import *

def main():
    parser = argparse.ArgumentParser(description="Generate translate file for patgen.")
    parser.add_argument('-d', '--dictionary', required=True, dest="DICTIONARY",
                        help="dictionary file")
    parser.add_argument('-t', '--table', required=True, dest="TABLE",
                        help="translation table to be used for converting letters to lowercase")
    args = parser.parse_args()
    load_table(args.TABLE)
    conn, c = open_dictionary(args.DICTIONARY)
    c.execute("SELECT text FROM dictionary WHERE braille IS NOT NULL")
    alphabet = ""
    for text, in c.fetchall():
        alphabet = "".join(set(alphabet + text))
    print("")
    for k, g in groupby(sorted(alphabet, key=to_lowercase), to_lowercase):
        g = list(g)
        g.remove(k)
        println(" " + " ".join([k] + g))
    conn.close()

if __name__ == "__main__": main()