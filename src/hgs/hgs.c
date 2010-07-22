/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgs.c
 * Copyright (C) 2006-2010 Akira TAGOH
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <hieroglyph/hgvm.h>


/*< private >*/
static gboolean
_hgs_arg_define_cb(const gchar  *option_name,
		   const gchar  *value,
		   gpointer      data,
		   GError      **error)
{
	gboolean retval = FALSE;
	hg_vm_t *vm G_GNUC_UNUSED = data;

	if (value && *value) {
		/* XXX */
		g_print("%s\n", value);
		retval = TRUE;
	}

	return retval;
}

/*< public >*/
int
main(int    argc,
     char **argv)
{
	GError *err = NULL;
	const gchar *psfile = NULL;
	gint errcode = 0;
	hg_vm_t *vm = NULL;
	guint arg_langlevel = HG_LANG_LEVEL_3;
	GOptionContext *ctxt = g_option_context_new("<PostScript file>");
	GOptionGroup *group;
	GOptionEntry entries[] = {
		{"define", 'd', 0, G_OPTION_ARG_CALLBACK, _hgs_arg_define_cb, "Define a variable in systemdict.", "SYMBOL"},
		{"langlevel", 'l', 0, G_OPTION_ARG_INT, &arg_langlevel, "Specify a language level supported on VM", "INT"},
		{NULL}
	};

	hg_vm_init();

	vm = hg_vm_new();
	if (!vm) {
		g_printerr("Out of memory.\n");
		goto finalize;
	}
	group = g_option_group_new(NULL, NULL, NULL, vm, NULL);
	g_option_context_set_main_group(ctxt, group);
	g_option_context_add_main_entries(ctxt, entries, NULL);
	if (!g_option_context_parse(ctxt, &argc, &argv, &err)) {
		if (err) {
			g_print("%s (code: %d)\n",
				err->message,
				err->code);
			g_error_free(err);
		}
		goto finalize;
	}
	if (arg_langlevel >= HG_LANG_LEVEL_END) {
		g_printerr("Unknown language level: %d\n",
			   arg_langlevel);
		goto finalize;
	}

	if (argc > 1)
		psfile = argv[1];

	if (!hg_vm_startjob(vm, arg_langlevel, psfile, TRUE)) {
		g_printerr("Unable to start PostScript VM.");
		goto finalize;
	}
	errcode = hg_vm_get_error_code(vm);

  finalize:
	if (vm)
		hg_vm_destroy(vm);
	g_option_context_free(ctxt);
	hg_vm_tini();

	return errcode;
}