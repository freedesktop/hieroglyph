/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgscanner.l
 * Copyright (C) 2010 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
%{
#include <ctype.h>
#include <math.h>
#include <glib.h>
#include "hgerror.h"
#include "hgmem.h"
#include "hgint.h"
#include "hgreal.h"
#include "hgscanner.h"


#define HG_SCANNER_MEM_SIZE	100000
#define YY_DECL			static gboolean _hg_scanner_yylex(hg_scanner_t *scanner, yyscan_t yyscanner, hg_mem_t *mem, GError **error)
#define yyterminate()		{BEGIN(INITIAL);return FALSE;}
#define YY_NO_INPUT
#define YY_INPUT(_buf_,_result_,_max_size_)			\
	G_STMT_START {						\
		hg_scanner_t *_scanner_ = yyextra;		\
								\
		_result_ = hg_file_read(_scanner_->infile,	\
					(_buf_),		\
					1,			\
					(_max_size_),		\
					NULL);			\
		g_print("%d\n", _result_);			\
	} G_STMT_END
#define YY_EXTRA_TYPE hg_scanner_t *
#define YY_USER_INIT	yyextra = scanner;


struct _hg_scanner_t {
	yyscan_t    yyscanner;
	hg_mem_t   *mem;
	hg_name_t  *name;
	hg_quark_t  self;
	hg_quark_t  result;
	hg_file_t  *infile;
};

static gboolean _hg_scanner_yylex(hg_scanner_t  *scanner,
				  yyscan_t       yyscanner,
				  hg_mem_t      *mem,
				  GError       **error);
#ifndef yyget_column
int yyget_column(yyscan_t yyscanner);
#endif
#ifndef yyset_column
void yyset_column(int column_no, yyscan_t yyscanner);
#endif
%}
%option nounput noyywrap warn nounistd reentrant
%x STRING HEXSTRING ASCII85

CONTROL		[\x04%\(\)/<>\[\]\{\}]
SPACE		[\t\f\r\n ]
HEXDIGITS	[0-9a-fA-F]
%%
	gint string_count = 0;
	GString *string = NULL;
	static const gchar *const radix_index = "0123456789abcdefghijklmnopqrstuvwxyz";
	gchar buf_ascii85[8];
	gint ascii85_counter;

