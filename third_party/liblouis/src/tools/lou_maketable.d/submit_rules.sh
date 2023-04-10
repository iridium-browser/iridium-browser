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

CURDIR=$(cd $(dirname "$0") && pwd)
WORKING_FILE=${1}
CONTRACTIONS_TABLE=${2}
BASE_TABLE=${3}
RULE_GREP="^[ \t]*[+-]\?\(nocross[ \t][ \t]*\)\?\(always\|word\|begword\|endword\|midword\|begmidword\|midendword\|prfword\|sufword\)"
set -e
[[ $VERBOSE == true ]] && set -x
if [ -e $WORKING_FILE ]; then
	cat $WORKING_FILE | grep -v "^$" | grep -v "^#" > tmp || true
	mv tmp $WORKING_FILE
	if cat $WORKING_FILE | grep "$RULE_GREP" > tmp; then
		python3 $CURDIR/submit_rules.py -t $BASE_TABLE tmp $CONTRACTIONS_TABLE > tmp2
		mv tmp2 tmp
		cat <<- EOF > $CONTRACTIONS_TABLE
			##################################
			# don't edit this file directly! #
			##################################
		EOF
		cat tmp >> $CONTRACTIONS_TABLE
		cat $WORKING_FILE | grep -v "$RULE_GREP" > tmp || true
		mv tmp $WORKING_FILE
	else
		rm -f tmp
	fi
	[ -s $WORKING_FILE ] || rm $WORKING_FILE
fi
