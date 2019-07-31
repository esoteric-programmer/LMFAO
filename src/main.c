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

/*
 * Debuginformationen:
 *
 * Labeltree+Zieladressen
 * Sourcecode-Verweise für Malbolge-Zellen
 * Entrypoint bzw. Anzahl der Operationen bis zum Erreichen des Selbigen
 *
 * Ggf. MD5-Hash / CRC-Hash der Malbolge-Datei sowie ggf. auch der Source-Code-Datei...
 * Ggf. Filenames von Source-Code und Malbolge-Code...
 *
 * => -d-flag erzeugt automatisch .dbg-file, passend zu .mb-file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "typedefs.h"
#include "initialize.h"
#include "malbolge.h"
#include "main.h"
#include "highlevel_initializer.h"

/**
 * File extension that is added to the input file if no output file name is given as command line argument.
 */
#define MALBOLGE_FILE_EXTENSION "mu"
#define MALBOLGE_DEBUG_FILE_EXTENSION "dbg"


#define COPY_CODE_POSITION(to, from_start, from_end) {(to).first_line = (from_start).first_line;(to).first_column = (from_start).first_column;(to).last_line = (from_end).last_line;(to).last_column = (from_end).last_column;};


LabelTree* labeltree;
DataBlocks datablocks;
CodeBlocks codeblocks;

/**
 *  Bison-generated parser function.
 *
 *  \return TODO
 */
int yyparse();

/**
 * Determines if a given Malbolge command will be executed as a NOP.
 *
 * \param command Malbolge command.
 * \return Non-zero if the Malbolge command is equivalent to a NOP instruction. Zero otherwise.
 */
int is_nop(int command);


/**
 * This function checks if R_ prefixes are only used with identifiers that point to the code section.
 * It substitutes U_ prefixes by not-prefixed labels and the corresponding offset adjustments, also.
 *
 * \param datablocks TODO
 * \param codeblocks TODO
 * \param labeltree TODO
 * \return TODO
 */
int hande_U_and_R_prefixes(DataBlocks* datablocks, CodeBlocks* codeblocks, LabelTree* labeltree);

/**
 * Adds a CodeBlock at a relative offset to the memory layout. The usage field of the affected memory cells will be set to code, preinitialized code, or reserved code. This function chooses the lowest possible offset for the CodeBlock.
 * \param block The CodeBlock that should be added.
 * \param memory_layout The memory layout the CodeBlock should be added to.
 * \param possible_positions_mod_94 An array of size 94 indicating valid start positions for the given CodeBlock modulo 94. Valid start positions are indicated by non-zero entries. Invalid start positions are indicated by zero entries.
 * \param must_be_initialized Indicates whether the given CodeBlock must be initialized or can be written directly into the preinitialized memory region. This will be written into the usage field of the affected memory cells.
 * \return A non-zero value is returned in the case that this function succeeds. Otherwise, zero is returned.
 */
int add_codeblock_to_memory_layout(CodeBlock* block, MemoryList* memory_layout, unsigned char possible_positions_mod_94[]);

/**
 * Adds a DataBlock at a relative offset to the memory layout. The usage field of the affected memory cells will be set to data, or reserved data. This function chooses the lowest possible offset for the DataBlock.
 * \param block The DataBlock that should be added.
 * \param memory_layout The memory layout the DataBlock should be added to.
 * \return A non-zero value is returned in the case that this function succeeds. Otherwise, zero is returned.
 */
int add_datablock_to_memory_layout(DataBlock* block, MemoryList* memory_layout);

/**
 * Prints a usage message to stdin.
 *
 * \param executable_name Command used to run this program. If set to NULL, "./lmfao" will be printed instead.
 */
void print_usage_message(char* executable_name) {
	printf("Usage: %s [options] file\n",executable_name!=0?executable_name:"./lmfao");
	printf("Options:\n");
	printf("  -o <file>           Write the output into <file>\n");
//	printf("  -f                  Fast mode (bigger output)\n");
	printf("  -l <number>         Insert a line break every <number> characters\n");
//	printf("  -d                  Write debug information\n");
}

/**
 * Parses command line arguments.
 *
 * \param argc Number of command line arguments.
 * \param argv Array containing argc command line arguments.
 * \param line_length The number of characters per line in the output file will be written here.
 *                    If no valid value is given at the command line line_length will be set to 0.
 * \param output_filename The name of the output file will be written here. Therefore new memory is allocated and should be freed later.
 *                        If no output file name is given at the command line the file extension of input_file is replaced by MALBOLGE_FILE_EXTENSION.
 * \param input_filename The name of the input file. The pointer is copied from the argv array.
 * \return A non-zero value will be returned in case of success. Otherwise, zero will be returned.
 */
