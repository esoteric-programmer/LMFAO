#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "typedefs.h"
#include "highlevel_initializer.h"

/**
 * Input: memory layout file
 * Output: Malbolge Unshackled program
 *
 * Input format:
 * File = Entry Program
 * Entry = Value NL
 * Program = Line NL Program | Line NL?
 * Line = Value: Value
 * Value = (0|1)t(0|1|2)* | (C|...)(0|1)(0|1|2)*
 * NL = \n
 *
 * Semantics:
 * Entry: Address of Entry point
 * Line:  Address: Value to be generated there
 *
 * Semicolon: begin comment
 */


typedef struct {
	Number addr;
	Number val;
} MemInit;


MemInit* generation;
long long generation_size;
Trit entry_offset;
long long entry;

unsigned int min_rot_width; /* of VM */
unsigned int needed_backjump_rotwidth; /*minimum */

const char* normalization = "+b(29e*j1VMEKLyC})8&m#~W>qxdRp0wkrUo[D7,XT"
		"cA\"lI.v%{gJh4G\\-=O@5`_3i<?Z';FNQuY]szf$!BS/|t:Pn6^Ha";

void denormalize(char* code, long long offset) {
	long long i;
	long long len = strlen(code);
	for (i=0;i<len;i++){
		if (code[i] >= 33 && code[i] < 127) {
			/* denormalize this char! */
			long long j;
			for (j=0;j<strlen(normalization);j++) {
				if (code[i] == normalization[(j+offset)%94]) {
					code[i] = j+33;
					break;
				}
			}
			offset++;
		}
	}
}

char* code;
long long current_code_size;
long long min_t0_cell;


void set_gen_init_03(long long value, unsigned int width, int omit_last_rot);
int initialize_memory_cell(MemInit cell, unsigned int rot_width_at_least, long long c_start_offset);
int initialize_entry_point(Trit entry_offset, long long entry, unsigned int rot_width_at_least, long long c_start_offset);
void add_code(const char* code);
Trit get_trit(unsigned int position, long long value);
long long set_trit(Trit trit, unsigned int position, long long value);
void init_all(long long c_offset);

void add_code(const char* cd) {
	int length = strlen(cd);
	code = (char*)realloc(code, current_code_size + length + 1);
	if (code == 0) {
		fprintf(stderr,"error: out of memory\n");
		exit(1);
	}
	memcpy(code+current_code_size,cd,length+1);
	current_code_size += length;
}

void change_0_1_trits(Number* data) {
	long long max_value_C1_rot_width = 1;
	unsigned int width = 1;
	while ((data->offset == T1 && (data->val > max_value_C1_rot_width || -data->val > max_value_C1_rot_width))
			|| (data->val > 2*max_value_C1_rot_width)
			) {
		max_value_C1_rot_width *= 3;
		max_value_C1_rot_width += 1;
		width++;
	}
	if (data->offset == T0) {
		unsigned int i;
		long long new_val = data->val;
		data->offset = T1;
		for (i=0;i<width;i++) {
			Trit cur = get_trit(i,new_val);
			new_val = set_trit(cur==T1?T0:(cur==T0?T1:cur),i,new_val);
		}
		data->val = new_val - max_value_C1_rot_width;
	}else if (data->offset == T1) {
		unsigned int i;
		long long new_val = max_value_C1_rot_width + data->val;
		data->offset = T0;
		for (i=0;i<width;i++) {
			Trit cur = get_trit(i,new_val);
			new_val = set_trit(cur==T1?T0:(cur==T0?T1:cur),i,new_val);
		}
		data->val = new_val;
	}else{
		/* not implemented (value cannot be generated anyway) */
		fprintf(stderr,"error: unconstructable value\n");
		exit(1);
	}
}


