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

%{
#include "malbolge.h"
#include "typedefs.h"

#include "lmfao.tab.h"

#define YYABORT return 1;


#ifndef YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
#define YYLTYPE_IS_DECLARED
#endif
extern YYLTYPE yylloc;


YYLTYPE last_curly_bracket;

#define COPY_CODE_POSITION(to, from_start, from_end) {(to).first_line = (from_start).first_line;(to).first_column = (from_start).first_column;(to).last_line = (from_end).last_line;(to).last_column = (from_end).last_column;};

static void update_loc(){
  static int curr_line = 1;
  static int curr_col  = 1;

  yylloc.first_line   = curr_line;
  yylloc.first_column = curr_col;

  {char * s; for(s = yytext; *s != '\0'; s++){
    if(*s == '\n'){
      curr_line++;
      curr_col = 1;
    }else{
      curr_col++;
    }
  }}

  yylloc.last_line   = curr_line;
  yylloc.last_column = curr_col-1;
}

#define YY_USER_ACTION update_loc();


int yyerror(const char *s);

/* force whitespaces if needed. */
int require_whitespace = 0;
int in_data_section = 0;
#define CHECK_WHITESPACE() if(require_whitespace){yyerror("Misformed identifier"); YYABORT;}
#define REQUIRE_WHITESPACE() require_whitespace = 1;

char suppress_emptyline = 0;
%}
%option nounput
%option noinput
%option yylineno