int parse_input_args(int argc, char** argv, int* line_length, char** output_filename, const char** input_filename, char** debug_filename) {
	int i;
	if (argc<2 || argv == 0 || line_length == 0 || output_filename == 0 || input_filename == 0 || debug_filename == 0) {
		return 0;
	}
	*line_length = -1;
	*output_filename = 0;
	*input_filename = 0;
	for (i=1;i<argc;i++) {
		if (argv[i][0] == '-') {
			/* read parameter */
			switch (argv[i][1]) {
				long int tmp; 
				case 'l':
					i++;
					if (*line_length != -1) {
						return 0; /* double parameter: -l */
					}
					if (i>=argc) {
						return 0; /* missing argument for parameter: -l */
					}
					tmp = strtol(argv[i], 0, 10);
					if (tmp > C2+1)
						tmp = C2+1;
					if (tmp < 0)
						tmp = 0;
					*line_length = (int)tmp;
					break;

				case 'o':
					i++;
					if (*output_filename != 0) {
						return 0; /* double parameter: -o */
					}
					if (i>=argc) {
						return 0; /* missing argument for parameter: -o */
					}
					*output_filename = (char*)malloc(strlen(argv[i])+1);
					memcpy(*output_filename,argv[i],strlen(argv[i])+1);
					break;
				default:
					return 0; /* unknown parameter */
			}
		}else{
			/* read input file name */
			if (*input_filename != 0) {
				return 0; /* more than one input file given */
			}
			*input_filename = argv[i];
		}
	}
	if (*input_filename == 0) {
		return 0; /* no input file name given */
	}
	if (*line_length == -1)
		*line_length = 0;
	if (*output_filename == 0) {
		char* file_extension;
		size_t input_file_name_length;
		/* get file extension and overwrite it - or append it. */
		file_extension = strrchr((char*)*input_filename,'.');
		if (file_extension == 0 || strrchr(*input_filename,'\\')>file_extension || strrchr(*input_filename,'/')>file_extension) {
			input_file_name_length = strlen(*input_filename);
		}else{
			if (strcmp(file_extension+1,MALBOLGE_FILE_EXTENSION)==0) {
				input_file_name_length = strlen(*input_filename);
			}else{
				input_file_name_length = file_extension - *input_filename;
			}
		}
		/* add extension MALBOLGE_FILE_EXTENSION to file name. */
		*output_filename = (char*)malloc(input_file_name_length+1+strlen(MALBOLGE_FILE_EXTENSION)+1);
		memcpy(*output_filename,*input_filename,input_file_name_length);
		(*output_filename)[input_file_name_length] = '.';
		memcpy(*output_filename+input_file_name_length+1,MALBOLGE_FILE_EXTENSION,strlen(MALBOLGE_FILE_EXTENSION)+1);
	}
	/*
	if (debug_mode) {
		char* file_extension;
		size_t output_file_name_length;
		/ * get file extension and overwrite it - or append it. * /
		file_extension = strrchr(*output_filename,'.');
		if (file_extension == 0 || strrchr(*output_filename,'\\')>file_extension || strrchr(*output_filename,'/')>file_extension) {
			output_file_name_length = strlen(*output_filename);
		}else{
			if (strcmp(file_extension+1,MALBOLGE_DEBUG_FILE_EXTENSION)==0) {
				output_file_name_length = strlen(*output_filename);
			}else{
				output_file_name_length = file_extension - *output_filename;
			}
		}
		/ * add extension MALBOLGE_FILE_EXTENSION to file name. * /
		*debug_filename = (char*)malloc(output_file_name_length+1+strlen(MALBOLGE_DEBUG_FILE_EXTENSION)+1);
		memcpy(*debug_filename,*output_filename,output_file_name_length);
		(*debug_filename)[output_file_name_length] = '.';
		memcpy(*debug_filename+output_file_name_length+1,MALBOLGE_DEBUG_FILE_EXTENSION,strlen(MALBOLGE_DEBUG_FILE_EXTENSION)+1);
	}
	*/
	return 1; /* success */
}

/**
 * Main routine of LMAO.
 * Parses command line arguments, reads and parses input file, generates Malbolge code from HeLL program, and writes generated Malbolge code to output file.
 *
 * \param argc Number of command line arguments.
 * \param argv Array with command line arguments.
 * \return Zero will be returned in case of success. Otherwise, a non-zero error code will be returned.
 */
