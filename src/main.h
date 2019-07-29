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

#ifndef MAININCLUDED
#define MAININCLUDED

#include "typedefs.h"
#include <stdio.h>

/**
 * Searches DataBlock or CodeBlock referenced by a given string in the LabelTree.
 *
 * \param destination_code The pointer to the CodeBlock will be written here. If the label is defined in the data section, NULL will be written here.
 * If the label is expected to be defined in the data section, NULL can be passed for this parameter.
 * If this parameter is NULL and the label is defined in the code section, this function will fail.
 * \param destination_data The pointer to the DataBlock will be written here. If the label is defined in the code section, NULL will be written here.
 * If the label is expected to be defined in the code section, NULL can be passed for this parameter.
 * If this parameter is NULL and the label is defined in the data section, this function will fail.
 * \param label Label string that should be searched in the LabelTree.
 * \param labeltree LabelTree that should be searched.
 * \return One if the Block has been found. Zero otherwise.
 */
int get_label(CodeBlock** destination_code, DataBlock** destination_data, const char* label, LabelTree* labeltree);

/**
 * Gets a single xlat2 cycle and its position. Computates whether the given xlat2 cycle is possible at the given position or not.
 * If start_symbol is not null and the gevien xlat2 cycle exists, the first ASCII value of the xlat2 cycle will be written to *start_symbol.
 *
 * \param cycle Xlat2 cycle that should be checked for existence.
 * \param position Desired position of the xlat2 cycle at the Malbolge virtual machine memory.
 * \param start_symbol An ASCII character that will result in the given xlat2 cycle will be written here if such a cycle exists.
 *                     If this pointer is set to NULL, the ASCII character won't be written.
 * \return If the given xlat2 cycle exists at the given position, this funciton returns a non-zero value. Otherwise zero is returned.
 */
int is_xlatcycle_existant(XlatCycle* cycle, Number position, char* start_symbol);

/* debug-informations */
/*
void print_labeltree(FILE* destination, LabelTree* labeltree);
void print_source_positions(FILE* destination, MemoryCell memory[C2+1]);
*/

void write_number(Number number, FILE* file);

#endif

