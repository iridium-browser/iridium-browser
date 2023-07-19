# Liblouis Python ctypes bindings
#
# Copyright (C) 2009, 2010 James Teh <jamie@jantrid.net>
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

"""Liblouis Python ctypes bindings
These bindings allow you to use the liblouis braille translator and back-translator library
from within Python.
This documentation is only related to the Python helper.
Please see the liblouis documentation for more information.

Most of these functions take a C{tableList}      argument which specifies
a list of translation tables to use. Please see the liblouis documentation
concerning the C{tableList} parameter to the C{lou_translateString}
function for information about how liblouis searches for these tables.

@author: Michael Curran <mick@kulgan.net>
@author: James Teh <jamie@jantrid.net>
@author: Eitan Isaacson <eitan@ascender.com>
@author: Michael Whapples <mwhapples@aim.com>
@author: Davy Kager <mail@davykager.nl>
@author: Leonard de Ruijter <alderuijter@gmail.com>
@author: Babbage B.V. <info@babbage.com>
@author: Andre-Abush Clause <dev@andreabc.net>
"""

from sys import getfilesystemencoding, platform, version_info
from atexit import register
from ctypes import (
    c_ushort,
    CFUNCTYPE,
    cdll,
    c_char_p,
    c_char,
    c_int,
    POINTER,
    byref,
    create_string_buffer,
)

try:  # Native win32
    from ctypes import WINFUNCTYPE, windll
    _loader, _functype = windll, WINFUNCTYPE
except ImportError:  # Unix/Cygwin
    _loader, _functype = cdll, CFUNCTYPE
liblouis = _loader["###LIBLOUIS_SONAME###"]
_is_windows = platform == "win32"

# { Module Configuration
#: Specifies the charSize (in bytes) used by liblouis.
#: This is fetched once using L{liblouis.lou_charSize}.
#: Call it directly, since L{charSize} is not yet defined.
#: @type: int
wideCharBytes = liblouis.lou_charSize()
#: Specifies the number by which the input length should be multiplied
#: to calculate the maximum output length.
#: @type: int
# This default will handle the case where every input character is
# undefined in the translation table.
outlenMultiplier = 4 + wideCharBytes * 2
#: Specifies the encoding to use when encode/decode file/dir name
#: @type: str
fileSystemEncoding = "mbcs" if _is_windows else getfilesystemencoding()
#: Specifies the encoding to use when converting from byte strings to unicode strings.
#: @type: str
conversionEncoding = "utf_%d_le" % (wideCharBytes * 8)
# }

# Some general utility functions
def _createTablesString(tablesList):
    """Creates a tables string for liblouis calls"""
    return b",".join(
        [
            x.encode(fileSystemEncoding) if isinstance(x, str) else bytes(x)
            for x in tablesList
        ]
    )


def _createTypeformbuf(length, typeform=None):
    """Creates a typeform buffer for liblouis calls"""
    return (c_ushort * length)(*typeform) if typeform else (c_ushort * length)()


ENCODING_ERROR_HANDLER = "surrogatepass"


def createEncodedByteString(
    x, encoding=conversionEncoding, errors=ENCODING_ERROR_HANDLER
):
    return str(x).encode(encoding, errors)


register(liblouis.lou_free)

liblouis.lou_version.restype = c_char_p

liblouis.lou_charSize.restype = c_int

liblouis.lou_translateString.argtypes = (
    c_char_p,
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_ushort),
    POINTER(c_char),
    c_int,
)

liblouis.lou_translate.argtypes = (
    c_char_p,
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_ushort),
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_int),
    POINTER(c_int),
    c_int,
)

liblouis.lou_backTranslateString.argtypes = (
    c_char_p,
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_ushort),
    POINTER(c_char),
    c_int,
)

liblouis.lou_backTranslate.argtypes = (
    c_char_p,
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_ushort),
    POINTER(c_char),
    POINTER(c_int),
    POINTER(c_int),
    POINTER(c_int),
    c_int,
)