trit        [0-2]
tritail     ({trit})*
constant    (C|\.\.\.)(0|1|2){tritail}|(0|1|2)t{tritail}
digit       [0-9]
decimal     {digit}+
chr         [!-&(-\[\]-~ ]|\\'|\\n|\\r|\\t|\\\\
character   '({chr})'
command     MovD|Nop|Jmp|In|Out|Opr|Rot|Hlt
identifier  [A-Za-z_][0-9A-Za-z_]{0,99}
label       {identifier}:
whitespace  [ \t\r]
comment     ([;%#]|\/\/)[^\n]*|\/\*\/*(([^\*\/]\/*)*\*+)+\/
part        [\n]{whitespace}*[\n]
section     \.(CODE|DATA)
offset      \.OFFSET|@
uprefixed   U_{identifier}
rprefixed   R_{identifier}
string      \"(\\.|[^\"\\\r\n])*\"

%%
{comment} {
        // ignore
    }

{string} {
        CHECK_WHITESPACE();
        yylval.s_val = strdup(yytext);
        REQUIRE_WHITESPACE();
        return STRING;
    }

,   {
        require_whitespace=0;
        return COMMA;
    }

{character} {
        CHECK_WHITESPACE();
        yylval.ternary.offset = T0;
        if (yytext[1] == '\\') {
            switch (yytext[2]) {
                case 'n':
                    yylval.ternary.val = '\n'; break;
                case 'r':
                    yylval.ternary.val = '\r'; break;
                case 't':
                    yylval.ternary.val = '\t'; break;
                case '\\':
                    yylval.ternary.val = '\\'; break;
            }
        }else{
            yylval.ternary.val = (long long)((unsigned char)(yytext[1]));
        }
        REQUIRE_WHITESPACE();
        return CONSTANT;
    }

\{  {
        require_whitespace = 0;
        if (suppress_emptyline == 0) {
            suppress_emptyline = 1;
            COPY_CODE_POSITION(last_curly_bracket,yylloc,yylloc);
            return EMPTYLINE;
        } else {
            yyerror("Opening curly bracket found, but previous curly bracket has not been closed");
            fprintf(stderr,"Previous curly bracket was at line %d column %d.\n",
                    last_curly_bracket.first_line, last_curly_bracket.first_column);
            YYABORT;
        }
    }

\}  {
        require_whitespace = 0;
        if (suppress_emptyline == 1) {
            suppress_emptyline = 0;
            COPY_CODE_POSITION(last_curly_bracket,yylloc,yylloc);
            return EMPTYLINE;
        } else {
            yyerror("Closing curly bracket without opening curly bracket found");
            YYABORT;
        }
    }

{constant} {
        int offset = 0;
        yylval.ternary.val = 0;
        CHECK_WHITESPACE();
        switch (yytext[0]) {
            case 'C':
                yylval.ternary.offset = (yytext[1]=='2'?T2:(yytext[1]=='1'?T1:T0));
                offset = 2;
                break;
            case '.':
                yylval.ternary.offset = (yytext[3]=='2'?T2:(yytext[3]=='1'?T1:T0));
                offset = 4;
                break;
            case '2':
                yylval.ternary.offset = T2;
                offset = 2;
                break;
            case '1':
                yylval.ternary.offset = T1;
                offset = 2;
                break;
            case '0':
                yylval.ternary.offset = T0;
                offset = 2;
                break;
            default:
                yyerror("Unexpected character");
                YYABORT;
                break;
        }
        for(;yytext[offset]!=0;offset++) {
            yylval.ternary.val*=3;
            yylval.ternary.val+=yytext[offset]-'0'-(yylval.ternary.offset==T2?2:(yylval.ternary.offset==T1?1:0));
        }
        REQUIRE_WHITESPACE();
        return CONSTANT;
    }

{decimal} {
        long long num=0; int position=0;
        CHECK_WHITESPACE();
        for(;yytext[position]!=0;position++) { num*=10; num+=yytext[position]-'0'; }
        yylval.ternary.offset = T0;
        yylval.ternary.val = num;
        REQUIRE_WHITESPACE();
        return CONSTANT;
    }

RNop {
        CHECK_WHITESPACE();
        REQUIRE_WHITESPACE();
        return RNOP;
    }

{command}: {
        yyerror("Label must not be a Malbolge command"); YYABORT;
    }

{command} {
        CHECK_WHITESPACE();
        /* identify command */
        if      (!strncmp(yytext,"Nop",4))  yylval.c_val = MALBOLGE_COMMAND_NOP;
        else if (!strncmp(yytext,"MovD",5)) yylval.c_val = MALBOLGE_COMMAND_MOVED;
        else if (!strncmp(yytext,"Jmp",4))  yylval.c_val = MALBOLGE_COMMAND_JMP;
        else if (!strncmp(yytext,"In",3))   yylval.c_val = MALBOLGE_COMMAND_IN;
        else if (!strncmp(yytext,"Out",4))  yylval.c_val = MALBOLGE_COMMAND_OUT;
        else if (!strncmp(yytext,"Hlt",4))  yylval.c_val = MALBOLGE_COMMAND_HALT;
        else if (!strncmp(yytext,"Opr",4))  yylval.c_val = MALBOLGE_COMMAND_OPR;
        else if (!strncmp(yytext,"Rot",4))  yylval.c_val = MALBOLGE_COMMAND_ROT;
        else    {yyerror("Internal error"); YYABORT;}
        REQUIRE_WHITESPACE();
        return COMMAND;
    }

{uprefixed}: {yyerror("Label must not start with U_"); YYABORT;}
{rprefixed}: {yyerror("Label must not start with R_"); YYABORT;}

{label} {
        char identifier[101]; int len = (int)strlen(yytext);
        CHECK_WHITESPACE();
        memcpy(identifier,yytext,len-1); identifier[len-1] = 0; /* remove colon */
        yylval.s_val = strdup(identifier);
        return LABEL;
    }

{uprefixed} {
        CHECK_WHITESPACE();
        yylval.s_val = strdup(yytext+2); /* remove U_-prefix */
        REQUIRE_WHITESPACE();
        return U_PREFIXED_IDENTIFIER;
    }

{rprefixed}    {CHECK_WHITESPACE();
        yylval.s_val = strdup(yytext+2); /* remove R_-prefix */
        REQUIRE_WHITESPACE();
        return R_PREFIXED_IDENTIFIER;}

{identifier} {
        CHECK_WHITESPACE();
        yylval.s_val = strdup(yytext);
        REQUIRE_WHITESPACE();
        return IDENTIFIER;
    }

\/  {
        require_whitespace = 0;
        if (in_data_section == 1) {
            yylval.c_val = '/';
            return MULDIV;
        }
        return SLASH;
    }

{section} {
            if (suppress_emptyline == 0) {
            CHECK_WHITESPACE();
            REQUIRE_WHITESPACE();
            if (yytext[1]=='C') { /* .CODE or .DATA */
                in_data_section = 0;
                return CSEC;
            } else {
                in_data_section = 1;
                return DSEC;
            }
        }else{
            if (yytext[1]=='C') { /* .CODE or .DATA */
                yyerror("Unexpected .CODE inside curly bracktes");
                YYABORT;
            }else{
                yyerror("Unexpected .DATA inside curly bracktes");
                YYABORT;
            }
        }
    }

({part})+ {
        require_whitespace = 0;
        if (suppress_emptyline == 0) {
            return EMPTYLINE;
        }
    }

{offset}    {if (yytext[0]!='@') {CHECK_WHITESPACE();
        REQUIRE_WHITESPACE();}else{require_whitespace = 0;}
        return OFFSET;}

\?\-? {
        CHECK_WHITESPACE();
        REQUIRE_WHITESPACE();
        if (yytext[1] == '-') {
            return NOTUSED;
        }
        return DONTCARE;
    }

\+|- {
        yylval.c_val = yytext[0];
        require_whitespace=0;
        return PLUSMINUS;
    }

\* {
        yylval.c_val = '*';
        require_whitespace=0;
        return MULDIV;
    }

>>|<< {
        yylval.c_val = yytext[0];
        require_whitespace=0;
        return SHIFT;
    }

!   {
        require_whitespace=0;
        return CRAZY;
    }

\(  {
        return BRACKETLEFT;
    }

\)  {
        return BRACKETRIGHT;
    }

{whitespace} {
        require_whitespace = 0;
    }

\n {
        require_whitespace = 0;
    }

<<EOF>> {
        if (suppress_emptyline == 1) {
            yyerror("Missing closing curly bracket at EOF");
            fprintf(stderr,"Opening curly bracket was at line %d column %d.\n",
                    last_curly_bracket.first_line, last_curly_bracket.first_column);
            YYABORT;
        } else {
            yyterminate();
        }
    }

.   {
        yyerror("Unexpected character");
        YYABORT;
    }