\%.*	{
	/* XXX: ignoring all of comments so far */
}
<STRING>{
	"("	{
		string_count++;
		g_string_append_c(string, yytext[0]);
	}
	")"	{
		string_count--;
		if (string_count == 0) {
			gboolean retval = TRUE;

			BEGIN(INITIAL);
			scanner->result = hg_string_new_with_value(mem,
								   NULL,
								   string->str,
								   string->len);
			if (scanner->result == Qnil) {
				g_set_error(error, HG_ERROR, ENOMEM,
					    "Out of memory");
				retval = FALSE;
				/* yyterminate(); */
			}
			g_string_free(string, TRUE);
			return retval;
		} else {
			g_string_append_c(string, yytext[0]);
		}
	}
	\\[0-7]{1,3}	{
		gint result;

		sscanf(yytext + 1,
		       "%o",
		       &result);
		if (result > 0xff) {
			g_set_error(error, HG_ERROR, ERANGE,
				    "Octadecimal code is too big");
			g_string_free(string, TRUE);
			yyterminate();
		}
		g_string_append_c(string, result);
	}
	\\[0-9]+	{
		g_set_error(error, HG_ERROR, EINVAL,
			    "bad octal sequence");
		g_string_free(string, TRUE);
		yyterminate();
	}
	"\\n"		{
		g_string_append_c(string, '\n');
	}
	"\\r"		{
		g_string_append_c(string, '\r');
	}
	"\\t"		{
		g_string_append_c(string, '\t');
	}
	"\\b"		{
		g_string_append_c(string, '\b');
	}
	"\\f"		{
		g_string_append_c(string, '\f');
	}
	\\(.|\n)	{
		g_string_append_c(string, yytext[1]);
	}
	.|\r\n|\r|\n	{
		g_string_append_len(string, yytext, yyleng);
	}
	<<EOF>>		{
		g_set_error(error, HG_ERROR, EINVAL,
			    "Reached the end of the file during scanning the string");
		g_string_free(string, TRUE);
		yyterminate();
	}
}
\(	{
		string_count++;
		BEGIN(STRING);
		string = g_string_new(NULL);
	}
<ASCII85>{
	"~>"	{
		gboolean retval = TRUE;

		BEGIN(INITIAL);
		scanner->result = hg_string_new_with_value(mem,
							   NULL,
							   string->str,
							   string->len);
		if (scanner->result == Qnil) {
			g_set_error(error, HG_ERROR, ENOMEM,
				    "Out of memory");
			retval = FALSE;
			/* yyterminate(); */
		}
		g_string_free(string, TRUE);
		return retval;
	}
	{SPACE}		{
		/* ignore */
	}
	[!-uz]	{
		if (yytext[0] == 'z') {
			if (ascii85_counter == 0) {
				/* special case to take care of all-zero */
				g_string_append_c(string, 0);
				g_string_append_c(string, 0);
				g_string_append_c(string, 0);
				g_string_append_c(string, 0);
			} else {
				g_set_error(error, HG_ERROR, EINVAL,
					    "Syntax error");
				g_string_free(string, TRUE);
				yyterminate();
			}
		} else {
			buf_ascii85[ascii85_counter++] = yytext[0];
			if (ascii85_counter == 5) {
				guint64 decoded = 0;

				decoded += (buf_ascii85[0] - 33) * 0x31c84b1LL;
				decoded += (buf_ascii85[1] - 33) * 0x95eedLL;
				decoded += (buf_ascii85[2] - 33) * 0x1c39LL;
				decoded += (buf_ascii85[3] - 33) * 0x55LL;
				decoded += (buf_ascii85[4] - 33);
				if (decoded > G_MAXUINT32) {
					g_set_error(error, HG_ERROR, EINVAL,
						    "Syntax error");
					g_string_free(string, TRUE);
					yyterminate();
				}
				g_string_append_c(string, (decoded & 0xff000000) >> 24);
				g_string_append_c(string, (decoded & 0x00ff0000) >> 16);
				g_string_append_c(string, (decoded & 0x0000ff00) >> 8);
				g_string_append_c(string, (decoded & 0x000000ff));
				ascii85_counter = 0;
			}
		}
	}
	<<EOF>>		{
		g_set_error(error, HG_ERROR, EINVAL,
			    "Reached the end of the file during scanning the hex string");
		g_string_free(string, TRUE);
		yyterminate();
	}
	.	{
		g_set_error(error, HG_ERROR, EINVAL,
			    "Syntax error");
		g_string_free(string, TRUE);
		yyterminate();
	}
}
\<~	{
	BEGIN(ASCII85);
	string = g_string_new(NULL);
	ascii85_counter = 0;
}
<HEXSTRING>{
	">"	{
		gboolean retval = TRUE;

		BEGIN(INITIAL);
		scanner->result = hg_string_new_with_value(mem,
							   NULL,
							   string->str,
							   string->len);
		if (scanner->result == Qnil) {
			g_set_error(error, HG_ERROR, ENOMEM,
				    "Out of memory");
			retval = FALSE;
			/* yyterminate(); */
		}
		g_string_free(string, TRUE);
		return retval;
	}
	{HEXDIGITS}{1,2}/{HEXDIGITS}|{SPACE}|>	{
		gint i;
		gchar *p;
		gsize index;
		gint32 result = 0;

		for (i = 0; i < yyleng; i++) {
			p = strchr(radix_index, tolower(yytext[i]));
			index = p - radix_index;
			result *= 16;
			result += index;
		}
		if (i == 1)
			result *= 16;
		g_string_append_c(string, result);
	}
	{SPACE}		{
		/* ignore */
	}
	<<EOF>>		{
		g_set_error(error, HG_ERROR, EINVAL,
			    "Reached the end of the file during scanning the hex string");
		g_string_free(string, TRUE);
		yyterminate();
	}
	.	{
		g_set_error(error, HG_ERROR, EINVAL,
			    "Syntax error");
		g_string_free(string, TRUE);
		yyterminate();
	}
}
\<	{
		BEGIN(HEXSTRING);
		string = g_string_new(NULL);
	}
\/\/[^\x04%\(\)/<>\[\]\{\}\t\f\r\n ]+/{CONTROL}|{SPACE}	{
		scanner->result = HG_QEVALNAME (scanner->name, &yytext[2]);
		hg_quark_set_executable(&scanner->result, TRUE);
		return TRUE;
	}
\/[^\x04%\(\)/<>\[\]\{\}\t\f\r\n ]+/{CONTROL}|{SPACE}	{
		scanner->result = HG_QNAME (scanner->name, &yytext[1]);
		return TRUE;
	}
[\x80-\xa1]+		{
		/* binary tokens */
		g_warning("FIXME: binary token appears");
	}
\r\n			{
		/* ignore */
	}
\r			{
		/* ignore */
	}
\n			{
		/* ignore */
	}
[\t\f ]+		{
		/* ignore */
	}
[+-]?[0-9]+\.[0-9]+|[+-]?[0-9]+(\.[0-9]+)?e[+-][0-9]+/{CONTROL}|{SPACE}	{
		gdouble result = 0.0L;

		sscanf(yytext, "%lf", &result);
		scanner->result = HG_QREAL ((gfloat)result);

		return TRUE;
	}
[0-9]+#[0-9a-zA-Z]+/{CONTROL}|{SPACE}	{
		gdouble fradix = 0.0L;
		gint32 result = 0;
		gsize radix, len, i, index;
		gchar *s, *p;

		s = g_new(gchar, yyleng);
		sscanf(yytext, "%lf#%s", &fradix, s);
		radix = fradix;
		if (fradix > G_MAXINT32 ||
		    fradix < G_MININT32 ||
		    radix < 2 ||
		    radix > 36) {
			g_free(s);
			REJECT;
		}
		len = strlen(s);
		for (i = 0; i < len; i++) {
			p = strchr(radix_index, tolower(s[i]));
			index = p - radix_index;
			if (index >= radix) {
				g_free(s);
				REJECT;
			}
			if ((result > 0 && (result * radix) < 0) ||
			    (result < 0 && (result * radix) > 0)) {
				g_set_error(error, HG_ERROR, ERANGE,
					    "digit is too big.");
				g_free(s);
				yyterminate();
			}
			result *= radix;
			result += index;
		}
		g_free(s);
		scanner->result = HG_QINT (result);
		return TRUE;
	}