int main(int argc, char **argv) {
	int line_length=0;
	char* output_filename = 0;
	char* debug_filename = 0;
	const char* input_filename = 0;

	/* Save final memory layout here (this will be the same as stored in the dump file). */
	MemoryList memory_layout = {0, 0};

	DataBlock* entrypoint;

	unsigned int i; /* loop variable */
	int tmp; /* temporary variable */
	FILE* outputfile;

	printf("LMFAO v0.1.5 (Look, Malbolge Unshackled From Assembly, Ooh!) by Matthias Lutter.\n");

	if (!parse_input_args(argc, argv, &line_length, &output_filename, &input_filename, &debug_filename)){
		print_usage_message(argc>0?argv[0]:0);
		return 0;
	}

	if (freopen(input_filename, "r", stdin) == NULL) {
		fprintf(stderr,"Error: Cannot open file %s\n", input_filename);
		return 1;
	}

	labeltree = 0;
	datablocks.datafield = 0;
	datablocks.size = 0;
	codeblocks.codefield = 0;
	codeblocks.size = 0;
	entrypoint = 0;

	/* Parse input; exit on error. */
	tmp = yyparse();
	if (tmp) {
		return tmp;
 	}

	/* search entry point */
	if (!get_label(NULL, &entrypoint, "ENTRY", labeltree)){
		fprintf(stderr,"Error: Cannot find entry point (label ENTRY) in data section.\n");
		return 1;
	}
	if (entrypoint == 0) {
		fprintf(stderr,"Error: Internal error while looking for entry point.\n");
		return 1;
	}


	if (!hande_U_and_R_prefixes(&datablocks, &codeblocks, labeltree)) {
		/* TODO: print error message?? */
		return 1;
	}


	/* find positions for code blocks.
	 * TODO: maybe source out this for-loop into a function */
	int fxofs;
	for (fxofs=0;fxofs<2;fxofs++) { // two runs: one with fixed offsets (initialize these first), one without fixed offsets
	for (i=0;i<codeblocks.size;i++){
		CodeBlock* current_block = codeblocks.codefield[i];
		unsigned char possible_positions[94];
		long long j;
		long long pos = 0;
		int has_fixed_offset = (codeblocks.codefield[i]->offset.offset != UNSET);
		if ((fxofs==0 && !has_fixed_offset) || (fxofs==1 && has_fixed_offset)) {
			// initialize later
			continue;
		}
		for (j=0;j<94;j++) {
			possible_positions[j] = 1;
		}

		if (codeblocks.codefield[i]->num_of_blocks < 1)
			continue;

		/* treat every xlat-cycle in the current code block */
		while (current_block != 0){
			int positionsLeft = 0;
			XlatCycle* current_command = current_block->command;

			/* check for every position that is still possible if the current xlat-cycle is allowed there. remove not-allowed positions.
			 * break with error if no position is allowed any more. */
			for (j=0;j<94;j++){
				Number position;
				position.offset = T0;
				position.val = j + pos;
				char symbol = 0;
				if (possible_positions[j] != 0 && !is_xlatcycle_existant(current_command, position, &symbol)) {
					possible_positions[j] = 0;
				}
				if (possible_positions[j] != 0) {
					positionsLeft = 1;
				}
			}
			pos++;

			if (!positionsLeft){
				fprintf(stderr,"Error: Forced xlat cycle doesn't exist at line %d column %d.\n", current_block->code_position.first_line, current_block->code_position.first_column);
				return 1;
			}

			/* get next xlat-cycle in code block */
			current_block = current_block->next;

		}

		/* put current code block into appropriate memory-layout-array! */
		if (!add_codeblock_to_memory_layout(codeblocks.codefield[i], &memory_layout, possible_positions)) {
			fprintf(stderr,"Error: Overlapping or invalid offsets in code section.\n");
			return 1;
		}

	}


	/* TODO: maybe source out this for-loop into a function */
	for (i=0;i<datablocks.size;i++){
		int has_fixed_offset = (datablocks.datafield[i]->offset.offset != UNSET);
		if ((fxofs==0 && !has_fixed_offset) || (fxofs==1 && has_fixed_offset)) {
			// initialize later
			continue;
		}
		if (datablocks.datafield[i]->num_of_blocks < 1)
			continue;
		/* put current code block into appropriate memory-layout-array! */
		if (!add_datablock_to_memory_layout(datablocks.datafield[i], &memory_layout)) {
			fprintf(stderr,"Error: Overlapping offsets in data section or between code and data section.\n");
			return 1;
		}

	}
	}


	update_offsets(memory_layout);
	if (generate_opcodes_from_memory_layout(memory_layout, labeltree)==0) {
		fprintf(stderr,"Error: Cannot generate opcode from symbolic values.\n");
		return 1;
	}

	outputfile = fopen(output_filename, "wt");
	if (outputfile == 0) {
		fprintf(stderr,"Error: Cannot write to file %s\n", output_filename);
		return 1;
	}
/*
	if (line_length > 0 && line_length < size){
		int remaining = size;
		while(remaining>0) {
			fwrite(use_program+size-remaining,1,(remaining>line_length?line_length:remaining),outputfile);
			fwrite("\n",1,1,outputfile);
			remaining-=line_length;
		}
	}else{
	*/
	
//*
	if (generate_malbolge(memory_layout, entrypoint->offset, outputfile, line_length>0?(size_t)line_length:0)!=0) {
		fprintf(stderr,"Error: Cannot write Malbolge Unshackled code to file.\n");
	}
//*/
/*	
	write_number(entrypoint->offset,outputfile);
	fwrite("\n",1,1,outputfile);
	MemoryListElement* it = memory_layout.first;
	while (it != 0) {
		if (it->opcode.offset == T0 || it->opcode.offset == T1 || it->opcode.offset == T2) {
			write_number(it->address,outputfile);
			fwrite(":",1,1,outputfile);
			write_number(it->opcode,outputfile);
			fwrite("\n",1,1,outputfile);
		} else if (it->opcode.offset != UNSET) {
			fprintf(stderr,"warning: invalid offset value in ternary number.\n");
		}
		it = it->next;
	}
	//*/
	fclose(outputfile);
	printf("Malbolge Unshackled code written to %s\n", output_filename);


	/*
	if (debug_filename != 0) {
		FILE* debugfile = fopen(debug_filename, "wt");
		if (debugfile == 0) {
			fprintf(stderr,"Error: Cannot write debugging information to file %s\n", debug_filename);
			return 1;
		}
		fprintf(debugfile,":LABELS:\n");
		print_labeltree(debugfile, labeltree);
		fprintf(debugfile,":SOURCEPOSITIONS:\n");
		print_source_positions(debugfile, memory_layout);
		fprintf(debugfile,":EXECUTION_STEPS_UNTIL_ENTRY_POINT:\n");
		fprintf(debugfile,"%d\n", execution_steps_until_entry_point);
		fprintf(debugfile,":SOURCE_FILE:\n");
		fprintf(debugfile,"%s\n",input_filename);
		fprintf(debugfile,":MALBOLGE_FILE:\n");
		fprintf(debugfile,"%s\n",output_filename);
		fclose(debugfile);
		printf("Debugging information written to %s\n", debug_filename);
	}*/

	return 0;
}