int generate_malbolge(MemoryList to_initialize, Number entry_point, FILE* output, size_t ll) {
	long long j;
	long long c = 19391; /* see Malbolge Unshackled code below (init_module) */
	const char* init_module;
	min_t0_cell = -1;
	MemoryListElement* it = to_initialize.first;

	generation_size = 0;
	generation = (MemInit*)malloc(sizeof(MemInit));
	if (generation == 0) {
		fprintf(stderr,"error: out of memory\n");
		return 1;
	}
	
	entry_offset = entry_point.offset;
	entry = entry_point.val;
	//printf("entry: C%d %s%d\n", entry_offset==T1?1:0, entry<0?"":"+", entry);

	while (it != 0) {
		if (it->address.offset != UNSET && it->opcode.offset != UNSET) {
			generation = (MemInit*)realloc(generation,(generation_size+1)*sizeof(MemInit));
			if (generation == 0) {
				fprintf(stderr,"error: out of memory\n");
				return 1;
			}
		
			if (it->address.offset == T0 && it->address.val < 0) {
				it->address.offset = T2;
				it->address.val++;
			}else if (it->address.offset == T2 && it->address.val > 0) {
				it->address.offset = T0;
				it->address.val--;
			}
			if (it->opcode.offset == T0 && it->opcode.val < 0) {
				it->opcode.offset = T2;
				it->opcode.val++;
			}else if (it->opcode.offset == T2 && it->opcode.val > 0) {
				it->opcode.offset = T0;
				it->opcode.val--;
			}
		
			memcpy(&(generation[generation_size].addr), &(it->address), sizeof(Number));
			memcpy(&(generation[generation_size].val),  &(it->opcode),  sizeof(Number));
			generation_size++;
		}
		it = it->next;
	}


	needed_backjump_rotwidth = 10;
	init_module =
		"bCBA@?>=<;:9876543210/.-,I*)(E~%$#\"RQ}={zyxwvutsrD0|nQl,"
		"+*)(f%dF\"a3_^]\\[ZYXWVUTSRQJmNMLKJIHGFEDCBA@?>=<;:987654"
		"3210/.-,+*)('&%$#dc~}|_^yrwZutsrqpinPlOjihgf_dcEaD_^]\\UZ"
		"YX:V9TSRQPONGLK.IHGF?DCB$@#>=<;4987w/v3210/.-,+$k('&%|#\""
		"!~w`{zyxwputslUponmlkjiKgf_Hcba`_^]\\[ZYXWP9TSRQPONMFKJCH"
		"A*EDCBA@?>=<|:98y6543210).-,%*#j'&%$#\"!~}|^ty\\wvutslkTo"
		"nmlkjcLafedc\\aD_X]\\[TY<WVOTSLQ4ONMFKDC,GFEDC<A$9>=<;49z"
		"765.3,+r/.-,l*)(!h%$#\"!~}|{zyxwvunWrqponmlkjihgfed]Fa`_^"
		"]\\[ZYXW9UTSRK4ONMLKJIHGFEDCBA@?8!<;:9876543210/.-,%l)('&"
		"%$#\"!~}|^t]xqvYnsrqpinQlejihafIdc\\a`Y^A\\[ZSXQ:8NSRQJ3N"
		"MLKJIHGFEDC%;$?8=<}49876543210qp',+*)('&%$#\"!b}`{zyrwpuX"
		"mrqpohmlOdihgfedcba`_^]\\?=S<WPU8SRQJOHM0EJIHG@ED'<A@?>=<"
		";:987654us+r/(-,m*#('&%$#\"!~a`{tyxwvutsrqpoRmPkjibg`eHcb"
		"[`_X]@UZYXWPUT7RKPONMLKJIHGFED'%;$?8=~;:387054u2+0/.-,+*)"
		"('&%$ecyb}v{z]xwputsrqponQPkjchgfedcba`_B]@[ZSXWPU8SLQPOH"
		"ML/JIBGFEDCBA@?>=<}{3z705v321*/(om%*)j'&}$#\"!~}|{zyxwZXn"
		"WrkpoRmlkdihgfedcFE`_^W\\[ZYXWVUT7R5PONGLEJ-HG@ED=B%@9>=<"
		"5:9z765.3210/.-,+*)jh~g${\"c~}v{zsxwZutslqponmlkjihgJH^Gb"
		"[`_B]\\[ZSXWVUTS65PONMFKJIHGFED'B%@?>7<5:9z7654-210/.-,+*"
		")jh~g${dbx}|_zyxwputsrqponmlOMcLg`edGba`_^W\\[ZYX;:UTSRQJ"
		"ONMLKJI,G*EDC<A:?\"=<5:927x5.321*/p',+*)\"'&g$#\"!~w|{zyx"
		"wvutWUkTohmlOjihgf_dcba`_^]\\?=S<WPUT7RQPONMLEJIH+*EDCBA@"
		"?8=<;:9z7x543,1*/.o,+*)('&}$#\"!~}|_]s\\wputWrqponmlejihg"
		"fedGE[D_X]\\?ZYXWVUTSLQP32MLKJIHGF?DCBA$?\"=<;4927x54-21*"
		"/p-&+*)\"'h}$#\"!x}|_zyxwvutslqponmlOMcLg`edGba`_^]\\[TYX"
		"WVUT75K4OHML/JIHGFEDCBA@9\"=~;:987654321*/p-n+*)\"'~%f#\""
		"y~}v{^sxwvunsrUponmlkjihgf_dcbECYB]V[>YXQVUNSR5PONMLKJIHG"
		"F?DCB%#9\"=6;:{876543210/.-&m*)j'&%$#\"!~}|{zs\\wZutslqjo"
		"RmfkjibgfIdcba`_^]\\[ZYRWV97M6QJ31GLK.IHGFEDCBA@?>7<;|z2y"
		"6/43t10/.-,+*)('&%|e\"!~a|{zyxwvutsrqpiRmlOjibgf_dGb[`_^W"
		"\\[>YXWVUTSRQPONMFK.,B+F?DC&A@?>=<;:98765.3tr*q.',+l)('&%"
		"$#\"!~}|{zs\\wvutWrqponmlkjihgfe^Gba`C^]\\UZSX;VUNSRKP3HM"
		"LKJCHG*EDCBA@?>=<;:981xv.u2+0/p-,+*)('&%$#\"!~w`^t]xqvuXs"
		"rqponmlkjihgfe^Gba`_^A\\[ZYXWVUTSRQPONG0KJIH+FED=B;@?\"=<"
		";:9876543210/(o,l$k(!hf|#\"c~}|{zyxwvutsrqpiRmOeNibgfIdcb"
		"a`_^]\\[ZYXWVUN7RQPONM0KJIHGFEDCBA@?>=<5|98765v3,10/(-n%*"
		")('~%$e\"!~}|{zyxwvutsrqjSnmOeNibgJe^cbaZ_^A\\[ZYXWVUTSRQ"
		"PONMF/JI+A*E>CB%@?>=<;:9876543210)p-,+*)('h%$#\"!~}|{zyxw"
		"vutslUponmlkNihg`e^cbE`_^]\\[ZYXWVUTSRQPI2MLK-C,G@E(CB;@?"
		"8=~;498705v-210/(om%*)j'&%$#\"!~}|{zyxwvunWrqpRhQlejiLgfe"
		"dcba`_^]\\[ZYXWVO87RQ4ONMLKJIHGFEDCBA@?>7~}:{876/4-2s0/(-"
		",%*k\"'&%${\"!b}|{zyxwvutsrqponmleNihgfH^Gb[`C^]V[ZSXW:UT"
		"SRQPONMLKJIHGFED=&A@?>~6}:387x543210/.-,+*)('&%$#zc~a|{^y"
		"xwvutsrqponmlkjihg`IdGbE`_X]\\UZ=XQVUTMRQ4ONMLKJIHGFEDCBA"
		"@?>=6}:9876v.u2+0q.-,%*#jh~%$e\"!~}|{zyxwvutsrqpongPkjihg"
		"I_Hc\\a`C^]\\?TY<WVUTSR5PO2MLK.CH+FEDCBA$?\"=<;4927x54-21"
		"*/p-&+*)\"'h}$#\"!x}|_zyx[puXsrqponmlkjLbKf_dGba`Y^W\\?ZS"
		"XWVOTS6QPO2GL/JIHGFEDCBA#9\"=6;:{876w4321*/.o,+*)('&%$#\""
		"c~}`{zy\\wvutmrqTonmlkjihgfeHcFa`_X]V[>YRWVUNS6KPONMFKJ-H"
		"GF)DCBA:?>!<;:9876543210/.n&m*#('h%$#d!~}|uzy\\wvutsrqpon"
		"mlkjiKaJe^cbED_XA\\[ZYXWVUTSR5PO21LE.IHGFEDCBA@?\"=~;:927"
		"05v32+0/(-n%*)('~%$ed!xa|{zyxwvutsrqponPfOjchgJId]Fa`_^]\\"
		"[ZYXWVUTS5K4OHML/.IHA*EDCBA@?>=<;:{87xw43,s0/.-,+*)('&%f"
		"#\"cb}|u^yxwvutsrqponmlkjLbKf_dcFE`_XA\\[ZYXWVUTSRQPONM/E"
		".IBGF)(CBA:#>=<;:98765432s0/po,+*#j'&%$#\"!~}|{zy\\wZunsr"
		"qjoRglkjibgfIHcbaZC^]\\[ZYXWVUTSRQPON0F/JCH+F?DCB;@?\"!<;"
		":3z76543210/.-,+*)('g}f#z!~a`{zyxqvutsrqponmlkjihKfeHGba`"
		"_X]\\[ZYXWVUTSRQPO2M0KJCHG@E(=BA@?8=<}|9876/43210/.-,+*)("
		"'&%$#\"bxa|uzy\\[vutslqponmlkjihgfedcba`_AW@[TYX;:UTSRQJO"
		"NMLKJIHGFEDCB%@?\"!<;:9816543210/.-,+*)j'h%$#z!x}`uzyxwpu"
		"tWVqponmfkjihgfedcba`_^]\\[Z<R;VOT7RQPINGL/DIHGF?DC&%@?>="
		"<5:9876543210/.-,+*)i!h%|#\"cb}|{zyxqvutsrqponmlkjMhgJIdc"
		"ba`_X]\\[ZYXWVUTSRQ4O2MLKDIBG*ED=BA:?\"7<;:9276wv3210/.',"
		"+*)('&%$#\"!~}|{z\\r[votWrqjonglkNMhgfedc\\a`_^]\\[ZYXWVU"
		"TSRQ3I2MFKJ-,GFEDCBA:?>=<;:987654u21rq.-,+*)(!&%$#\"!~}|{"
		"zy\\wZutmrqjoRmfkjibgfIHcba`_^]V[ZYXWVUTSRQPONML.D-HAF)DC"
		"B;@9\"~6;:{z7654321*/.-,+*)('&%$#\"!~`v_zsxwvutmVqponmlkj"
		"ihgJedcbaZC^]\\[ZYXWVUT7R5PONGLEJ-HG@ED=B%@9>=<5:{27654-2"
		"10/.'n+*)('&%$#\"!~}|{]s\\wpuXsrqjohmPkdihg`edcbaZC^]\\[Z"
		"YXWVUTSRQP2H1LEJIHGFEDCBA:#>=<;:98765432+r/.-,+*)('&%$#zc"
		"~`v_zs\\ZputsrqponmleNiKaJe^cba`_^]\\[ZYRWVUTSRQPONMLKJIH"
		"GFEDCBA@?>=<;:98y65432s0/.-,+*)('&}$#\"!~}|{zyxwvutsrqpon"
		"mlkjihgfedcFa`_^]@[>YXWPUNSRQPONMLKJIBGFEDCBA@?>=<;:98765"
		"43210/.-,+*)(i&%$#\"!~}|^t]xqvutsrqponmlejihgfedcba`_^]\\"
		"[ZYXWVUTSRQPONMLK.IHGFEDCBA#9\"=6;:9876543210/(-,+*)('&%$"
		"#\"!~}|{zyxwvutsrqponQlkjihKfedcba`_^]\\[ZSXWVUTSRQPONMLK"
		"JIHGFEDCBA@?>=<;|98765v3t10).-&+l)\"'&%|#dy~}|{tyxwvutsrq"
		"ponmfkjihgfedcba`_^]\\[ZYXWVUTSRQPON1LKJIHGFED&<%@9>!<;49"
		"816w4-210).-,+*)('&%$#\"y~}|{zyxwvutsrqponmlkjihgfedcbaD_"
		"^]\\[ZYXW9O8SLQPONMLKJIHGFED=BA@?>=<;:9876543210/.-,+*)('"
		"&g$#\"!~a|{zyxwvutsrqpohmlkjihgfedcba`_^]\\[ZYXWVUTSRQ4ON"
		"MLK.I,GFE>C<A$?8=<;49z16543,10/.-,+*)('&%${\"!~}|{zyxwvut"
		"srqponmlkjihgfedGba`_^]\\[Z<R;VOT7RQPINGL/JCHGF?DCBA@?>=<"
		";:9870543210/.-,+*)('&%$#\"!~}|{zyxwZutsrqponmOeNibgfedcb"
		"a`_^]\\[ZYRWVUTSRQPONMLKJIHGFEDCBA@?>=<}:9876w43210/.-,+*"
		")('&}$#\"!~}|{zyxwvutsrqponmlkjihgJedcbaD_^]\\[ZYXWVUTSRQ"
		"JONMLKJIHGFEDCBA@?>=<;:987654u210/.-,+*j\"i&}$e\"y~}|uz]r"
		"wvutmVTjonmlkjihgfedcbaZ_^]\\[ZYXWVUTSRQPONMLKJIHGFED'BA@"
		"?>=<;:z2y6/43210q(-,+*)('&%$#\"!~}|{zyxwvutsrqTonmlkNihgf"
		"eH]ba`_^]\\[ZYXWVUTSRQPONMLKJIH+FEDCB%@#>=6;:387654u,10/."
		"-,+*)('&%$#\"!~}|{zyxwvuXsrqponmlkMcLg`eHc\\a`_X]@UZYXWP9"
		"7MRQPON1FKJIHGFEDCBA@?>=<;:987654321r/.-,+*)('g}f#z!~}|{^"
		"yrwvutsrqponmlkjihgfedcba`_^A\\[ZYX;VUTSR5PINMLKJIHGFEDCB"
		"A@?>=<;:98765v3210/p-,+*)j'~%$#\"!~}|{zyxwvutsrqponmlkjMh"
		"gfedcba`BXA\\UZ=XWPUTMR5JONMLE.,BGFEDC&A:?>=<;:9876543210"
		"/.-,+*)('&g$#\"!~}|{z\\r[votsrqpSnmfkjihgfedcba`_^]\\[ZYX"
		"WVUTS6QPONM0KJIHG*ED=BA@?>=<;:9876543210/.-,+*k('&%$e\"c~"
		"}v{zsx[votsrkponmlOjibgfedcba`_^]\\[ZYXWVUTSRQPO2MLKJIHGF"
		"E'=&A:?\"=6;:9276543t10).-,+*)('&%$#\"!~}|{zyxwvutWrqponm"
		"lkjLbKf_dcba`C^]\\UZYXWVUTSRQPONMLKJIHGFEDC&A@?>=~;:987x5"
		"43,10/.-,+*)('&%$#\"!~}|{zyx[vutsrUponmlOjihafedcba`_^]\\"
		"[ZYXWVUTSRQPO2MLKJIHGFE'=&A:?\"=<;4927x5.321*/p',+*)\"ig}"
		"$#\"!~a|{zsxwvutsrqponmlkjihgfedcbaD_^]\\[ZYXW9O8SLQPONM0"
		"KJIHGFEDCB;@?>=<;:9876543210q.-,+*k('&%$e\"!~}|{zyxwputsr"
		"qponmlkjihgfeHcba`_B]@[ZYRWPU8SRKPOHM0KDIHG@E(=BA@?8=<;:9"
		"z76543210/.',+*)('&%$#\"!~}|{z]xwvutsrqpRhQlejMhg`ed]bE`Y"
		"^]\\UZ=RWVUTMRQPON1LKJIHGFEDC<A@?>=<;:987654321r/.-,+*)('"
		"g}f#z!~}|{^yxwvutsrUjonmlkjihgfedcbE`_^]\\?ZYXWV9TSRQPONM"
		"0EJIHGFEDCBA@?>=~;:987x5v321*/(-n+$)('~%f{\"!~}v{zyxwZuts"
		"rqponQfkjihgfedcba`_^A\\[ZYXWVUT6L5PIN1LKJCHAF)D=BA@9>=<;"
		":{87654321r).-,+*)('&%$#\"!b}|{zyxwvuWmVqjonmPeNihgfedcba"
		"`_^]\\[ZY<WVUTS6QPO2G0KJIHGFEDCBA@?>=<;|98765v3t10/(-&+l)"
		"\"'&%|#dy~}|{tyxwZoXsrqponmlkjihgfedcFa`_^]\\[ZY;Q:UNSRQ4"
		"I2MLKJIHGFEDCBA@?>=~;:9876543s+r/(-,+l)\"'h%$#\"!~}|{zyxw"
		"vutsVqponmPkjiLg`eHcba`_^]\\[ZYXWVUTS6QPONM0K.IHG@E>C&A:?"
		">=6;|38765.321r/(-n+*)('&%$#\"!~}|{zy\\wvutsrqpoQgPkdihgJ"
		"e^cFa`_^]\\[ZYXWVUTSRQ4ONMLKJIHG)?(C<A@?\"=<5|9876543210/"
		".-,+*)j'&%$#d!~}`{zs\\wvutsrqponmlkjihgJedcbaD_B]\\UZYRW:"
		"OTSRQJONM0KJC,GFEDCBA@?>=<;:987x543210/.-m%l)\"'h}$#\"!x}"
		"|{^yxqZutsrqponmlkjihgfeHcba`_^]\\[=S<WPUTS6QPOHM0KJIHGFE"
		"DCBA@?>=<;|98765v321r/.-&+l)('&%$#\"!~}|{zyxwZutsrqToRmlk"
		"dibgJed]baZ_BW\\[ZYRWVU8SRQJO2MLKJIHGFEDCBA@?>=~;:9876543"
		"s+r/(-,+l)('~%f#\"!~}|{zyxwvutsrqTonmlkjihgI_Hc\\a`_B]\\["
		"ZYRW:UTSRQPONMLKJIHGFE(CBA@?\"=<;|98765.3t10/.-,+*)('&%$#"
		"\"!b}|{zy\\wZunsrqjoRglkjibgfeHcba`_X]@[ZYXWVUTSRQPONMLK."
		"IHGFEDCBA#9\"=6;|92765.321r/.-,+$)j'&%$#\"!~}|{zyxwvuXsrq"
		"ponmlkMcLg`edcFa`_^]\\U>YXWVUTSRQPONMLKJI,GFEDC&A@?\"=<;:"
		"981x543210/.-,+*)('&%f#\"!~}`{^yxwpunsVqpinmfkNchgfe^cbaD"
		"_^]\\[ZS<WVUTSRQPONMLKJIHG*EDCBA@?>=}5|92765v3210/.'n+*)("
		"'&%$#\"!~}|{zy\\wvutsrqpoQgPkdihgJedcba`_^W@[ZYXWVUTSRQPO"
		"NMLK.IHGFE(CBA$?>=<;:981x543210/.-,+*)('&%f#\"!~}`{^yrwvu"
		"nsVkponmfkjiLgfedcba`YB]\\[ZYXWVUTSRQPONM0KJIHGFEDC%;$?8="
		"~;49870543t10/.-,+*#j'&%$#\"!~}|{zyxwvuXsrqponmlkMcLg`edc"
		"Fa`_^]\\[ZYRW:UTSRQPONMLKJIHGFE(CBA@?\"=<;|987654321*/p-,"
		"+*)('&%$#\"!~}|{^yxwvuXsVqpohmfkNihafe^cF[`_^]V[ZY<WVUTSR"
		"QPOHM0KJIHGFEDCBA@?>=<;|987654321q)p-&+*)j'&%$#\"!~}v{^yx"
		"wvutsrqponmlkjiLgfedcba`_AW@[TYXW:UTSRQPONMLKDI,GFEDCBA@?"
		">=<;:987x54321r/.-n+*)('&%$#\"!x}`{zyxwvutsrqponmlkNihgfe"
		"HcFaZ_^]V[>SXWVUNSRQ4ONMLKJIHGFE>C&A@?>=<;:987654321r/.-,"
		"+*)('g}f#z!b}v{zyrwvuXsrqponmlkjibgJedcba`_^]\\[ZYXWVU8SR"
		"QPONMLK-C,G@EDC&A@?>=<;:9876/v3210/.-,+*)('&%$#d!~}|{^yxw"
		"ZutsrqponmlkjcLgfedcba`_^]\\[ZYXW:UTSRQ4O2MLKDIBG*ED=BA:?"
		"\"7<;:92765v3210/.-,+*)(!h%$#\"!~}|{zyxwvutsVqponmlkjiKaJ"
		"e^cbaD_^]\\[ZYXWVUTM6QPONMLKJIHGFEDCBA$?>=<;:987w/v3,10/p"
		"-,+*)('&%$#\"!x}`{zyxwvutsrqponmlkNihgfeHcbaD_^]\\[ZYXWVU"
		"TSLQ4ONMLKJIHGFEDCBA@?\"=<;:9z765v3210/.-,+*)('~%f#\"!~}|"
		"{zyxwvutsrqTonmlkjihgI_Hc\\ECY^]\\?ZYXWVUTSRQPONGL/JIHGFE"
		"DCBA@?>=<;:{876543210p(o,%*)(i&%$#\"!~}|{zyxwpYtsrqponmlk"
		"jihgfedGba`_^A\\[Z=XWVUTSRQPONMLKD-HGFEDCBA@?>=<;:98y6543"
		"2s0q.',+*#(i~%$#\"y~}|_zyxwvutsrqponmfOjihgfedcba`_^]\\[Z"
		"=XWVUTSRQP2H1LEJ-HAFED=BA@#>=<;:987654321*q.-,+*)('&%$#\""
		"!~}|_zyxwvutsrTjSnglkjMhgfedcba`_^]\\[ZSX;VUTSRQPONMLKJIH"
		"GF)DCBA@#>=<}:9876543210/.-,%*k('&%$#\"!~}|{zyxwvYtsrqpSn"
		"QlkdihafI^cba`Y^]\\?ZYXWVUTSRQPONMLEJ-HGFEDCBA@?>=<;:98y6"
		"543210/.n&m*#('&g$#\"!~}|{zyxwvutmrUponmlkjihgfedcba`C^]\\"
		"[ZYXWV8N7RKPON1LKJIHGFEDCBA@?>=6}:9876543210/.-,+*k('&%$"
		"e\"!~a|{zyxwvutsrqponmfOjihgfedcba`_^]\\[Z=XWVUT7R5PONGLE"
		"J-HG@ED=B%:?>=<5:98y6543210/.-,+*)('~g$#\"!~}|{zyxwvutsrU"
		"ponmlkjihJ`Id]bE`_X]\\UZYX;VUTSRQPONMLKJIHG@)DCBA@?>=<;:9"
		"87654u210/.-,+*j\"i&}$#\"c~}|{zyxwvutsrqponglOjihgfedcba`"
		"_^]\\[Z=XWVUT7RQP3NMLKJIHGFEDCBA@?>7<}:9876543210/.-,+*k("
		"'&%$e\"c~}|uzsx[votsrkponQlkjihgfedcba`_^]\\UZ=XWVUTSRQPO"
		"NMLKJIH+FEDCBA@?>~6}:3zx0543t10/.-,+*)('&%$#\"!x}`{zyxwvu"
		"tsrqponmlkNihgfedcbaCYB]V[ZY<WVUTSRQPONMLKJIHGFEDCBA@?>=<"
		";:98765432+r/.-,+*)('&%$#\"!~}`{zyxwZutsVqponmlkjihgfedcb"
		"a`_^]\\[ZYXWVUTSRQPONMLE.IHGFEDCBA@?>=<;:9z76543t1r/.-&+$"
		")j'~%$#z!bw|{zyrwvuXsrqponmlkjihgfedcba`_^]\\[ZYXWVUTSRQP"
		"ONG0KJIHGFEDCBA@?>=<;|987654321q)p-&+l)('~%|#d!x}|{ty\\qv"
		"utslqpoRmlkjihgfedcba`_^]\\[ZYXWVUTSRQPONMLKJIHA*EDCBA@?>"
		"=<;:98765v3210/.-,+k#j'~%$#d!~}|{zyxwvutsrqponmlkjihgfedc"
		"ba`_^]\\[ZYRW:UTSRQPONMLKJIHGFE(CBA@?\"=<;|9876543210/.-,"
		"+*)('&%$#\"!~}|{zyxwvutsrqjoRmlkjihgfedcba`_^]@[ZYXW:U8SR"
		"QJOHM0KDIHG@E(=BA@?8=<;|9876543210/.-,+*)('&%$#\"!~}|{zyx"
		"wvutsrqjoRmlkjihgfedcba`_^]@[ZYXWVUTS5K4OHMLK.IHGFEDCBA@?"
		">=<;:9876543210/.-,+*)('&%$#z!b}|{zyxwvutsrqponmPkjihgfed"
		"cE[D_X]\\[>YXWVUTSRQPONM0E.IHGFEDCBA@?>=<;:9z76543t10/p-,"
		"+*)('&%$#\"!bw`{zyxwvutsrqponmlkNihgfeHcFa`_X]V[>YRWVUNS6"
		"KPONMFKJI,GFEDCBA@?>=<;|3z76543210/.-,+*)('h%$#\"!~}|{]s\\"
		"wputsVqponmlkjihgfeH]Fa`_^]\\[ZYXWVUTSRQ4ONMLKJIHG)?(C<A"
		"@?\"=<;:987654321r/(-n+*)('&%$#\"!~}|{zy\\wvutsVqpoRmlkji"
		"hgfedcbaD_X]@[ZYXWVUTSRQPONMLK.IHGFE(C&A@?8=6;|92765.3t+0"
		"/.-&+*)j'&%$#\"!~}|{zy\\wpuXsrqponmlkjihgfedcFa`_^]\\[ZY;"
		"Q:UNSRQ4ONMLKJIHGFEDC&A:?\"=<;:9876543210/.-n+*)('&%$#cyb"
		"}v{zy\\wvutsrqponmlkNihaJedcba`_^]\\[ZYXWVU8SRQPO2MLK.IHG"
		"FEDCBA@?>=~;:3z76543210/.-,+*)('h%$#\"!b}`{zyrwpuXslqpohm"
		"Pejihg`edcFa`_^]\\[ZYXWVU8SRK4ONMLKJIHGFEDCBA@?\"=<;:9876"
		"5u-t1*/p-,+$)\"'h%|#\"!x}`uzyxwputsVqponmlkjihgfeHcb[D_^]"
		"\\[ZYXWVUTSRQPO2MLKJIHGFE'=&A:?>=~;:9876543210/p-,+$)j'&%"
		"$#\"!~}|{zyxwvuXsrqpoRmlkNihgfedcba`_^]@[ZYRW:UTSRQPONMLK"
		"JIHGFE(CBA@?\"=~;:92705v3,10/(-n%*)('~%$#d!~}|{zyxwvutsVq"
		"pohmPkjihgfedcba`_^]\\[>YXWVUTSRQ3I2MFKJI,GFEDCBA@?>=<;|9"
		"8705v3210/.-,+*)('&%$#d!~}|{zyxwYoXslqpoRmlkjihgfedcbaD_^"
		"]\\U>YXWVUTSRQPONMLKJI,GFEDC&A@?\"=<;:987654321r/.-,%l)('"
		"&%$#\"!~}|{zyxwZutsrqToRmlkdibgJe^cbaZ_BW\\[ZYRWVU8SRQPON"
		"MLKJIHG*EDCB;$?>=<;:9876543210/p-,+*)('&%e{d!x}|{^yxwvuts"
		"rqponmPkjihaJedcba`_^]\\[ZYXWVU8SRQPONMLK-C,G@EDC&A@?>=<;"
		":98765v3210/(-n+*)('&%$#\"!~}|{zy\\wvutsVqpoRmlkjihgfedcb"
		"aD_^]\\[TY<WVUTSRQPONMLKJIHG*EDCBA$?\"=<;4927x5.321*/p',+"
		"*)\"'&%f#\"!~}|{zyxwvuXsrqpohmPkjihgfedcba`_^]\\[>YXWVUTS"
		"RQ3I2MFKJI,GFEDCBA@?>=<;|98765.3t10/.-,+*)('&%$#\"!b}|{zy"
		"xwvuWmVqjonmPkjihgfedcba`_B]\\[ZYXQ:UTSRQPONMLKJIHGFE(CBA"
		"@?\"=<;|9876543210/.-n+*)('&}f#\"!~}|{zyxwvutsrqTonmlkNiL"
		"gfe^c\\aD_X]\\[TY<QVUTSLQPO2MLKJIHGFEDCBA$?>=<;:3z7654321"
		"0/.-,+*)('h%$#\"!~}|{]s\\wputsVqponmlkjihgfeHcba`_^W@[ZYX"
		"WVUTSRQPONMLK.IHGFEDCBA#9\"=6;:9z76543210/.-,+l)('&%$#z!b"
		"}|{zyxwvutsrqponmPkjihgJedcFa`_^]\\[ZYXWVU8SRQPONMFK.IHGF"
		"EDCBA@?>=<;:9z76543t1r/.-&+$)j'~%$#z!bw|{zyrwvuXsrqponmlk"
		"jihgJedcba`_X]@[ZYXWVUTSRQPONMLK.IHGFEDCBA#9\"=6;:9z76543"
		"210/.-,+l)('&%$#z!b}|{zyxwvutsrqponmPkjihgfedcE[D_X]\\[>Y"
		"XWVUTSRQPONM0KJIHGFED=&A@?>=<;:987654321r/.-,+l)('h%$#\"!"
		"~}|{zyxwZutsrqpongPkjihgfedcba`_^]\\[>YXWVU8S6QPOHMFK.IBG"
		"FE>C&;@?>=6;:9z76543210/.-,+l)('&%$#\"yb}|{zyxwvutsrqponm"
		"PkjihgfedcE[D_X]\\[>YXWVUTSRQPONM0KJIHGFED=&A@?>=<;:98765"
		"4321r/.-,+*)('g}f#z!~}`{zyxwvutsrqpoRmlkjihgfe^cFa`_^]\\["
		"ZYXWVUTSRQ4ONMLK.IHG*EDCBA@?>=<;:9z76543210/(-n+*)('&%$#\""
		"!~}|{zy\\wvutsVqTonmfkdiLgf_dc\\aD_X]\\[TY<QVUTSLQPO2MLK"
		"JIHGFEDCBA$?>=<;:98705v3210/.-,+*)('&%$#d!~}|{zyxwYoXslqp"
		"oRmlkjihgfedcbaD_^]\\[ZYXWPU8SRQPONMLKJIHGFEDC&A@?>=<;:9y"
		"1x5.321r/.-,+*)('&%$#d!~}|{zyxwvoXsrqponmlkjihgfedcFa`_^]"
		"@[ZY<WVUTSRQPONMLK.IHGFEDCBA@9\"=<;:9876543210/.-n+*)('h%"
		"f#\"y~}v{^sxwvunsrqTonmlkjihgfedcFa`_^]\\[ZYXQ:UTSRQPONML"
		"KJIHGFE(CBA@?>=<;{3z70543t10/.-,+*)('&%f#\"!~}|{zyxqZutsr"
		"qponmlkjihgfeHcba`_^]\\[=S<WPUTS6QPONMLKJIHGFE(CBA@?>=<;:"
		"927x543210/.-,+*)('&%f#\"!~}`{zy\\wvutsrqponmlkNihgfedcba"
		"`_X]@[ZYXWVUTSRQPONMLK.IHGFE(C&A@?8=6;|98165.3t1*/.-&+l#("
		"'&%|#\"!b}|{zyxwvutsrqTonmlkjihgfe^cFa`_^]\\[ZYXWVUTSRQ4O"
		"NMLKJIHG)?(C<A@?\"=<;:987654321r/.-,+*)('&%|#d!~}|{zyxwvu"
		"tsrqpoRmlkjihgfeG]FaZ_^]@[ZYXWVUTSRQPO2MLKJIHGFEDCBA@9\"="
		"<;:9876543210/.-n+*)('h%$#d!~}|{zyxwvutsVqponmlkjihgfed]F"
		"a`_^]\\[ZYXWVUTSRQ4ONMLK.I,GFE>C<A$?>7<;49z70543,1r).-,+$"
		")('h%$#\"!~}|{zyxwZutsrqponmlkjihaJedcba`_^]\\[ZYXWVU8SRQ"
		"PONMLK-C,G@EDC&A@?>=<;:98765v3210/.-,+*)('&}f#\"!~}|{zyxw"
		"vutsrqTonmlkjihgI_Hc\\a`_B]\\[ZYXWVUTSRQ4ONMLKJIHGFEDCBA@"
		"?8=~;:9876543210/.-,+l)('&%f#\"!b}|{zyxwvutsrqTonmlkjihgf"
		"edcba`_X]@[ZYXWVUTSRQPONMLK.IHGFE(CBA$?>=<;:9876543t10/.-"
		",+*)('&%$#\"!x}`{zyxwvutsrqponmlkNihgfedcbaCYB]V[>YXQVUNS"
		"6QJONMFK.CHGFE>'%;@?>!<;:9876543210q.-,+*)('&%$#\"!~}|uz]"
		"xwvutsrqponmlkjihKfedcba`_^@V?ZSXWV9TSRQPONMLKJIH+FEDCBA@"
		"?>=<;:98765.u210/.-,+*)('&%$#\"c~}|{z]xwvYtsrqponmlkjihKf"
		"edcba`_^]\\[ZYXWVUN7RQPONMLKJIHGFEDCB%@?>=<}:{876/4-2s*/."
		"-,%*)(i&%$#\"!~}|{zyx[vutsrqponmlkjihgfe^Gba`_^]\\[ZYXWVU"
		"TSR5PONMLKJIH*@)D=B%:?>=<5:98y6543210/.-,+*k('&%$#\"!~}|{"
		"zyxwvunWrqponmlkjihgfedcbE`_^]\\[ZYX:P9TMRQP3NMLKJIHGFEDC"
		"B%@?>=<;:9876543210/.',m*)('&%$#\"!~}|{zyx[vutsrUponQlkji"
		"hgfedcba`C^]\\[ZYXWVUTSRQPONMLEJ-HGFEDCBA@?>=<;:98y65432s"
		"0q(-,+*#('&g$#\"!~}|{zyxwvYtsrqponmlkjihgfedcb[`C^]\\[ZYX"
		"WVUTSRQPON1LKJIHGFED&<%@9>!6;:981654u210/.-,+*)('&g$#\"!~"
		"}|{zyxwvutsrqpinQlkjihgfedcba`_^]\\?ZYXWVUTSR4J3NGLKJ-HGF"
		"EDCBA@?>=<}:9876543210/.-,+*)('~g$#\"!~}|{zyxwvutsrUponml"
		"OjihKfedcba`_^]\\[Z=XWVUTSRQPONMLKJIHGFE>'BA@?>=<;:987654"
		"32s0/.-,m*k('&}${\"c~}v{zsxwvYtsrqponmlkjihKfedcba`_^]\\["
		"ZYXWVUTSL5PONMLKJIHGFEDCBA@#>=<;:9876v.u2+rp(-,+l)('&%$#\""
		"!~}|{^yxwvutsrqponmlkjihgf_Hcba`_^]\\[ZYXWVUTS6QPONMLKJI"
		"+A*E>CBA$?>=<;:9876543t10/.-,+*)('&%$#\"!~}|{ty\\wvutsrqp"
		"onmlkjihgJedcbaD_^]@[ZYXWVUTSRQPO2MLKJIHGFEDCBA@?>=<;:927"
		"x543210/.-,+*)('&%f#\"!~}`{^yxwpunsVqjonmfkjiLgfedcba`_^]"
		"\\[>YXWVUTSRQPONMLKJIHGFE>C&A@?>=<;:987654321r/.-,+*)('g}"
		"f#z!b}|uzyr[YotsrUponmlkjihgfedGba`_^]\\[ZYXWVUTSRQPONGL/"
		"JIHGFEDCBA@?>=<;:{876543210p(o,%*)(i&%$#\"!~}|{zyx[vutsrq"
		"ponmlkjihgfedcbaZC^]\\[ZYXWVUTSRQPON1LKJIH+FED'BA@?>=<;:9"
		"876w43210/.-,+*)('&%$#\"!~}v_zyxwvutsrqponmlkjMhgfedGbE`_"
		"^W\\UZYX;VUTSRQPONMLKJ-HGFEDCBA@?>=<;:9876543,s0/.-,+*)('"
		"&%$#\"!~a|{zyxwvutVlUpiRPfkjiLgfedcba`_^]\\[>YXWVUTSRQPON"
		"MLKJIHGFED=&A@?>=<;:987654321r/.-,+*)('g}f#z!~}`{zyxwvuts"
		"rqpoRmlkjihgfedcba`_^]\\[ZYXWPU8SRQPONMLKJIHGFEDC&A@?>=~;"
		":9z76543210/.-,+l)('&%$#\"!~}|{zyxwvutsrqjoRmlkjihgfedcba"
		"`_^]@[ZYXW:UTS6QPONMLKJIHGFE(CBA@?>=<;:9876543210/.-&+l)("
		"'&%$#\"!~}|{zyxwZutsrqponmOeNibgJedc\\aZCAW\\[Z=XWVUTSRQP"
		"ONML/JIHGFEDCBA@?>=<;:987654-2s0/.-,+*)('&%$#\"!~a|{zyxwv"
		"utVlUpinmlOjihgfedcba`_^A\\[ZYXWVUTSRQPONMLKJIHGFE>'BA@?>"
		"=<;:98765432s0/.-,m*)(i&%$#\"!~}|{zyx[vutsrqponmlkjihgfed"
		"cba`_XA\\[ZYXWVUTSRQPONML/JIHGF)DCB%@?>=<;:987654u210/.-,"
		"+*)('&%$#\"!~}|{zyr[vutsrqponmlkjihgfIdcba`_^]\\>T=XQV9TS"
		"LQPIN1LEJIHAF)>CBA@9\"~6;:9z76543210/.-,+l)('&%$#\"!~}|{z"
		"yxwvutsrqpiRmlkjihgfedcba`_^]@[ZYXWVUTS5K4OHMLK.IHGFEDCBA"
		"@?>=~;:9876543210/.-,+*)('&%$#z!b}|{zyxwvutsrqponmPkjihgJ"
		"edcFa`_^]\\[ZYXWVU8SRQPONMLKJIHGFEDCBA@?>=<;49z76543210/."
		"-,+*)('h%$#\"!b}|{^yxwvutsrqponmPkjihgfedcba`_^]\\[ZYXWVU"
		"TSLQ4ONMLKJIHGFEDCBA@?\"=<;:98765u-t1*/.-n+*)('&%$#\"!~}`"
		"{zyxwvutsrqponmlkjihgfedc\\aD_^]\\[ZYXWVUTSRQPO2MLKJIHGFE"
		"'=&A:?>=~;:9876543210/p-,+*)('&%$#\"!~}|{zyxwvutsrkTonmlk"
		"jihgfedcba`_B]\\[ZY<WVU8SRQPONMLKJIHG*EDCBA@?>=<;:9876543"
		"210/.-,%l)('&%$#\"!~}|{zyxwZutsrqToRmlkdibgfeHcba`_^]\\[Z"
		"YXW:UTSRQPONMLKJIHGFEDCBA@?>=<5|9876543210/.-,+*)j'&%$#\""
		"!~}_u^yrwZutmrqjoRmfkjibgJ_dcbaZCAW\\[Z=XWVUTSRQPONML/JIH"
		"GFEDCBA@?>=<;:987654321*q.-,+*)('&%$#\"!~}|_zyxwvutsrTjSn"
		"glkjMhgfedcba`_^]\\?ZYXWVUTSRQPONMLKJIHGFEDCBA@9>!<;:9876"
		"543210/.-,m*)('&g$#\"c~}|{zyxwvutsrUponmlkjihgfedcba`_^]\\"
		"[ZYXWVOT7RQPONMLKJIHGFEDCB%@?>=<}:{876/4-2s*/.-,%*)(i&%$"
		"#\"!~}|{zyx[vutsrqponmlkjihgfedcba`_^]\\UZ=XWVUTSRQPONMLK"
		"JIH+FEDCBA@?>~6}:38y654-2+0q(-,+*#('&g$#\"!~}|{zyxwvYtsrq"
		"ponmlkjihgfedcba`_^]\\[ZSX;VUTSRQPONMLKJIHGF)DCBA@?>=<|4{"
		"81654u210/.-,+*)('&g$#\"!~}|{zyxwvutsrqponmlkjihg`Idcba`_"
		"^]\\[ZYXWVUT7RQPON1LKJ-HGFEDCBA@?>=<}:9876543210/.-,+*)('"
		"&%$#\"!~}v_zyxwvutsrqponmlkjMhgfedGbE`_X]\\UZ=RWVUTMRQP3N"
		"MLKJIHGFEDCB%@?>=<;:9876543210/.-,+*)('&%|e\"!~}|{zyxwvut"
		"srqpSnmlkjihgfH^Gb[`_^A\\[ZYXWVUTSRQP3NMLKJIHGFEDCBA@?>=<"
		";:9876543,s0/.-,+*)('&%$#\"!~a|{zyxwvutVlUpinmlOjihgfedcb"
		"a`_^A\\[ZYXWVUTSRQPONMLKJIHGFEDCBA@9>!<;:9876543210/.-,m*"
		")('&g$#\"c~}|{zyxwvutsrUponmlkjihgfedcba`_^]\\[ZYXWVUTMR5"
		"PONMLKJIHGFEDCBA@#>=<;:{8y654-2+0q.-&+*#(i~%$#\"y~}|_zyxw"
		"vutsrqponQlkjihgfedcba`_^]\\[ZYXWVUTSRQPIN1LKJIHGFEDCBA@?"
		">=<}:98765432r*q.',m*)\"'&}$#\"c~}|{zyxwvutsrUponmlkjihgf"
		"edcba`_^]\\[ZYXWVUTMR5PONMLKJIHGFEDCBA@#>=<;:9876v.u2+0/."
		"o,+*)('&%$#\"!~a|{zyxwvutsrqponmlkjihgfedcba`_XA\\[ZYXWVU"
		"TSRQPONML/JIHGF)DCB%@?>=<;:987654u210/.-,+*)('&%$#\"!~}|{"
		"zyxwvutslUponmlkjihgfedcba`C^]\\[Z=X;VUTMRKP3NMFKJCH+F?DC"
		"B;@?>!<;:9876543210q.-,+*)('&%$#\"!~}|{zyxwvutsrqpohQlkji"
		"hgfedcba`_^]\\?ZYXWVUTSR4J3NG0.DIHG*EDCBA@?>=<;:9z7654321"
		"0/.-,+*)('&%$#\"!~}|{zyxqZutsrqponmlkjihgfeHcba`_^]\\[=S<"
		"WPUTS6QPONMLKJIHGFE(CBA@?>=<;:9876543210/.-,+*)('&%|#d!~}"
		"|{zyxwvutsrqpoRmlkjiLgfeHcba`_^]\\[ZYXW:UTSRQPONMLKJIHGFE"
		"DCBA@?>=<;:98705v3210/.-,+*)('&%$#d!~}|{^y\\wvotslqTinmlk"
		"dihgJedcba`_^]\\[ZY<WVUTSRQPONMLKJIHGFEDCBA@?>=<;:927x543"
		"210/.-,+*)('&%f#\"!~}|{zy[qZunsrqTonmlkjihgfedcFa`_^]\\[Z"
		"YXWVUTSRQPONMLKJIHGFEDC<A$?>=<;:9876543210/p-,+*)('&%e{d!"
		"x}|{^yxwvutsrqponmPkjihgfedcba`_^]\\[ZYXWVUTSRQPONMLE.IHG"
		"FEDCBA@?>=<;:9z76543t10/p-,+*)('&%$#\"!b}|{zyxwvutsrqponm"
		"lkjihgfedcba`_^W@[ZYXWVUTSRQPONMLK.IHGFE(C&A@?8=6;|98165."
		"3t+0/.-&+*)j'&%$#\"!~}|{zy\\wvutsrqponmlkjihgfedcba`_^]\\"
		"[ZYXQ:UTSRQPONMLKJIHGFE(CBA@?>=<;{3z705v32+0/(-,+l)('&%$#"
		"\"!~}|{^yxwvutsrqponmlkjihgfedcba`_^]\\[ZS<WVUTSRQPONMLKJ"
		"IHG*EDCBA@?>=}5|92765v3210/.-,+*)('h%$#\"!~}|{zyxwvutsrqp"
		"onmlkjihgfedc\\aD_^]\\[ZYXWVUTSRQPO2MLKJI,GFE(CBA@?>=<;:9"
		"87x543210/.-,+*)('&%$#\"!~}|{zyxwvutslqTonmlkjihgfedcba`_"
		"B]\\[ZY<WVU8SRQPONMLKJIHG*EDCBA@?>=<;:9876543210/.-,+*)('"
		"&%|#d!~}|{zyxwvutsrqpoRmlkjihgfeG]FaZ_B]\\UZYRW:OTSRQJ31G"
		"LKJ-HGFEDCBA@?>=<}:9876543210/.-,+*)('&%$#\"!~}|{zyxqvYts"
		"rqponmlkjihgfedGba`_^]\\[Z<R;VOTSR5PONMLKJIHGFED'BA@?>=<;"
		":9876543210/.-,+*)('&%$#\"!xa|{zyxwvutsrqponmlOjihgfIdcbE"
		"`_^]\\[ZYXWVUT7RQPONMLKJIHGFEDCBA@?>=<;:987654321*q.-,+*)"
		"('&%$#\"!~}|_zyxwvYtWrqjonglOdihgf_dcbE`_^]\\[ZYXWVUT7RQP"
		"ONMLKJIHGFEDCBA@?>=<;:987654321*q.-,+*)('&%$#\"!~}|_zyxwv"
		"utsrTjSnglkjMhgfedcba`_^]\\?ZYXWVUTSRQPONMLKJIHGFEDCBA@?>"
		"=<;:92y6543210/.-,+*)('&g$#\"!~}|{z\\r[votsrUponmlkjihgfe"
		"dGba`_^]\\[ZYXWVUTSRQPONMLKJIHGFEDCBA@9>!<;:9876543210/.-"
		",m*)('&g$#\"c~}|{zyxwvutsrUponmlkjihgfedcba`_^]\\[ZYXWVUT"
		"SRQPONGL/JIHGFEDCBA@?>=<;:{87654u2s0/.',%*k('~%${\"cx}|{z"
		"sxwvYtsrqponmlkjihKfedcba`_^]\\[ZYXWVUTSRQPONMLKJIHGFED=B"
		"%@?>=<;:9876543210q.-,+*)('&f|e\"y~a|{tyxqvutWrqponmlkjih"
		"gfIdcba`_^]\\[ZYXWVUTSRQPONMLKJIHGFEDCB;@#>=<;:9876543210"
		"/.o,+*)('&%$dzc~w|{z]xwvutsrqponmlOjihgfedcba`_^]\\[ZYXWV"
		"UTSRQPONMLKJIHG@)DCBA@?>=<;:987654u210/.o,+*k('&%$#\"!~}|"
		"{z]xwvutsrqponmlkjihgfedcba`_^]\\[ZYXWVUN7RQPONMLKJIHGFED"
		"CB%@?>=<}:98y6543210/.-,+*k('&%$#\"!~}|{zyxwvutsrqponmlkj"
		"ihgfedc\\E`_^]\\[ZYXWVUTSRQP3NMLKJIHGF(>'B;@#>=<5:38y6/43"
		"2+0q(-,+*#jh~%$#d!~}|{zyxwvutsVqponmlkjihgfedcba`_^]\\[ZY"
		"XWVUTSRQPONG0KJIHGFEDCBA@?>=<;|98765v3210/.-,+*";
	code = (char*)malloc(1);
	if (code == 0) {
		fprintf(stderr,"error: out of memory\n");
		return 1;
	}
	code[0]=0;
	current_code_size=0;
	min_rot_width=10;
	for (j=0;j<generation_size;j++) {
		change_0_1_trits(&(generation[j].val));
	}
	init_all(c);

	while ((current_code_size + c) % 94 != 17) {
		add_code("o");
	}
	add_code("<<");

	if (min_t0_cell >= 0 && min_t0_cell < current_code_size) {
		fprintf(stderr,"error: at least one address is too low and overlaps with initialization code\n");
		return 1;
	}

	denormalize(code,c);

	size_t pos = 0;
	size_t i;
	size_t len = strlen(init_module);
	for (i=0;i<len;i++) {
		fwrite(init_module+i,1,1,output);
		pos++;
		if (ll > 0 && pos%ll==0) {
			fwrite("\n",1,1,output);
		}
	}
	len = strlen(code);
	for (i=0;i<len;i++) {
		fwrite(code+i,1,1,output);
		pos++;
		if (ll > 0 && pos%ll==0) {
			fwrite("\n",1,1,output);
		}
	}
	if (ll <= 0 || pos%ll!=0) {
		fwrite("\n",1,1,output);
	}
	return 0; /* success */
}

