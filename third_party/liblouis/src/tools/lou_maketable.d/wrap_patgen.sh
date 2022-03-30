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

set -e
DICTIONARY_FILE=${1}
PATTERN_FILE=${2}
PATOUT_FILE=${3}
TRANSLATE_FILE=${4}
LEFT_HYPHEN_MIN=${5}
LEFT_HYPHEN_MAX=${6}
HYPH_LEVEL=${7}
PAT_START=${8}
PAT_FINISH=${9}
GOOD_WEIGHT=${10}
BAD_WEIGHT=${11}
THRESHOLD=${12}
FIFO=tmp
rm -f $FIFO
mkfifo $FIFO
patgen $DICTIONARY_FILE $PATTERN_FILE $PATOUT_FILE $TRANSLATE_FILE <$FIFO &
echo $LEFT_HYPHEN_MIN $LEFT_HYPHEN_MAX >$FIFO
echo $HYPH_LEVEL $HYPH_LEVEL >$FIFO
echo $PAT_START $PAT_FINISH >$FIFO
echo $GOOD_WEIGHT $BAD_WEIGHT $THRESHOLD >$FIFO
echo y >$FIFO
wait $!
ret=$?
rm -f $FIFO
if [ $ret == 0 ]; then
    touch $PATOUT_FILE pattmp.$HYPH_LEVEL
else
    rm -f $PATOUT_FILE pattmp.$HYPH_LEVEL
fi
exit $ret
