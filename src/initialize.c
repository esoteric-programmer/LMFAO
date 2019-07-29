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

#include "malbolge.h"
#include "main.h"
#include "initialize.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void update_offsets(MemoryList memory){
	MemoryListElement* it = memory.first;
	/* save all offsets. */
	while (it != 0){
		if (it->cell != 0) {
			if (it->cell->usage == CODE || it->cell->usage == PREINITIALIZED_CODE) {
				it->cell->code->offset.offset = it->address.offset;
				it->cell->code->offset.val = it->address.val;
			} else if (it->cell->usage == DATA || it->cell->usage == RESERVED_DATA) {
				it->cell->data->offset.offset = it->address.offset;
				it->cell->data->offset.val = it->address.val;
			}
		}
		it = it->next;
	}
}


Number parse_dataatom(DataAtom* data, LabelTree* labeltree){
	Number value;
	value.offset = UNSET;
	value.val = -1;
	if (data==0) {
			return value; /* ERROR */
	}
	if (data->destination_label == 0) {
		/* data is a constant integer number. */
		value.offset = data->number.offset;
		value.val = data->number.val;
	}else{
		/* data is an address. */
		/* get type of addressed memory cell: data or code word. */
		CodeBlock* destination_c = 0;
		/* get addressed memory cell.*/
		DataBlock* destination_d = 0;
		if (!get_label(&destination_c, &destination_d, data->destination_label, labeltree)){
			fprintf(stderr,"Internal error: Cannot find label %s again.\n", data->destination_label);
			return value; /*ERROR! */
		}
		/* destination_type: CODE = 0; DATA = 1 */
		/* cast destination pointer according to destination_type. */
		/* add offset to destination address */
		if (destination_c != NULL) {
			if (data->number.offset != T0 && data->number.offset != T2) {
				// TODO: can this occur?
				fprintf(stderr,"Internal error: FIXME: Unexpected offset while parsing %s.\n", data->destination_label);
				return value;
			}
			value.offset = destination_c->offset.offset;
			value.val = destination_c->offset.val + (data->number.offset == T2?-1:0)+data->number.val;
		}else if (destination_d != NULL) {
			if (data->number.offset != T0 && data->number.offset != T2) {
				// TODO: can this occur?
				fprintf(stderr,"Internal error: FIXME: Unexpected offset while parsing %s.\n", data->destination_label);
				return value;
			}
			value.offset = destination_d->offset.offset;
			value.val = destination_d->offset.val + (data->number.offset == T2?-1:0)+data->number.val;
		}else{
			fprintf(stderr,"Internal error: Invalid type of label %s.\n",data->destination_label);
			return value; /*ERROR! */
		}
		/* subtract one from pointer address: this is done to compensate the auto increment of C and D register in Malbolge. */
		/* handle underflow */
		value.val--;
	}
	/* normalize value */
	if (value.val < 0 && value.offset == T0) {
		value.val++;
		value.offset = T2;
	}
	if (value.val > 0 && value.offset == T2) {
		value.val--;
		value.offset = T0;
	}
	return value;
}