void init_all(long long c_offset) {

	long long j;
	code[0]=0;
	current_code_size=0;


	min_rot_width = 10;
	while (min_rot_width < needed_backjump_rotwidth) {
		unsigned int i;
		long long twos = 2;


		/* erzeuge IRGENDWO (-> in var1) C0, erzeuge dann ...0222222 mit maximal vielen 2en, crazie das in C0, springe ans ziel mit movd; kehre von dort zurück! */
		/* -> am besten in var1 */

		for (i=0;i<min_rot_width;i++) { /* width */
			add_code("ooooooooj*jo");
		}

		add_code("*oooojpjoooooojpjoooooojpjo"); /* erzeuge C0 an position var1 */

		/* erzeuge ...0222222 */
		for (i=1;i<min_rot_width;i++) {
			twos *= 3;
			twos += 2;
		}
		set_gen_init_03(twos, min_rot_width, 0);
		add_code("oj*op");

		/* crazie das in var1, erhalte ...100000 */
		add_code("oojpjo");

		// ERZEUGE BACK-JUMP-address!!
		{
			long long max_value_C1_rot_width = 1;
			long long address_gen_size;
			long long inner_reset_point;
			for (i=1;i<min_rot_width;i++) {
				max_value_C1_rot_width *= 3;
				max_value_C1_rot_width += 1;
			}

			inner_reset_point = current_code_size;
			address_gen_size = 150; /* start with this */
			do {
				long long backjump_address = c_offset + current_code_size + address_gen_size;

				if (backjump_address > 2*max_value_C1_rot_width) {
					fprintf(stderr,"error: cannot increase rotation width as required");
					exit(1);
				}

				add_code("*oooooojpjoooooooojpjo");

				long long gen = 0;
				long long tmp = backjump_address;
				long long factor = 1;
				long  long i;
				for (i=0;i<min_rot_width;i++) {
					if (tmp % 3==0) {
						gen += factor;
					}else{
						gen += 2*factor;
					}
					factor *= 3;
					tmp /= 3;
				}
				set_gen_init_03(gen, min_rot_width, 0);
				/*printf("bja1-gen: %d\n",gen); */
				add_code("oooojpjo");
				gen = 0;
				tmp = backjump_address;
				factor = 1;
				for (i=0;i<min_rot_width;i++) {
					if (tmp % 3==1) {
						gen += 2*factor;
					}else{
						gen += factor;
					}
					factor *= 3;
					tmp /= 3;
				}
				set_gen_init_03(gen, min_rot_width, 0);
				/*printf("bja2-gen: %d\n",gen); */
				add_code("oooojpjo");
				add_code("oooooooooji");
				if (backjump_address+1 >= c_offset + current_code_size) {
					while (backjump_address+1 > c_offset + current_code_size) {
						add_code("o");
					}
					add_code("jo");
					/*printf("bja: %d\n",backjump_address); */
					break;
				}
				address_gen_size += c_offset + current_code_size - backjump_address;
				current_code_size = inner_reset_point;
				code[inner_reset_point] = 0;
			}while(1);
		}



		/* gehe zu ...10000, kehre danach sofort zurück */
		add_code("ooooojjojooj");

		min_rot_width *= 2;
	}

	for (j=0;j<generation_size;j++) {
		if (initialize_memory_cell(generation[j],0,c_offset)==-1) {
			/* FAILED DUE TO rot-width; HAS BEEN INCREASED; => restart the whole process! */
			init_all(c_offset);
			return;
		}
	}


	/* generate ENTRY-Point, move to entry-point */
	if (initialize_entry_point(entry_offset, entry,0,c_offset)==-1) {
			/* FAILED DUE TO rot-width; HAS BEEN INCREASED; => restart the whole process! */
			init_all(c_offset);
			return;
	}
	// write command "i" to ...120 (value: 88).
	add_code("*ooojjopoooooooojoooojopoooooooojoooooooooj*jojopoooooooojoooo");
	
	/* jump to entry point, start program execution */
	add_code("oooooojji");
}




