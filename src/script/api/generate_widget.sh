#!/bin/bash

# $Id: generate_widget.sh 24664 2012-11-05 19:53:05Z frosch $

# This file is part of OpenTTD.
# OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.

# Set neutral locale so sort behaves the same everywhere
LC_ALL=C
export LC_ALL

# We really need gawk for this!
AWK=gawk

${AWK} --version > /dev/null 2> /dev/null
if [ "$?" != "0" ]; then
	echo "This script needs gawk to run properly"
	exit 1
fi

${AWK} -f generate_widget.awk script_window.hpp > script_window.tmp
mv script_window.tmp script_window.hpp
