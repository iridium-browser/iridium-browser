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

from ctypes import *
from itertools import takewhile, zip_longest, chain, tee
izip = zip
izip_longest = zip_longest

from louis import _loader, liblouis, wideCharBytes, outlenMultiplier, createEncodedByteString
import re
import sqlite3
import sys

def exit_if_not(expression):
    if not expression:
        raise RuntimeError()

exit_if_not(liblouis.lou_charSize() == 4)

liblouis.isLetter.argtypes = (c_wchar,)
liblouis.toLowercase.argtypes = (c_wchar,)
liblouis.toLowercase.restype = c_wchar

def println(line=""):
    sys.stdout.write(("%s\n" % line))

def printerrln(line=""):
    sys.stderr.write(("%s\n" % line))

def validate_chunks(chunked_text):
    return re.search(r"^([^)(|]+|\([^)(|][^)(|]+\))(\|?([^)(|]+|\([^)(|][^)(|]+\)))*$", chunked_text) != None

def print_chunks(text, hyphen_string):
    exit_if_not(len(hyphen_string) == len(text) - 1 and re.search("^[01x]+$", hyphen_string))
    chunked_text = []
    k = 0
    prev_c = None
    for c in hyphen_string:
        if c != prev_c and c == "0":
            chunked_text.append("(")
        chunked_text.append(text[k])
        if c != prev_c and prev_c == "0":
            chunked_text.append(")")
        if c == "1":
            chunked_text.append("|")
        prev_c = c
        k += 1
    chunked_text.append(text[k])
    if (prev_c == "0"):
        chunked_text.append(")")
    return "".join(chunked_text)

def parse_chunks(chunked_text):
    exit_if_not(validate_chunks(chunked_text))
    text, _ = read_text(chunked_text)
    hyphen_string = ["x"] * (len(text) - 1)
    k = 0
    for c in chunked_text:
        if c == "(":
            hyphen_string[k:] = ["0"] * (len(text) - 1 - k)
        elif c == ")":
            hyphen_string[k-1:] =  ["x"] * (len(text) - 1 - (k-1))
        elif c == "|":
            hyphen_string[k-1] = "1"
        else:
            k += 1
            if k > len(text):
                break
    return text, "".join(hyphen_string)

def read_text(maybe_chunked_text):
    if re.search("[)(|]", maybe_chunked_text) != None:
        text = re.sub("[)(|]", "", maybe_chunked_text)
        chunked_text = maybe_chunked_text
    else:
        text = maybe_chunked_text
        chunked_text = None
    return text, chunked_text

def compare_chunks(expected_hyphen_string, actual_hyphen_string, text):
    exit_if_not(len(expected_hyphen_string) == len(text) - 1 and re.search("^[01x]+$", expected_hyphen_string))
    exit_if_not(len(actual_hyphen_string) == len(text) - 1 and re.search("^[01]+$", actual_hyphen_string))
    chunk_errors = my_zip(text,
                          map(lambda e, a: "*" if e in "1x" and a == "1" else
                                           "." if a == "1" else
                                           "-" if e == "1" else None,
                              expected_hyphen_string, actual_hyphen_string))
    return chunk_errors if re.search(r"[-\.]", chunk_errors) else None

# split a string into words consisting of only letters (at least two)
# return an empty list if the provided hyphen string does not have zeros at all positions before and after non-letters
def split_into_words(text, hyphen_string):
    exit_if_not(len(hyphen_string) == len(text) - 1 and re.search("^[01x]+$", hyphen_string))
    words = []
    word_hyphen_strings = []
    word = []
    word_hyphen_string = []
    for c,(h1,h2) in izip(text, pairwise('1' + hyphen_string + '1')):
        if is_letter(c):
            word.append(c)
            word_hyphen_string.append(h1)
        elif h1 not in "1x" or h2 not in "1x":
            return []
        else:
            if len(word) > 1:
                words.append("".join(word))
                word_hyphen_strings.append("".join(word_hyphen_string[1:]))
            word = []
            word_hyphen_string = []
    if len(word) > 1:
        words.append("".join(word))
        word_hyphen_strings.append("".join(word_hyphen_string[1:]))
    return izip(words, word_hyphen_strings)