int initialize_memory_cell(MemInit cell, unsigned int rot_width_at_least, long long c_start_offset) { /* breite: 10 (wegen backjump) */

	unsigned int i;
	long long reset_point = current_code_size;
	long long max_value_C1_rot_width = 1;
	long long inner_reset_point;
	long long address_gen_size;

	if (cell.addr.offset == T2 || (cell.addr.offset == T0 && cell.addr.val <= 0)) {
		/* invalid address */
		fprintf(stderr,"error: invalid address\n");
		exit(1);
	}

	if (cell.val.offset == T2 || (cell.val.offset == T0 && cell.val.val < 0)) {
		/* invalid address */
		fprintf(stderr,"error: invalid value: C%d%s%lld\n",cell.val.offset==T2?2:0,cell.val.val<0?"":"+",cell.val.val);
		exit(1);
	}

	if (cell.addr.offset == T1 && cell.addr.val > -41 && cell.addr.val < 38) {
		fprintf(stderr,"error: canot initialize cell in reserved memory area\n");
		exit(1);
	}

	if (cell.addr.offset == T0) {
		if (min_t0_cell == -1 || min_t0_cell < cell.addr.val) {
			min_t0_cell = cell.addr.val;
		}
	}

	if (rot_width_at_least < needed_backjump_rotwidth) {
		rot_width_at_least = needed_backjump_rotwidth;
	}


	/* get minimal necessary rot width: */
	/* compute maximum possible constant */
	for (i=1;i<rot_width_at_least;i++) {
		max_value_C1_rot_width *= 3;
		max_value_C1_rot_width += 1;
	}
	/* check whether the rot width is enough or not... */
	while ((cell.addr.offset == T1 && (cell.addr.val >= max_value_C1_rot_width || -cell.addr.val+1 >= max_value_C1_rot_width))
			|| (cell.val.offset == T1 && (cell.val.val > max_value_C1_rot_width || -cell.val.val >= max_value_C1_rot_width))
			|| (cell.addr.val >= 2*max_value_C1_rot_width)
			|| (cell.val.val > 2*max_value_C1_rot_width)
			) {
		rot_width_at_least++;
		max_value_C1_rot_width *= 3;
		max_value_C1_rot_width += 1;
	}
	/* HINT: rot-width is not checked / cannot be checked for backjump-address here */


	if (rot_width_at_least > min_rot_width) {
		/* increase rot-width. */
		needed_backjump_rotwidth = (needed_backjump_rotwidth>rot_width_at_least?needed_backjump_rotwidth:rot_width_at_least);
		return -1; /* BREAK UP, RESET EVERYTHING, INCREASE MIN_ROT_WIDTH, THEN START AGAIN!!! */
	}


	/* generate cell_position in var2, value in var1, return-address (c-register) in var3 */
	/* then call ROTATION-MODULE */
	/* at last: load C1, jump to [var2], crazy, jump to [var2], crazy, jump to var1, rot, jump to [var2], crazy */

	/* set rotation-width */
	for (i=0;i<rot_width_at_least;i++) { /* width */
		add_code("ooooooooj*jo");
	}

	/* set var1 */
	add_code("*oooojpjoooooojpjo");
	switch (cell.val.offset) {
		case T0:
		{

			long long gen = 0;
			long long tmp = cell.val.val;
			long long factor = 1;
			long long i;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==0) {
					gen += factor;
				}else{
					gen += 2*factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 1);
			add_code("oojpjo");
			gen = 0;
			tmp = cell.val.val;
			factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==1) {
					gen += 2*factor;
				}else{
					gen += factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 1);
			add_code("oojpjo");
			break;
		}
		case T1:
		{

			long long gen = 0;
			long long tmp = cell.val.val + max_value_C1_rot_width; /* OFFSET-CALCULATION */
			long long factor = 1;
			long long i;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==2) {
					gen += 2*factor;
				}else{
					gen += factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 1);
			add_code("oojpjo");
			gen = 0;
			tmp = cell.val.val + max_value_C1_rot_width; /* OFFSET-CALCULATION */
			factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==0) {
					gen += 2*factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 1);
			add_code("oj*op");
			add_code("oojpjo");
			break;
		}
		default:
			fprintf(stderr,"error: invalid value (stage 2)\n");
			exit(1); /* error!! */
	}

	/* set var2 */
	add_code("*ooooojpjooooooojpjo");
	switch (cell.addr.offset) {
		case T0:
		{
			long long i;
			long long gen = 0;
			long long tmp = cell.addr.val-1;
			long long factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==0) {
					gen += factor;
				}else{
					gen += 2*factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 0);
			add_code("ooojpjo");
			gen = 0;
			tmp = cell.addr.val-1;
			factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==1) {
					gen += 2*factor;
				}else{
					gen += factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 0);
			add_code("ooojpjo");
			break;
		}
		case T1:
		{
			long long i;
			long long gen = 0;
			long long tmp = cell.addr.val + max_value_C1_rot_width - 1; /* OFFSET-CALCULATION */
			long long factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==2) {
					gen += 2*factor;
				}else{
					gen += factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 0);
			add_code("ooojpjo");
			gen = 0;
			tmp = cell.addr.val + max_value_C1_rot_width - 1; /* OFFSET-CALCULATION */
			factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==0) {
					gen += 2*factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 0);
			add_code("oj*op");
			add_code("ooojpjo");
			break;
		}
		default:
			fprintf(stderr,"error: invalid address (stage 2)\n");
			exit(1); /* error!! */
	}


	/* NOW: GENERATE BACKJUMP ADDRESS! */

	inner_reset_point = current_code_size;
	address_gen_size = 150; /* start with this */
	do {
		long long backjump_address = c_start_offset + current_code_size + address_gen_size;

		if (backjump_address > 2*max_value_C1_rot_width) {
			needed_backjump_rotwidth = rot_width_at_least+1;
			/* restart THIS function with new rot-width */
			/* DELETE EVERYTHING DONE IN THIS FUNCTION!!! */
			current_code_size = reset_point;
			code[current_code_size] = 0;
			return initialize_memory_cell(cell, needed_backjump_rotwidth,c_start_offset);
		}

		add_code("*oooooojpjoooooooojpjo");

		long long gen = 0;
		long long tmp = backjump_address;
		long long factor = 1;
		long long i;
		for (i=0;i<rot_width_at_least;i++) {
			if (tmp % 3==0) {
				gen += factor;
			}else{
				gen += 2*factor;
			}
			factor *= 3;
			tmp /= 3;
		}
		set_gen_init_03(gen, rot_width_at_least, 0);
		/*printf("bja1-gen: %d\n",gen); */
		add_code("oooojpjo");
		gen = 0;
		tmp = backjump_address;
		factor = 1;
		for (i=0;i<rot_width_at_least;i++) {
			if (tmp % 3==1) {
				gen += 2*factor;
			}else{
				gen += factor;
			}
			factor *= 3;
			tmp /= 3;
		}
		set_gen_init_03(gen, rot_width_at_least, 0);
		/*printf("bja2-gen: %d\n",gen); */
		add_code("oooojpjo");
		add_code("oooooooooji");
		if (backjump_address+1 >= c_start_offset + current_code_size) {
			while (backjump_address+1 > c_start_offset + current_code_size) {
				add_code("o");
			}
			add_code("jo");
			/*printf("bja: %d\n",backjump_address); */
			break;
		}
		address_gen_size += c_start_offset + current_code_size - backjump_address;
		current_code_size = inner_reset_point;
		code[inner_reset_point] = 0;
	}while(1);
	int var2_even = 0;
	/* if (even(var2)) { add_code("o"); } */
	if (cell.addr.val < 0) {
		if ((-cell.addr.val) % 2 == 0) {
			var2_even = 1;
		}
	}else{
		if (cell.addr.val % 2 == 0) {
			var2_even = 1;
		}
	}
	/* set cell at var2 to C1 */
	add_code("*ooooojjp");
	if (var2_even) add_code("o");
	add_code("joojoooooojjp");
	if (var2_even) add_code("o");
	add_code("joojoooooj*jooooooojjp"); /* write var1 into var2 */
	if (var2_even) add_code("o");
	/* jump back */
	add_code("jooj");
	return 0;
}