liblouis.lou_hyphenate.argtypes = (
    c_char_p,
    POINTER(c_char),
    c_int,
    POINTER(c_char),
    c_int,
)

liblouis.lou_checkTable.argtypes = (c_char_p,)

liblouis.lou_compileString.argtypes = (c_char_p, c_char_p)

liblouis.lou_getTypeformForEmphClass.argtypes = (c_char_p, c_char_p)

liblouis.lou_dotsToChar.argtypes = (
    c_char_p,
    POINTER(c_char),
    POINTER(c_char),
    c_int,
    c_int,
)

liblouis.lou_charToDots.argtypes = (
    c_char_p,
    POINTER(c_char),
    POINTER(c_char),
    c_int,
    c_int,
)

LogCallback = _functype(None, c_int, c_char_p)

liblouis.lou_registerLogCallback.restype = None

liblouis.lou_setLogLevel.restype = None
liblouis.lou_setLogLevel.argtypes = (c_int,)


def version():
    """Obtain version information for liblouis.
    @return: The version of liblouis, plus other information, such as
        the release date and perhaps notable changes.
    @rtype: str
    """
    return liblouis.lou_version().decode("ASCII")


def charSize():
    """Obtain charSize information for liblouis.
    @return: The size of the widechar with which liblouis was compiled.
    @rtype: int
    """
    return liblouis.lou_charSize()