table = None

def load_table(new_table):
    global table
    table = new_table
    table = table.encode("ASCII") if isinstance(table, str) else bytes(table)
    liblouis.loadTable(table);

def is_letter(text):
    return all([liblouis.isLetter(c) for c in text])

def to_lowercase(text):
    return "".join([liblouis.toLowercase(c) for c in text])

def to_dot_pattern(braille):
    c_braille = create_unicode_buffer(braille)
    c_dots = create_string_buffer(9 * len(braille))
    liblouis.toDotPattern(c_braille, c_dots)
    return c_dots.value.decode('ascii')

def hyphenate(text):
    c_text = createEncodedByteString(text)
    c_text_len = c_int(int(len(c_text)/wideCharBytes))
    c_hyphen_string = create_string_buffer(c_text_len.value + 1)
    exit_if_not(liblouis.lou_hyphenate(table, c_text, c_text_len, c_hyphen_string, 0))
    return "".join(['1' if int(p) % 2 else '0' for p in c_hyphen_string.value[1:]])

def translate(text):
    c_text = create_unicode_buffer(text)
    c_text_len = c_int(len(text))
    braille_len = len(text) * outlenMultiplier
    c_braille = create_unicode_buffer(braille_len)
    c_braille_len = c_int(braille_len)
    max_rules = 16
    c_rules = (c_void_p * max_rules)()
    c_rules_len = c_int(max_rules)
    exit_if_not(liblouis._lou_translate(table, table, c_text, byref(c_text_len), c_braille, byref(c_braille_len),
                                        None, None, None, None, None, 0, c_rules, byref(c_rules_len)))
    return c_braille.value, c_rules[0:c_rules_len.value]

def get_rule(c_rule_pointer):
    c_rule_string = create_unicode_buffer(u"", 128)
    if not liblouis.printRule(cast(c_rule_pointer, c_void_p), c_rule_string):
        return None
    return tuple(c_rule_string.value.split("\t"))

def suggest_chunks(text, braille):
    c_text = create_unicode_buffer(text)
    c_braille = create_unicode_buffer(braille)
    c_hyphen_string = create_string_buffer(len(text) + 2)
    if not liblouis.suggestChunks(c_text, c_braille, c_hyphen_string):
        return None;
    hyphen_string = c_hyphen_string.value.decode('ascii')
    hyphen_string = hyphen_string[1:len(hyphen_string)-1]
    assert len(hyphen_string) == len(text) - 1 and re.search("^[01x]+$", hyphen_string)
    return hyphen_string

def find_relevant_rules(text):
    c_text = create_unicode_buffer(text)
    max_rules = 16
    c_rules = [u""] * max_rules + [None]
    for i in range(0, max_rules):
        c_rules[i] = create_unicode_buffer(c_rules[i], 128)
        c_rules[i] = cast(c_rules[i], c_wchar_p)
    c_rules = (c_wchar_p * (max_rules + 1))(*c_rules)
    liblouis.findRelevantRules(c_text, c_rules)
    return map(lambda x: tuple(x.split("\t")), takewhile(lambda x: x, c_rules))

def open_dictionary(dictionary):
    conn = sqlite3.connect(dictionary)
    c = conn.cursor()
    return conn, c

def filterfalse(predicate, iterable):
    return [x for x in iterable if not predicate(x)]

def partition(pred, iterable):
    t1, t2 = tee(iterable)
    return filterfalse(pred, t1), filter(pred, t2)

def pairwise(iterable):
    a, b = tee(iterable)
    next(b, None)
    return izip(a, b)

def my_zip(*iterables):
    return "".join([x for x in chain(*izip_longest(*iterables)) if x is not None])

class future:
    def __init__(self, f):
        self.f = f
        self.fut = f
        self.is_realized = False
    def __call__(self):
        if not self.is_realized:
            self.fut = self.f()
            self.is_realized = True
        return self.fut