Number parse_datacell(DataCell* data, LabelTree* labeltree) {
	Number value;
	Number left; 
	Number right;
	value.offset = UNSET;
	value.val = -1;
	left.offset = UNSET;
	left.val = -1;
	right.offset = UNSET;
	right.val = -1;
	if (data==0) {
		return value; /* ERROR */
	}
	if (data->leaf_element != 0) {
		return parse_dataatom(data->leaf_element, labeltree);
	}
	left = parse_datacell(data->left_element, labeltree);
	if (left.offset == UNSET) {
		return value; /* ERROR */
	}
	right = parse_datacell(data->right_element, labeltree);
	if (right.offset == UNSET) {
		return value; /* ERROR */
	}
	switch (data->_operator) {
		case DATACELL_OPERATOR_PLUS:
		{
			if (left.offset == T2) {
				left.offset = T0;
				left.val--;
			}
			if (right.offset == T2) {
				right.offset = T0;
				right.val--;
			}
				
			if (left.offset == T0 && right.offset == T1) {
				value.offset = T1;
			} else if (left.offset == T1 && right.offset == T0) {
				value.offset = T1;
			} else if (left.offset == T1 && right.offset == T1) {
				value.offset = T2;
			} else {
				value.offset = T0;
			}
			value.val = left.val + right.val;
			break;
		}
		case DATACELL_OPERATOR_MINUS:
		{
			if (left.offset == T0) {
				left.offset = T2;
				left.val++;
			}
			if (right.offset == T2) {
				right.offset = T0;
				right.val--;
			}
				
			if (left.offset == T2 && right.offset == T1) {
				value.offset = T1;
			} else if (left.offset == T1 && right.offset == T0) {
				value.offset = T1;
			} else if (left.offset == T1 && right.offset == T1) {
				value.offset = T0;
			} else {
				value.offset = T2;
			}
			value.val = left.val - right.val;
			break;
		}
		case DATACELL_OPERATOR_TIMES:
		{
			if (left.offset == T2) {
				left.offset = T0;
				left.val--;
			}
			if (right.offset == T2) {
				right.offset = T0;
				right.val--;
			}
			
			if ((left.offset == T1) || (right.offset == T1)) {
				fprintf(stderr,"Error: Multiplication operator (*) with operand in C1-region not defined.\n");
				return value;
			}
			value.offset = T0;
			value.val = left.val * right.val;
			break;
		}
		case DATACELL_OPERATOR_DIVIDE:
		{
			if (left.offset == T2) {
				left.offset = T0;
				left.val--;
			}
			if (right.offset == T2) {
				right.offset = T0;
				right.val--;
			}
			if ((left.offset == T1) || (right.offset == T1)) {
				fprintf(stderr,"Error: Division operator (/) with operand in C1-region not defined.\n");
				return value;
			}
			value.offset = T0;
			value.val = left.val / right.val;
			break;
		}
		case DATACELL_OPERATOR_CRAZY:
			value = crazy(left, right);
			break;
		case DATACELL_OPERATOR_ROTATE_L:
		{
			int warning_printed = 0;
			if (right.offset == T1) {
				fprintf(stderr,"Error: Rotation operator (<<) with right operand in C1-region not defined.\n");
				return value;
			}
			if (right.offset == T2) {
				right.offset = T0;
				right.val--;
			}
			value.offset = left.offset;
			value.val = left.val;
			while (right.val>0) {
				value.val *= 3;
				right.val--;
			}
			while (right.val<0) {
				if (value.val%3 == 0 && value.offset == T2) {
					value.offset = T0;
					value.val--;
				} else if (value.val%3 == 2 && value.offset == T0) {
					value.offset = T2;
					value.val++;
				}
				if (!warning_printed &&
						((value.val%3 == 0 && value.offset != T0) ||
						(value.val%3 == 1 && value.offset != T1) ||
						(value.val%3 == 2 && value.offset != T2)) )
				{
					fprintf(stderr,"Error: Rotation operator (<<) with negative right operand replaced by shift operator.\n");
					warning_printed = 1;
				}
				value.val /= 3;
				right.val++;
			}
			break;
		}
		case DATACELL_OPERATOR_ROTATE_R:
		{
			int warning_printed = 0;
			if (right.offset == T1) {
				fprintf(stderr,"Error: Rotation operator (<<) with right operand in C1-region not defined.\n");
				return value;
			}
			if (right.offset == T2) {
				right.offset = T0;
				right.val--;
			}
			value.offset = left.offset;
			value.val = left.val;
			while (right.val<0) {
				value.val *= 3;
				right.val++;
			}
			while (right.val>0) {
				if (value.val%3 == 0 && value.offset == T2) {
					value.offset = T0;
					value.val--;
				} else if (value.val%3 == 2 && value.offset == T0) {
					value.offset = T2;
					value.val++;
				}
				if (!warning_printed &&
						((value.val%3 == 0 && value.offset != T0) ||
						(value.val%3 == 1 && value.offset != T1) ||
						(value.val%3 == 2 && value.offset != T2)) )
				{
					fprintf(stderr,"Error: Rotation operator (>>) replaced by shift operator.\n");
					warning_printed = 1;
				}
				value.val /= 3;
				right.val--;
			}
			break;
		}
		default:
			return value; /* ERROR */
	}
	/* normalize value */
	if (value.val < 0 && value.offset == T0) {
		value.offset = T2;
		value.val++;
	} else if (value.val > 0 && value.offset == T2) {
		value.offset = T0;
		value.val--;
	}
	return value;
}


int generate_opcodes_from_memory_layout(MemoryList memory_layout, LabelTree* labeltree){
	MemoryListElement* it = memory_layout.first;
	/* save all offsets. */
	while (it != 0){
		it->opcode.offset = UNSET;
		it->opcode.val = -1;
		if (it->cell == 0) {
			it = it->next;
			continue;
		}
		if (it->cell->usage == CODE || it->cell->usage == PREINITIALIZED_CODE) {
			char symbol;
			if (!is_xlatcycle_existant(it->cell->code->command, it->address, &symbol)){
				fprintf(stderr,"Internal error: Cannot find xlat2 cycle again.\n");
				return 0; /*ERROR! */
			}
			it->opcode.offset = T0;
			it->opcode.val = (unsigned char)symbol;
		}else if (it->cell->usage == DATA) {
			Number data = parse_datacell(it->cell->data->data, labeltree);
			if (data.offset == T0 && data.val < 0){
					fprintf(stderr,"Internal error: Found negative value in data segment.\n");
					return 0; /*ERROR! */
			}
			it->opcode.offset = data.offset;
			it->opcode.val = data.val;
		}else if (it->cell->usage == RESERVED_CODE) {
			it->opcode.offset = T0;
			it->opcode.val = 81;
		}
		it = it->next;
	}
	return 1;
}