void set_gen_init_03(long long value, unsigned int width, int omit_last_rot) {
	unsigned int i;
	/* expect: const-gen-module; address 54 == C1; A=C1 */

	/* if value == C1 */
	/* return(success) */

	/* expect D=65 */

	add_code("*opjp");
	int anything_set = 0;

	/* D=68, A=C1 */

	for (i=0;i<width;i++) {
		if (get_trit(i, value) == T2) {
			if (omit_last_rot && i+1 == width) {
				add_code("oj*pp");
			}else{
				add_code("oj*ppj*");
			}
			anything_set = 1;
		} else if (anything_set && (!omit_last_rot || i+1 != width)) {
			add_code("j*");
		}
	}
	return;
}

Trit get_trit(unsigned int position, long long value) {
	unsigned int i;
	if (position < 0) {
		fprintf(stderr,"error: invalid call to get_trit: position\n");
		exit(1); /* invalid call */
	}
	if (value < 0) {
		fprintf(stderr,"error: invalid call to get_trit: value\n");
		exit(1); /* invalid call */
	}
	for (i=0;i<position;i++) {
		value /= 3;
	}
	switch (value%3) {
		case 2:
			return T2;
		case 1:
			return T1;
		default:
			return T0;
	}
}

long long set_trit(Trit trit, unsigned int position, long long value) {
	Trit current = get_trit(position, value);
	long long pos_val = 1;
	unsigned int i;
	if (current == trit) {
		return value;
	}
	for (i=0;i<position;i++) {
		pos_val *= 3;
	}
	switch (current) {
		case T2:
			value -= pos_val;
		case T1:
			value -= pos_val;
		default:
			break;
	}
	switch (trit) {
		case T2:
			value += pos_val;
		case T1:
			value += pos_val;
		default:
			break;
	}
	return value;
}


