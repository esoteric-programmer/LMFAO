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

#ifndef INITIALIZEINCLUDED
#define INITIALIZEINCLUDED

#include "typedefs.h"

/**
 * This function sets the offsets stored in the codeblocks and datablocks to fit to the given memory layout.
 *
 * \param memory LinkedList of GeneralMemoryCells corresponding to current memory layout. The offset of each codeblock and datablock referenced by the memory cells will be updated.
 */
void update_offsets(MemoryList memory);

/**
 * \brief Generates opcodes for memory cells of the final initialized Malbolge program.
 *
 * After that step the initialization code can be generated to get the final Malbolge code.
 *
 * \param memory_layout Memory layout that should be converted into opcodes. The size of this array must be C2+1=59049.
 * \param last_preinitialized Address of last memory cell that will be initialized directly after the Malbolge interpreter read in the source file.
 * \param opcodes The generated opcodes will be written into this array. If no opcode is generated for a memory cell, it will be set to -1. The size of this array must be C2+1=59049.
 * \param no_error_printing If set to non-zero, error messages won't be printed to stdout.
 * \return If an error occurs, zero will be returned. Otherwise, a non-zero value will be returned.
 */
int generate_opcodes_from_memory_layout(MemoryList memory_layout, LabelTree* labeltree);

#endif