void write_number(Number number, FILE* file) {
	long long max_value_C1_rot_width = 1;
	unsigned int width = 1;
	unsigned int i;
	long long nr;
	long long first_digit_factor = 1;

	if (!file) {
		return;
	}

	if (number.offset == T0 && number.val < 0) {
		number.offset = T2;
		number.val++;
	}else if (number.offset == T2 && number.val > 0) {
		number.offset = T0;
		number.val--;
	}

	fwrite(number.offset==T2?"2":(number.offset==T1?"1":"0"),1,1,file);
	fwrite("t",1,1,file);


	while ((number.offset == T1 && (number.val > max_value_C1_rot_width || -number.val > max_value_C1_rot_width))
			|| (number.val > 2*max_value_C1_rot_width)
			|| (-number.val > 2*max_value_C1_rot_width)
			) {
		max_value_C1_rot_width *= 3;
		max_value_C1_rot_width += 1;
		first_digit_factor *= 3;
		width++;
	}
	
	nr = number.val;
	if (number.offset == T1) {
		nr += max_value_C1_rot_width;
	}else if (number.offset == T2) {
		nr += 2*max_value_C1_rot_width;
	}
		
	for (i=0;i<width;i++) {
		char write_symbol = '0' + (nr / first_digit_factor) % 3;
		fwrite(&write_symbol,1,1,file);
		first_digit_factor /= 3;
	}
}



int unused_datacell_crash_warning_displayed = 0;
int get_label(CodeBlock** destination_code, DataBlock** destination_data, const char* label, LabelTree* labeltree){

	if (labeltree == 0 || label == 0 || (destination_code == 0 && destination_data == 0))
		return 0; /* ERROR */

	while(labeltree != 0) {
		/* select sub tree */
		int cmp = strncmp(label,labeltree->label,101);
		if (cmp > 0){
			labeltree = labeltree->left;
		}else if (cmp < 0){
			labeltree = labeltree->right;
		}else{
			if ((labeltree->destination_code != 0) && (labeltree->destination_data != 0)) {
				return 0; /* ERROR: entry for code and data section in label tree */
			}
			if (labeltree->destination_code != 0) {
				if (destination_code == 0) {
					return 0; /* ERROR: label to data section expected during function call, but label to code section found. */
				}
				*destination_code = labeltree->destination_code;
				if (destination_data != 0) {
					*destination_data = 0;
				}
			}else if (labeltree->destination_data != 0) {
				if (destination_data == 0) {
					return 0; /* ERROR: label to code section expected during function call, but label to data section found. */
				}
				*destination_data = labeltree->destination_data;
				if (destination_code != 0) {
					*destination_code = 0;
				}
			}else{
				return 0; /* ERROR: found link to code AND data section for this label */
			}

			if (labeltree->destination_data != 0) {
				DataCell* block = labeltree->destination_data->data;
				if (block->_operator == DATACELL_OPERATOR_NOT_USED){
					if (unused_datacell_crash_warning_displayed == 0) {
						unused_datacell_crash_warning_displayed = 1;
						fprintf(stderr,"Warning: The label %s at line %d column %d points to an unused data cell.\n"
								"Labels pointing to unused data cells are not supported yet.\n"
								"This may cause a crash or faulty error messages.\n"
								"Further warnings of this type will be suppressed.\n",
								label, labeltree->code_position.first_line,
								labeltree->code_position.first_column);
					}
				}
			}

			return 1;
		}
	}

	return 0; /* Not found */
}


/*
void print_labeltree(FILE* destination, LabelTree* labeltree) {
	if (labeltree == 0 || destination == 0)
		return;

	if (labeltree->label != 0) {
		if ((labeltree->destination_code != 0) && (labeltree->destination_data != 0)) {
		} else if (labeltree->destination_code != 0) {
			fprintf(destination, "%s: CODE %d\n",labeltree->label,labeltree->destination_code->offset);
		} else if (labeltree->destination_data != 0) {
			fprintf(destination, "%s: DATA %d\n",labeltree->label,labeltree->destination_data->offset);
		}else{
		}
	}
	print_labeltree(destination, labeltree->left);
	print_labeltree(destination, labeltree->right);
}


void print_source_positions(FILE* destination, MemoryCell memory[C2+1]){
	int i;
	if (destination == 0)
		return;
	/ * save all offsets. * /
	for (i=0;i<C2+1;i++){
		if (memory[i].usage == CODE || memory[i].usage == PREINITIALIZED_CODE) {
			fprintf(destination,"%d: CODE %d:%d - %d:%d\n",i, memory[i].code->code_position.first_line, memory[i].code->code_position.first_column, memory[i].code->code_position.last_line, memory[i].code->code_position.last_column);
		} else if (memory[i].usage == DATA || memory[i].usage == RESERVED_DATA)
			fprintf(destination,"%d: DATA %d:%d - %d:%d\n",i, memory[i].data->code_position.first_line, memory[i].data->code_position.first_column, memory[i].data->code_position.last_line, memory[i].data->code_position.last_column);
	}
}
*/