def translate(tableList, inbuf, typeform=None, cursorPos=0, mode=0):
    """Translate a string of characters, providing position information.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @param inbuf: The string to translate.
    @type inbuf: str
    @param typeform: A list of typeform constants indicating the typeform for each position in inbuf,
        C{None} for no typeform information.
    @type typeform: list of int
    @param cursorPos: The position of the cursor in inbuf.
    @type cursorPos: int
    @param mode: The translation mode; add multiple values for a combined mode.
    @type mode: int
    @return: A tuple of: the translated string,
        a list of input positions for each position in the output,
        a list of output positions for each position in the input, and
        the position of the cursor in the output.
    @rtype: (str, list of int, list of int, int)
    @raise RuntimeError: If a complete translation could not be done.
    @see: lou_translate in the liblouis documentation
    """
    tablesString = _createTablesString(tableList)
    inbuf = createEncodedByteString(inbuf)
    inlen = c_int(len(inbuf) // wideCharBytes)
    outlen = c_int(inlen.value * outlenMultiplier)
    outbuf = create_string_buffer(outlen.value * wideCharBytes)
    typeformbuf = None
    if typeform:
        typeformbuf = _createTypeformbuf(outlen.value, typeform)
    inPos = (c_int * outlen.value)()
    outPos = (c_int * inlen.value)()
    cursorPos = c_int(cursorPos)
    if not liblouis.lou_translate(
        tablesString,
        inbuf,
        byref(inlen),
        outbuf,
        byref(outlen),
        typeformbuf,
        None,
        outPos,
        inPos,
        byref(cursorPos),
        mode,
    ):
        raise RuntimeError(
            "Can't translate: tables %s, inbuf %s, typeform %s, cursorPos %s, mode %s"
            % (tableList, inbuf, typeform, cursorPos, mode)
        )
    if isinstance(typeform, list):
        typeform[:] = list(typeformbuf)
    return (
        outbuf.raw[: outlen.value * wideCharBytes].decode(
            conversionEncoding, errors=ENCODING_ERROR_HANDLER
        ),
        inPos[: outlen.value],
        outPos[: inlen.value],
        cursorPos.value,
    )


def translateString(tableList, inbuf, typeform=None, mode=0):
    """Translate a string of characters.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @param inbuf: The string to translate.
    @type inbuf: str
    @param typeform: A list of typeform constants indicating the typeform for each position in inbuf,
        C{None} for no typeform information.
    @type typeform: list of int
    @param mode: The translation mode; add multiple values for a combined mode.
    @type mode: int
    @return: The translated string.
    @rtype: str
    @raise RuntimeError: If a complete translation could not be done.
    @see: lou_translateString in the liblouis documentation
    """
    tablesString = _createTablesString(tableList)
    inbuf = createEncodedByteString(inbuf)
    inlen = c_int(len(inbuf) // wideCharBytes)
    outlen = c_int(inlen.value * outlenMultiplier)
    outbuf = create_string_buffer(outlen.value * wideCharBytes)
    typeformbuf = None
    if typeform:
        typeformbuf = _createTypeformbuf(outlen.value, typeform)
    if not liblouis.lou_translateString(
        tablesString,
        inbuf,
        byref(inlen),
        outbuf,
        byref(outlen),
        typeformbuf,
        None,
        mode,
    ):
        raise RuntimeError(
            "Can't translate: tables %s, inbuf %s, typeform %s, mode %s"
            % (tableList, inbuf, typeform, mode)
        )
    if isinstance(typeform, list):
        typeform[:] = list(typeformbuf)
    return outbuf.raw[: outlen.value * wideCharBytes].decode(
        conversionEncoding, errors=ENCODING_ERROR_HANDLER
    )


def backTranslate(tableList, inbuf, typeform=None, cursorPos=0, mode=0):
    """Back translates a string of characters, providing position information.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @param inbuf: Braille to back translate.
    @type inbuf: str
    @param typeform: List where typeform constants will be placed.
    @type typeform: list
    @param cursorPos: Position of cursor.
    @type cursorPos: int
    @param mode: Translation mode.
    @type mode: int
    @return: A tuple: A string of the back translation,
        a list of input positions for each position in the output,
        a list of the output positions for each position in the input and
        the position of the cursor in the output.
    @rtype: (str, list of int, list of int, int)
    @raise RuntimeError: If a complete back translation could not be done.
    @see: lou_backTranslate in the liblouis documentation.
    """
    tablesString = _createTablesString(tableList)
    inbuf = createEncodedByteString(inbuf)
    inlen = c_int(len(inbuf) // wideCharBytes)
    outlen = c_int(inlen.value * outlenMultiplier)
    outbuf = create_string_buffer(outlen.value * wideCharBytes)
    typeformbuf = None
    if isinstance(typeform, list):
        typeformbuf = _createTypeformbuf(outlen.value)
    inPos = (c_int * outlen.value)()
    outPos = (c_int * inlen.value)()
    cursorPos = c_int(cursorPos)
    if not liblouis.lou_backTranslate(
        tablesString,
        inbuf,
        byref(inlen),
        outbuf,
        byref(outlen),
        typeformbuf,
        None,
        outPos,
        inPos,
        byref(cursorPos),
        mode,
    ):
        raise RuntimeError(
            "Can't back translate: tables %s, inbuf %s, typeform %s, cursorPos %d, mode %d"
            % (tableList, inbuf, typeform, cursorPos, mode)
        )
    if isinstance(typeform, list):
        typeform[:] = list(typeformbuf)
    return (
        outbuf.raw[: outlen.value * wideCharBytes].decode(
            conversionEncoding, errors=ENCODING_ERROR_HANDLER
        ),
        inPos[: outlen.value],
        outPos[: inlen.value],
        cursorPos.value,
    )


def backTranslateString(tableList, inbuf, typeform=None, mode=0):
    """Back translate from Braille.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @param inbuf: The Braille to back translate.
    @type inbuf: str
    @param typeform: List for typeform constants to be put in.
        If you don't want typeform data then give None
    @type typeform: list
    @param mode: The translation mode
    @type mode: int
    @return: The back translation of inbuf.
    @rtype: str
    @raise RuntimeError: If a complete back translation could not be done.
    @see: lou_backTranslateString in the liblouis documentation.
    """
    tablesString = _createTablesString(tableList)
    inbuf = createEncodedByteString(inbuf)
    inlen = c_int(len(inbuf) // wideCharBytes)
    outlen = c_int(inlen.value * outlenMultiplier)
    outbuf = create_string_buffer(outlen.value * wideCharBytes)
    typeformbuf = None
    if isinstance(typeform, list):
        typeformbuf = _createTypeformbuf(outlen.value)
    if not liblouis.lou_backTranslateString(
        tablesString,
        inbuf,
        byref(inlen),
        outbuf,
        byref(outlen),
        typeformbuf,
        None,
        mode,
    ):
        raise RuntimeError(
            "Can't back translate: tables %s, inbuf %s, mode %d"
            % (tableList, inbuf, mode)
        )
    if isinstance(typeform, list):
        typeform[:] = list(typeformbuf)
    return outbuf.raw[: outlen.value * wideCharBytes].decode(
        conversionEncoding, errors=ENCODING_ERROR_HANDLER
    )


def hyphenate(tableList, inbuf, mode=0):
    """Get information for hyphenation.
    @param tableList: A list of translation tables and hyphenation
        dictionaries.
    @type tableList: list of str
    @param inbuf: The text to get hyphenation information about.
        This should be a single word and leading/trailing whitespace
        and punctuation is ignored.
    @type inbuf: str
    @param mode: Lets liblouis know if inbuf is plain text or Braille.
        Set to 0 for text and anyother value for Braille.
    @type mode: int
    @return: A string with '1' at the beginning of every syllable
        and '0' elsewhere.
    @rtype: str
    @raise RuntimeError: If hyphenation data could not be produced.
    @see: lou_hyphenate in the liblouis documentation.
    """
    tablesString = _createTablesString(tableList)
    inbuf = createEncodedByteString(inbuf)
    inlen = c_int(len(inbuf) // wideCharBytes)
    hyphen_string = create_string_buffer(inlen.value + 1)
    if not liblouis.lou_hyphenate(tablesString, inbuf, inlen, hyphen_string, mode):
        raise RuntimeError(
            "Can't hyphenate: tables %s, inbuf %s, mode %d" % (tableList, inbuf, mode)
        )
    return hyphen_string.value.decode("ASCII")


def checkTable(tableList):
    """Check if the specified tables can be found and compiled.
        This can be used to check if a list of tables contains errors
        before sending it to other liblouis functions
        that accept a list of tables.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @raise RuntimeError: If compilation failed.
    @see: lou_checkTable in the liblouis documentation
    """
    tablesString = _createTablesString(tableList)
    if not liblouis.lou_checkTable(tablesString):
        raise RuntimeError("Can't compile: tables %s" % tableList)


def compileString(tableList, inString):
    """Compile a table entry on the fly at run-time.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @param inString: The table entry to be added.
    @type inString: str
    @raise RuntimeError: If compilation of the entry failed.
    @see: lou_compileString in the liblouis documentation
    """
    tablesString = _createTablesString(tableList)
    inBytes = inString.encode("ASCII") if isinstance(inString, str) else bytes(inString)
    if not liblouis.lou_compileString(tablesString, inBytes):
        raise RuntimeError(
            "Can't compile entry: tables %s, inString %s" % (tableList, inString)
        )


def getTypeformForEmphClass(tableList, emphClass):
    """Get the typeform bit for the named emphasis class.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @param emphClass: An emphasis class name.
    @type emphClass: str
    @see: lou_getTypeformForEmphClass in the liblouis documentation
    """
    tablesString = _createTablesString(tableList)
    if _is_py3:
        emphClass = emphClass.encode("ASCII")
    return liblouis.lou_getTypeformForEmphClass(tablesString, emphClass)


def dotsToChar(tableList, inbuf):
    """"Convert a string of dot patterns to a string of characters according to the specifications in tableList.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @param inbuf: a string of dot patterns, either in liblouis format or Unicode braille.
    @type inbuf: str
    @raise RuntimeError: If a complete conversion could not be done.
    @see: lou_dotsToChar in the liblouis documentation
    """
    tablesString = _createTablesString(tableList)
    inbuf = createEncodedByteString(inbuf)
    length = c_int(len(inbuf) // wideCharBytes)
    outbuf = create_string_buffer(length.value * wideCharBytes)
    if not liblouis.lou_dotsToChar(tablesString, inbuf, outbuf, length, 0):
        raise RuntimeError(
            "Can't convert dots to char: tables %s, inbuf %s" % (tableList, inbuf)
        )
    return outbuf.raw[: length.value * wideCharBytes].decode(
        conversionEncoding, errors=ENCODING_ERROR_HANDLER
    )


def charToDots(tableList, inbuf, mode=0):
    """"Convert a string of characterss to a string of dot patterns according to the specifications in tableList.
    @param tableList: A list of translation tables.
    @type tableList: list of str
    @param inbuf: a string of characters.
    @type inbuf: str
    @param mode: The translation mode; add multiple values for a combined mode.
    @type mode: int
    @raise RuntimeError: If a complete conversion could not be done.
    @see: lou_charToDots in the liblouis documentation
    """
    tablesString = _createTablesString(tableList)
    inbuf = createEncodedByteString(inbuf)
    length = c_int(len(inbuf) // wideCharBytes)
    outbuf = create_string_buffer(length.value * wideCharBytes)
    if not liblouis.lou_charToDots(tablesString, inbuf, outbuf, length, mode):
        raise RuntimeError(
            "Can't convert char to dots: tables %s, inbuf %s, mode %d"
            % (tableList, inbuf, mode)
        )
    return outbuf.raw[: length.value * wideCharBytes].decode(
        conversionEncoding, errors=ENCODING_ERROR_HANDLER
    )


def registerLogCallback(logCallback):
    """Register logging callbacks.
    Set to C{None} for default callback.
    @param logCallback: The callback to use.
        The callback must take two arguments:
        @param level: The log level on which a message is logged.
        @type level: int
        @param message: The logged message.
            Note that the callback should provide its own ASCII decoding routine.
        @type message: bytes

        Example callback:

        @louis.LogCallback
        def incomingLouisLog(level, message):
            print("Message %s logged at level %d" % (message.decode("ASCII"), level))

    @type logCallback: L{LogCallback}
    """
    if logCallback is not None and not isinstance(logCallback, LogCallback):
        raise TypeError(
            "logCallback should be of type {} or NoneType".format(LogCallback.__name__)
        )
    return liblouis.lou_registerLogCallback(logCallback)


def setLogLevel(level):
    """Set the level for logging callback to be called at.
    @param level: one of the C{LOG_*} constants.
    @type level: int
    @raise ValueError: If an invalid log level is provided.
    """
    if level not in logLevels:
        raise ValueError("Level %d is an invalid log level" % level)
    return liblouis.lou_setLogLevel(level)


# { Typeforms
plain_text = 0x0000
emph_1 = comp_emph_1 = italic = 0x0001
emph_2 = comp_emph_2 = underline = 0x0002
emph_3 = comp_emph_3 = bold = 0x0004
emph_4 = 0x0008
emph_5 = 0x0010
emph_6 = 0x0020
emph_7 = 0x0040
emph_8 = 0x0080
emph_9 = 0x0100
emph_10 = 0x0200
computer_braille = 0x0400
no_translate = 0x0800
no_contract = 0x1000
# }

# { Translation modes
noContractions = 1
compbrlAtCursor = 2
dotsIO = 4
compbrlLeftCursor = 32
ucBrl = 64
noUndefined = 128
noUndefinedDots = noUndefined  # alias for backward compatiblity
partialTrans = 256
# }

# { logLevels
LOG_ALL = 0
LOG_DEBUG = 10000
LOG_INFO = 20000
LOG_WARN = 30000
LOG_ERROR = 40000
LOG_FATAL = 50000
LOG_OFF = 60000
# }

logLevels = (LOG_ALL, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL, LOG_OFF)

if __name__ == "__main__":
    # Just some common tests.
    print(version())
    print(translate([b"../tables/en-us-g2.ctb"], "Hello world!", cursorPos=5))
