#!/bin/sh

extra_cflag=$1

echo "/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */"
echo "#ifndef MEDIA_EXTRA_H"
echo "#define MEDIA_EXTRA_H"
echo ""

echo "$extra_cflag" | tr ':' '\n' | while read unit; do
    if [[ -z "${unit// }" ]]; then
        continue
    fi
    if [[ "$unit" == *"#"* ]]; then
        macro_name="${unit%%#*}"
        macro_value="${unit#*#}"
        echo "#define $macro_name $macro_value"
    else
        echo "#define $unit"
    fi
done

echo ""
echo "#endif"