int is_nop(int command){
	if (command != MALBOLGE_COMMAND_OPR
			&& command != MALBOLGE_COMMAND_MOVED
			&& command != MALBOLGE_COMMAND_ROT
			&& command != MALBOLGE_COMMAND_JMP
			&& command != MALBOLGE_COMMAND_OUT
			&& command != MALBOLGE_COMMAND_IN
			&& command != MALBOLGE_COMMAND_HALT)
		return 1;
	return 0;
}


int is_xlatcycle_existant(XlatCycle* cycle, Number pos, char* start_symbol){
	/* find first NON-NOP in cycle, count NOPs until it.
	 * append NOPs before NON-NOP virtually at the end; start with NON-NOP
	 * set character to the NON-OP-character, then check if xlat2 does exactly what it should.
	 *
	 * if an xlat2 cycle is found AND startSymbol is not a NULL pointer, the ASCII character for the cycle will be written to *startSymbol. */

	unsigned int prefixed_nops = 0;
	unsigned char first_non_nop_char_in_cycle;
	unsigned char current_char;
	unsigned int i; /* loop variable */
	int position = 0;

	char start_sym_tmp = 0;
	
	if (pos.offset == T0 && pos.val < 0) {
		pos.offset = T2;
		pos.val++;
	} else if (pos.offset == T2 && pos.val > 0) {
		pos.offset = T0;
		pos.val--;
	}
	
	switch (pos.offset) {
		case T2:
			position = pos.val + 16;
			break;
		case T1:
			position = pos.val + 8;
			break;
		default:
			position = pos.val;
			break;
	}

	if (position >= 0) {
		position = position % 94;
	} else {
		position = (94 - ((-position) % 94)) % 94;
	}

	if (cycle == 0)
		return 0; /* invalid argument */
	if (position >= 94)
		return 0; /* invalid argument */

	if (cycle->next == 0) {
		/* no xlat cycle given -> non-loop-resistant command. */
		if (start_symbol != 0) {
			*start_symbol = (char)((cycle->cmd+94-position)%94);
			if (*start_symbol < 33)
				*start_symbol += 94;
		}
		return 1;
	}

	while (cycle->cmd == MALBOLGE_COMMAND_NOP && cycle->next != 0 && cycle->next != cycle){
		cycle = cycle->next;
		prefixed_nops++;
	}

	if (cycle->cmd == MALBOLGE_COMMAND_NOP) {
		/* is a RNop -> Lou Scheffer showed that immutable NOPs exists for every position.
		 * one immutable nop for each position is stored in immutable_nops.
		 * but there may be a better NOP that can be initialized easier.
		 * So this may be optimized in the future. */
		if (start_symbol != 0) {
			const char* const immutable_nops = "FFFFFF>><<::FFFFF3FFFFFF***)))FFFFFFF}FFFFxxFFFrroooFFFFFFF**FF**FFFFFFFFFFFFFFFFPPF**LJJFFFFF";
			*start_symbol = immutable_nops[position];
		}
		return 1;
	}

	/* calc character; store it. */
	first_non_nop_char_in_cycle = (unsigned char)((cycle->cmd+94-position)%94);
	if (first_non_nop_char_in_cycle < 33)
		first_non_nop_char_in_cycle += 94;
	current_char = first_non_nop_char_in_cycle;

	/* while cycle->next: xlat; cmp */
	while (cycle->next != 0){
		unsigned char cur_cmd;
		cycle = cycle->next;
		current_char = (unsigned char)(XLAT2[(int)current_char-33]);
		cur_cmd = (unsigned char)((current_char + position)%94);
		/* TEST: cur_cmd == cycle->cmd */
		if (is_nop(cur_cmd) != is_nop(cycle->cmd))
			return 0;
		if (!is_nop(cur_cmd))
			if (cur_cmd != cycle->cmd)
				return 0;
	}

	/* if an xlat2 cycle exists, then this will be the first ASCII character of the cycle: */
	start_sym_tmp = XLAT2[current_char-33];

	/* for prefixed_nops: xlat; test for NOP */
	for (i=0;i<prefixed_nops;i++) {
		unsigned char curCmd;
		current_char = (unsigned char)XLAT2[current_char-33];
		curCmd = (unsigned char)((current_char + position)%94);
		/* TEST: curCmd is a NOP */
		if (!is_nop(curCmd))
			return 0;
	}

	/* compare with calculated character */
	if (first_non_nop_char_in_cycle != (unsigned char)XLAT2[current_char-33])
		return 0; /* different! */

	if (start_symbol != 0)
		*start_symbol = start_sym_tmp;

	/* cycle exists. */
	return 1;
}