int initialize_entry_point(Trit entry_offset, long long entry, unsigned int rot_width_at_least, long long c_start_offset) {

	unsigned int i;
	long long reset_point = current_code_size;
	long long max_value_C1_rot_width = 1;
	long long inner_reset_point;
	long long address_gen_size;

	if (entry_offset == T2 || (entry_offset == T0 && entry <= 0)) {
		/* invalid address */
		fprintf(stderr,"error: invalid entry point\n");
		exit(1);
	}

	if (entry_offset == T1 && entry > -41 && entry < 38) {
		fprintf(stderr,"error: entry point in reserved memory area\n");
		exit(1);
	}

	if (entry_offset == T0) {
		if (min_t0_cell == -1 || min_t0_cell < entry) {
			min_t0_cell = entry;
		}
	}

	if (rot_width_at_least < needed_backjump_rotwidth) {
		rot_width_at_least = needed_backjump_rotwidth;
	}


	/* get minimal necessary rot width: */
	/* compute maximum possible constant */
	for (i=1;i<rot_width_at_least;i++) {
		max_value_C1_rot_width *= 3;
		max_value_C1_rot_width += 1;
	}
	/* check whether the rot width is enough or not... */
	while ((entry_offset == T1 && (entry >= max_value_C1_rot_width || -entry+1 >= max_value_C1_rot_width))
			|| (entry >= 2*max_value_C1_rot_width)
			) {
		rot_width_at_least++;
		max_value_C1_rot_width *= 3;
		max_value_C1_rot_width += 1;
	}
	/* HINT: rot-width is not checked / cannot be checked for backjump-address here */

	if (rot_width_at_least > min_rot_width) {
		/* increase rot-width. */
		needed_backjump_rotwidth = (needed_backjump_rotwidth>rot_width_at_least?needed_backjump_rotwidth:rot_width_at_least);
		return -1; /* BREAK UP, RESET EVERYTHING, INCREASE MIN_ROT_WIDTH, THEN START AGAIN!!! */
	}


	/* generate entry point in var2, return-address (c-register) in var3 */
	/* then call ROTATION-MODULE */
	/* at last: jump to [var2], jump, jmp_c */

	/* set rotation-width */
	for (i=0;i<rot_width_at_least;i++) { /* width */
		add_code("ooooooooj*jo");
	}


	/* set var1 to ...101120 = C1 - 0t02221 = C1 - 2*27 - 2*9 - 2*3 - 1 */
	/* vermutlich, um dann ...10112 in ...120 zu schreiben, um dort ein Jmp zu erzeugen?? */
	add_code("*oooojpjoooooojpjo");
		{

			long long gen = 0;
			long long tmp = max_value_C1_rot_width - 2*27 - 2*9 - 2*3 - 1; /* OFFSET-CALCULATION */
			long long factor = 1;
			long long i;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==2) {
					gen += 2*factor;
				}else{
					gen += factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 1);
			add_code("oojpjo");
			gen = 0;
			tmp = max_value_C1_rot_width - 2*27 - 2*9 - 2*3 - 1; /* OFFSET-CALCULATION */
			factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==0) {
					gen += 2*factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 1);
			add_code("oj*op");
			add_code("oojpjo");
		}


	/* set var2 */
	add_code("*ooooojpjooooooojpjo");
	switch (entry_offset) {
		case T0:
		{

			long long gen = 0;
			long long tmp = entry-1;
			long long factor = 1;
			long long i;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==0) {
					gen += factor;
				}else{
					gen += 2*factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 0);
			add_code("ooojpjo");
			gen = 0;
			tmp = entry-1;
			factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==1) {
					gen += 2*factor;
				}else{
					gen += factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 0);
			add_code("ooojpjo");
			break;
		}
		case T1:
		{

			long long gen = 0;
			long long tmp = entry + max_value_C1_rot_width - 1; /* OFFSET-CALCULATION */
			long long factor = 1;
			long long i;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==2) {
					gen += 2*factor;
				}else{
					gen += factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 0);
			add_code("ooojpjo");
			gen = 0;
			tmp = entry + max_value_C1_rot_width - 1; /* OFFSET-CALCULATION */
			factor = 1;
			for (i=0;i<rot_width_at_least;i++) {
				if (tmp % 3==0) {
					gen += 2*factor;
				}
				factor *= 3;
				tmp /= 3;
			}
			set_gen_init_03(gen, rot_width_at_least, 0);
			add_code("oj*op");
			add_code("ooojpjo");
			break;
		}
		default:
			fprintf(stderr,"error: invalid address (stage 2)\n");
			exit(1); /* error!! */
	}


	/* NOW: GENERATE BACKJUMP ADDRESS! */

	inner_reset_point = current_code_size;
	address_gen_size = 150; /* start with this */
	do {
		long long backjump_address = c_start_offset + current_code_size + address_gen_size;

		if (backjump_address > 2*max_value_C1_rot_width) {
			needed_backjump_rotwidth = rot_width_at_least+1;
			/* restart THIS function with new rot-width */
			/* DELETE EVERYTHING DONE IN THIS FUNCTION!!! */
			current_code_size = reset_point;
			code[current_code_size] = 0;
			return initialize_entry_point(entry_offset, entry, needed_backjump_rotwidth,c_start_offset);
		}

		add_code("*oooooojpjoooooooojpjo");

		long long gen = 0;
		long long tmp = backjump_address;
		long long factor = 1;
		long long i;
		for (i=0;i<rot_width_at_least;i++) {
			if (tmp % 3==0) {
				gen += factor;
			}else{
				gen += 2*factor;
			}
			factor *= 3;
			tmp /= 3;
		}
		set_gen_init_03(gen, rot_width_at_least, 0);
		/*printf("bja1-gen: %d\n",gen); */
		add_code("oooojpjo");
		gen = 0;
		tmp = backjump_address;
		factor = 1;
		for (i=0;i<rot_width_at_least;i++) {
			if (tmp % 3==1) {
				gen += 2*factor;
			}else{
				gen += factor;
			}
			factor *= 3;
			tmp /= 3;
		}
		set_gen_init_03(gen, rot_width_at_least, 0);
		/*printf("bja2-gen: %d\n",gen); */
		add_code("oooojpjo");
		add_code("oooooooooji");
		if (backjump_address+1 >= c_start_offset + current_code_size) {
			while (backjump_address+1 > c_start_offset + current_code_size) {
				add_code("o");
			}
			add_code("jo");
			/*printf("bja: %d\n",backjump_address); */
			break;
		}
		address_gen_size += c_start_offset + current_code_size - backjump_address;
		current_code_size = inner_reset_point;
		code[inner_reset_point] = 0;
	}while(1);
	return 0;
}

