/*
 * This file is part of LMFAO (Look, Malbolge Unshackled From Assembly, Ooh!),
 * an assembler for Malbolge Unshackled.
 * Copyright (C) 2016 Matthias Lutter
 *
 * LMFAO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LMFAO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * E-Mail: matthias@lutter.cc
 */
 
#include <stdio.h>
#include "malbolge.h"

Number crazy(Number a, Number d){	
	long long max_value_C1_rot_width = 1;
	unsigned int width = 1;
	long long a_val = -1;
	long long d_val = -1;
	const long long crz[] = {1,0,0,1,0,2,2,2,1};
	unsigned int position = 0;
	long long multiple = 1;
	long long i,j,out;
	Number ret;
	
	while ((a.offset == T1 && (a.val > max_value_C1_rot_width || -a.val > max_value_C1_rot_width))
			|| (a.val > 2*max_value_C1_rot_width)
			|| (-a.val > 2*max_value_C1_rot_width)
			|| (d.offset == T1 && (d.val > max_value_C1_rot_width || -d.val > max_value_C1_rot_width))
			|| (d.val > 2*max_value_C1_rot_width)
			|| (-d.val > 2*max_value_C1_rot_width)
			) {
		max_value_C1_rot_width *= 3;
		max_value_C1_rot_width += 1;
		width++;
	}

	a_val = a.val;
	if (a.offset == T2) {
		a_val += 2*max_value_C1_rot_width;
	}
	if (a.offset == T1) {
		a_val += max_value_C1_rot_width;
	}
	d_val = d.val;
	if (d.offset == T2) {
		d_val += 2*max_value_C1_rot_width;
	}
	if (d.offset == T1) {
		d_val += max_value_C1_rot_width;
	}
	
	ret.val = 0;
	ret.offset = T0;
	while (position < width){
		i = a_val % 3;
		j =  d_val % 3;
		out = crz[i+3*j];
		ret.val += multiple*out;
		a_val /= 3;
		d_val /= 3;
		position++;
		multiple *= 3;
	}
	
	i = (a.offset == T2?2:(a.offset == T1?1:0));
	j = (d.offset == T2?2:(d.offset == T1?1:0));
	out = crz[i+3*j];
	if (out == 1) {
		ret.offset = T1;
		ret.val -= max_value_C1_rot_width;
	}
	if (out == 2) {
		ret.offset = T2;
		ret.val -= 2*max_value_C1_rot_width;
	}
	return ret;
}