[+-]?[0-9]+/{CONTROL}|{SPACE}	{
		gsize i, pos = 0;
		gdouble result = 0.0L, f;
		gint sign = 0;

		if (yytext[0] == '+' ||
		    yytext[1] == '-') {
			sign = (yytext[1] == '-' ? -1 : 0);
			pos++;
		}
		for (i = pos; i < yyleng; i++) {
			f = yytext[i] - '0';
			result += f * exp10((gdouble)(yyleng + sign - i - 1));
		}
		if (sign < 0)
			result = -result;
		if (isinf(result) ||
		    isnan(result)) {
			g_set_error(error, HG_ERROR, ERANGE,
				    "digit isn't a valid or too big.");
			yyterminate();
		}
		if (result <= G_MAXINT32 &&
		    result >= G_MININT32) {
			scanner->result = HG_QINT ((gint32)result);
		} else {
			scanner->result = HG_QREAL ((gfloat)result);
		}
		return TRUE;
	}
[^\x04%\(\)/<>\[\]\{\}\t\f\r\n ]+|<<|>>|\[|\]|\{|\}/{CONTROL}|{SPACE}	{
		scanner->result = HG_QNAME (scanner->name, yytext);
		hg_quark_set_executable(&scanner->result, TRUE);
		return TRUE;
	}
<<EOF>>		{
		g_set_error(error, HG_ERROR, EINVAL,
			    "Reached the end of the file during scanning a token");
		yyterminate();
	}
%%
/**
 * hg_scanner_new:
 * @mem:
 * @name:
 *
 * FIXME
 *
 * Returns:
 */
hg_scanner_t *
hg_scanner_new(hg_mem_t  *mem,
	       hg_name_t *name)
{
	hg_scanner_t *retval;
	hg_quark_t q;

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (name != NULL, NULL);

	q = hg_mem_alloc(mem, sizeof (hg_scanner_t), (gpointer *)&retval);
	if (q == Qnil)
		return NULL;
	memset(retval, 0, sizeof (hg_scanner_t));
	retval->mem = mem;
	retval->name = name;
	retval->self = q;
	retval->result = Qnil;

	yylex_init(&retval->yyscanner);
	if (retval->yyscanner == NULL) {
		hg_scanner_destroy(retval);
		return NULL;
	}

	return retval;
}

/**
 * hg_scanner_destroy:
 * @scanner:
 *
 * FIXME
 */
void
hg_scanner_destroy(hg_scanner_t *scanner)
{
	hg_return_if_fail (scanner != NULL);

	if (scanner->yyscanner)
		yylex_destroy(scanner->yyscanner);
	hg_mem_free(scanner->mem, scanner->self);
}

/**
 * hg_scanner_attach_file:
 * @scanner:
 * @file:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_scanner_attach_file(hg_scanner_t *scanner,
		       hg_file_t    *file)
{
	YY_BUFFER_STATE yystate;
	struct yyguts_t *yyg;

	hg_return_val_if_fail (scanner != NULL, FALSE);
	hg_return_val_if_fail (file != NULL, FALSE);

	yyg = (struct yyguts_t *)scanner->yyscanner;
	scanner->infile = file;

	if (YY_CURRENT_BUFFER) {
		yypop_buffer_state(scanner->yyscanner);
	}
	yystate = yy_create_buffer(NULL, YY_BUF_SIZE, scanner->yyscanner);
	yypush_buffer_state(yystate, scanner->yyscanner);
	yyset_lineno(hg_file_get_lineno(file), scanner->yyscanner);

	return TRUE;
}

/**
 * hg_scanner_scan:
 * @scanner:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_scanner_scan(hg_scanner_t  *scanner,
		hg_mem_t      *mem,
		GError       **error)
{
	gboolean retval;

	hg_return_val_with_gerror_if_fail (scanner != NULL, FALSE, error);
	hg_return_val_with_gerror_if_fail (mem != NULL, FALSE, error);
	hg_return_val_if_fail (error != NULL, FALSE);

	scanner->result = Qnil;

	retval = _hg_scanner_yylex(scanner, scanner->yyscanner, mem, error);
	hg_file_set_lineno(scanner->infile, yyget_lineno(scanner->yyscanner));

	return retval;
}

/**
 * hg_scanner_get_token:
 * @scanner:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_scanner_get_token(hg_scanner_t *scanner)
{
	return scanner->result;
}