int handle_U_and_R_prefix_for_data_atom(DataAtom* data, DataBlock* following_block, LabelTree* labeltree) {
	if (data->destination_label != 0){
		if (data->operand_label != 0 && data->number.offset == T0 && data->number.val == 0){
			/* find relative offset of operand_label and replace it by defining an offset. */
			DataBlock* iterator = following_block;
			DataBlock* destination = 0;
			CodeBlock* destination_c = 0;
			int offset_counter = 0;
			if (!get_label(0, &destination, data->operand_label, labeltree)){
				fprintf(stderr,"Error: Cannot find label %s in data section at line %d column %d.\n",data->operand_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			if (destination == 0) {
				fprintf(stderr,"Error: Internal error while looking for label %s at line %d column %d.\n",data->operand_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			while (iterator != 0 && iterator != destination) {
				iterator = iterator->next;
				offset_counter--;
			}
			if (iterator == 0){
				fprintf(stderr,"Error: Label %s used by U_ prefixed command is not in same data block at line %d column %d.\n",data->operand_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			data->number.offset = T0;
			data->number.val = offset_counter;
			if (data->number.val < 0) {
				data->number.offset = T2;
				data->number.val ++;
			}
			free((void*)data->operand_label);
			data->operand_label = 0;
			/* search data->destination_label and create the needed preceeding NOPs. */
			if (!get_label(&destination_c, 0, data->destination_label, labeltree)){
				fprintf(stderr,"Error: Cannot find label %s in code section at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			if (destination_c == 0) {
				fprintf(stderr,"Error: Internal error while looking for label %s at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			while (offset_counter < 0) {
				if (destination_c->prev != 0){
					XlatCycle* cycle;
					destination_c = destination_c->prev;
					/* check destination_c for immutable NOP! */
					cycle = destination_c->command;
					while (cycle->cmd == MALBOLGE_COMMAND_NOP && cycle->next != 0 && cycle->next != cycle){
						cycle = cycle->next;
					}

					if (cycle->cmd != MALBOLGE_COMMAND_NOP) {
						fprintf(stderr,"Error: Cannot construct nop chain for label %s at line %d column %d.\n", data->destination_label, data->code_position.first_line,data->code_position.first_column);
						fprintf(stderr,"Found non-nop command at line %d column %d.\n", cycle->code_position.first_line,cycle->code_position.first_column);
						return 0;
					}
				}else{
					/* create preceding immutable NOP! */
					CodeBlock* code;
					XlatCycle* cycle = (XlatCycle*)malloc(sizeof(XlatCycle));
					cycle->next = cycle;
					cycle->cmd = MALBOLGE_COMMAND_NOP;
					memcpy(&(cycle->code_position), &(destination_c->command->code_position), sizeof(HeLLCodePosition));
					code = (CodeBlock*)malloc(sizeof(CodeBlock));
					code->command = cycle;
					code->prev = 0;
					code->next = destination_c;
					destination_c->prev = code;
					code->num_of_blocks = destination_c->num_of_blocks+1;
					if (destination_c->offset.offset == UNSET) {
						code->offset.offset = UNSET;
					}else{
						code->offset.offset = destination_c->offset.offset;
						code->offset.val = destination_c->offset.val - 1;
						if (code->offset.offset == T0 && code->offset.val < 0) {
							code->offset.offset = T2;
							code->offset.val++;
						}
					}
					memcpy(&(code->code_position), &(cycle->code_position), sizeof(HeLLCodePosition));
					destination_c = code;
				}
				offset_counter++;
			}

		}else if (data->operand_label == 0 && data->number.offset == T0 && data->number.val == 1){
			/* R_ prefix: check that the destination of the JMP has a successor! */
			CodeBlock* destination = 0;
			if (!get_label(&destination, 0, data->destination_label, labeltree)){
				fprintf(stderr,"Error: Cannot find label %s in code block at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			if (destination == 0) {
				fprintf(stderr,"Error: Internal error while looking for label %s at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			if (destination->next == 0) {
				fprintf(stderr,"Error: Invalid use of R_ prefix for label %s at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
		}else if (data->operand_label == 0 && data->number.offset == T0 && data->number.val == 0){
			/* label without prefix => check if label exists */
			CodeBlock* destination_c = 0;
			DataBlock* destination_d = 0;
			if (!get_label(&destination_c, &destination_d, data->destination_label, labeltree)){
				fprintf(stderr,"Error: Cannot find label %s at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
		} /* else if (data->number > 1){
			/ * Cannot occur. -> ERROR! * /
			fprintf(stderr,"Internal error.\n");
			return 0;
		} */
	}
	return 1;
}

int hande_U_and_R_prefixes_recursive(DataCell* data, DataBlock* following_block, LabelTree* labeltree){
	if (data->_operator == DATACELL_OPERATOR_LEAF_ELEMENT) {
		/* handle leaf element */
		return handle_U_and_R_prefix_for_data_atom(data->leaf_element, following_block, labeltree);
	}
	if (data->left_element != 0)
		if (!hande_U_and_R_prefixes_recursive(data->left_element, following_block, labeltree))
			return 0;

	if (data->right_element != 0)
		if (!hande_U_and_R_prefixes_recursive(data->right_element, following_block, labeltree))
			return 0;
	return 1;
}


int hande_U_and_R_prefixes(DataBlocks* datablocks, CodeBlocks* codeblocks, LabelTree* labeltree){
	unsigned int i;

	if (datablocks == 0 || labeltree == 0)
		return 0;

	/* For all datablocks: Handle U_ prefix and check R_ prefix. */
	for (i=0;i<datablocks->size;i++){
		DataBlock* current_block = datablocks->datafield[i];
		if (datablocks->datafield[i]->num_of_blocks < 1)
			continue;
		/* Search for U_ prefixed DATA words and calculate offset for them */
		while (current_block != 0){
			if (!hande_U_and_R_prefixes_recursive(current_block->data, current_block->next, labeltree))
				return 0;
			current_block = current_block->next;
		}
	}
	/* iterate code section and check if prev pointer of first element is zero. If it is not zero, correct entry in codeblocks. */
	for (i=0;i<codeblocks->size;i++){
		CodeBlock* curblock = codeblocks->codefield[i];
		while (curblock->prev != 0){
			codeblocks->codefield[i] = (curblock = curblock->prev);
		}
	}

	/* successful */
	return 1;
}


int add_codeblock_to_memory_layout(CodeBlock* block, MemoryList* memory_layout, unsigned char possible_positions_mod_94[]) {
	if (block == 0 || memory_layout == 0 || possible_positions_mod_94 == 0) {
		return 0; /* invalid parameters */
	}
	
	Number offset;
	MemoryListElement* pred = memory_layout->last;
	if (block->offset.offset != UNSET){
		/* insert reserved space at the front of this block! */
		
		// TODO: test whether this is a possible_position mod 94!
		
		if ((block->offset.offset == T0 && block->offset.val <= 1) || (block->offset.offset == T2 && block->offset.val <= 2)) {
			// ERROR: position offset position not initializable
			fprintf(stderr,"Error: Code cannot be initialized at given offset.\n");
			return 0;
		}
		if (block->offset.offset == T2) {
			block->offset.offset = T1;
			block->offset.val--;
		}
		
		offset.offset = block->offset.offset;
		offset.val = block->offset.val - 1;
		
		#define NUMBER_SMALLER(a,b) \
			(((a).offset == T1 && (b).offset == T1 && (a).val < (b).val) \
			|| \
			((a).offset == T1 && (b).offset == T2 && (b).val <= 0) \
			|| \
			((a).offset == T1 && (b).offset == T0 && (b).val < 0) \
			|| \
			((a).offset == T0 && (a).val >= 0 && (b).offset == T1) \
			|| \
			((a).offset == T2 && (a).val > 0 && (b).offset == T1) \
			|| \
			((a).offset == T0 && (a).val < (b).val && ((a).val<0)==((b).val<0) && (b).offset == T0) \
			|| \
			((a).offset == T0 && (a).val>=0 && (b).val<0 && (b).offset == T0) \
			|| \
			((a).offset == T2 && (a).val <= (b).val && ((a).val<=0)==((b).val<0) && (b).offset == T0) \
			|| \
			((a).offset == T2 && (a).val>0 && (b).val<0 && (b).offset == T0) \
			|| \
			((a).offset == T0 && (a).val <= (b).val && ((a).val<0)==((b).val<=0) && (b).offset == T2) \
			|| \
			((a).offset == T0 && (a).val>=0 && (b).val<=0 && (b).offset == T2) \
			|| \
			((a).offset == T2 && (a).val < (b).val && ((a).val<=0)==((b).val<=0) && (b).offset == T2) \
			|| \
			((a).offset == T2 && (a).val>0 && (b).val<=0 && (b).offset == T2))
		
		// find predecessor in memory_layout
		pred = memory_layout->last;
		while (pred!=0) {
			if (NUMBER_SMALLER(pred->address, offset)) {
				break;
			}
			pred = pred->next;
		}
	}else {
		int i;
		int match = 0;
		if (pred != 0) {
			offset.offset = pred->address.offset;
			offset.val = pred->address.val + 1;
		}else{
			offset.offset = UNSET;
		}
		if (offset.offset == T2 && offset.val <= 0) {
			return 0; // cannot insert...
		}
		if (offset.offset != T1 || offset.val < 41) {
			offset.offset = T1;
			offset.val = 41;
		}
		for (i=0;i<94;i++) {
			if (possible_positions_mod_94[(offset.val + 8 + i + 1)%94]) {
				offset.val += i;
				match = 1;
				break;
			}
		}
		if (!match) {
			return 0; // no possible position given
		}
	}
	// create new MemoryCell!!
	MemoryCell* reserved = (MemoryCell*)malloc(sizeof(MemoryCell));
	
	reserved->usage = RESERVED_CODE;
	COPY_CODE_POSITION(reserved->code_position, block->code_position, block->code_position);
	reserved->code_position.last_line = reserved->code_position.first_line;
	reserved->code_position.last_column = reserved->code_position.first_column;
	MemoryListElement* elem = (MemoryListElement*)malloc(sizeof(MemoryListElement));
	elem->address.offset = offset.offset;
	elem->address.val = offset.val;
	elem->opcode.offset = UNSET;
	elem->opcode.val = -1;
	elem->cell = reserved;
	
	// INSERT!
	if (pred == 0) {
		MemoryListElement* tmp = memory_layout->first;
		memory_layout->first = elem;
		elem->prev = 0;
		elem->next = tmp;
		if (elem->next != 0) {
			elem->next->prev = elem;
		}
	} else {
		MemoryListElement* tmp = pred->next;
		pred->next = elem;
		elem->prev = pred;
		elem->next = tmp;
		if (elem->next != 0) {
			elem->next->prev = elem;
		}
	}
	if (elem->prev == memory_layout->last) {
		memory_layout->last = elem;
	}
	pred = elem;
	
	/* insert at given offset! */
	while (block != 0){
		offset.val++;
				
		// create new MemoryCell!!
		MemoryCell* insert = (MemoryCell*)malloc(sizeof(MemoryCell));
	
		insert->usage = CODE;
		COPY_CODE_POSITION(insert->code_position, block->code_position, block->code_position);
		insert->code = block;

		MemoryListElement* elem = (MemoryListElement*)malloc(sizeof(MemoryListElement));
		elem->address.offset = offset.offset;
		elem->address.val = offset.val;
		elem->opcode.offset = UNSET;
		elem->opcode.val = -1;
		elem->cell = insert;
	
		// INSERT!
		if (pred == 0) {
			MemoryListElement* tmp = memory_layout->first;
			memory_layout->first = elem;
			elem->prev = 0;
			elem->next = tmp;
			if (elem->next != 0) {
				elem->next->prev = elem;
			}
		} else {
			MemoryListElement* tmp = pred->next;
			pred->next = elem;
			elem->prev = pred;
			elem->next = tmp;
			if (elem->next != 0) {
				elem->next->prev = elem;
			}
		}
		if (elem->prev == memory_layout->last) {
			memory_layout->last = elem;
		}
		pred = elem;
		

		/* get next XlatCycle in code block */
		block = block->next;
	}
	
	if (pred->next != 0) {
		if (!NUMBER_SMALLER(pred->address, pred->next->address)) {
			return 0; // overlapping!!!
		}
	}

	return 1; /* successful */
}


int add_datablock_to_memory_layout(DataBlock* block, MemoryList* memory_layout){
	if (block == 0 || memory_layout == 0)
		return 0; /* invalid parameters */
		

	Number offset;
	MemoryListElement* pred = memory_layout->last;
	if (block->offset.offset != UNSET){
		if ((block->offset.offset == T0 && block->offset.val <= 1) || (block->offset.offset == T2 && block->offset.val <= 2)) {
			// ERROR: position offset position not initializable
			fprintf(stderr,"Error: Code cannot be initialized at given offset.\n");
			return 0;
		}
		if (block->offset.offset == T2) {
			block->offset.offset = T1;
			block->offset.val--;
		}
		
		offset.offset = block->offset.offset;
		offset.val = block->offset.val;

		// find predecessor in memory_layout
		pred = memory_layout->last;
		while (pred!=0) {
			if (NUMBER_SMALLER(pred->address, offset)) {
				break;
			}
			pred = pred->next;
		}
	}else {
		if (pred != 0) {
			offset.offset = pred->address.offset;
			offset.val = pred->address.val + 1;
		}else{
			offset.offset = UNSET;
		}
		if (offset.offset == T2 && offset.val <= 0) {
			return 0; // cannot insert...
		}
		if (offset.offset != T1 || offset.val < 41) {
			offset.offset = T1;
			offset.val = 41;
		}
	}
	/* insert at given offset! */
	while (block != 0){
					
		// create new MemoryCell!!
		MemoryCell* insert = 0;
		if (block->data->_operator != DATACELL_OPERATOR_NOT_USED) {
			insert = (MemoryCell*)malloc(sizeof(MemoryCell));
			if (block->data->_operator != DATACELL_OPERATOR_DONTCARE) {
				insert->usage = DATA;
			}else{
				insert->usage = RESERVED_DATA;
			}
			insert->data = block;
			COPY_CODE_POSITION(insert->code_position, block->code_position, block->code_position);
		}

		if (insert) {
			MemoryListElement* elem = (MemoryListElement*)malloc(sizeof(MemoryListElement));
			elem->address.offset = offset.offset;
			elem->address.val = offset.val;
			elem->opcode.offset = UNSET;
			elem->opcode.val = -1;
			elem->cell = insert;
		
			// INSERT!
			if (pred == 0) {
				MemoryListElement* tmp = memory_layout->first;
				memory_layout->first = elem;
				elem->prev = 0;
				elem->next = tmp;
				if (elem->next != 0) {
					elem->next->prev = elem;
				}
			} else {
				MemoryListElement* tmp = pred->next;
				pred->next = elem;
				elem->prev = pred;
				elem->next = tmp;
				if (elem->next != 0) {
					elem->next->prev = elem;
				}
			}
			if (elem->prev == memory_layout->last) {
				memory_layout->last = elem;
			}
			pred = elem;
		}
		offset.val++;
		/* get next XlatCycle in code block */
		block = block->next;
	}

	if (pred->next != 0) {
		if (!NUMBER_SMALLER(pred->address, pred->next->address)) {
			return 0; // overlapping!!!
		}
	}

	return 1; /* successful */
}
