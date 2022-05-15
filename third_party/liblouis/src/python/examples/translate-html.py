#! /usr/bin/python -u
#
# Copyright (C) 2020 Jake Kyle <jake@compassbraille.org>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved. This file is offered as-is,
# without any warranty.

"""Convert an HTML file to Braille

This is a tiny example that illustrates how you can use lxml to read
HTML and then translate only the text contained in the HTML nodes.

Similar scripts can be written for XML in general using lxml.etree.
Also there is much fine grained control possible by selecting
different elements and attributes more specifically, using xpath and
other methods available within lxml, to do different things
"""

import textwrap
import louis
from lxml import html

tableList = ["en-ueb-g2.ctb"]
lineLength = 38
fileIn = input("Please enter the input file name: ")
fileOut = input("Please enter the output file name: ")

with open(fileOut, "w") as outputFile:
    html_root = html.parse(fileIn).getroot()
    for head_or_body in html_root:
        for elem in head_or_body:
            if elem.xpath("string()").strip() != "":
                line = elem.xpath("string()")
                translation = louis.translateString(tableList, line, 0, 0)
                outputFile.write(textwrap.fill(translation, lineLength))
                outputFile.write("\n")

print ("Done.")
