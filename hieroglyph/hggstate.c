/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hggsate.c
 * Copyright (C) 2006-2010 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hgmem.h"
#include "hggstate.h"


HG_DEFINE_VTABLE_WITH (gstate, NULL, NULL, NULL);

/*< private >*/
static gsize
_hg_object_gstate_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_gstate_t));
}

static guint
_hg_object_gstate_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static gboolean
_hg_object_gstate_initialize(hg_object_t *object,
			     va_list      args)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object;

	gstate->qpath = Qnil;

	return TRUE;
}

static hg_quark_t
_hg_object_gstate_copy(hg_object_t              *object,
		       hg_quark_iterate_func_t   func,
		       gpointer                  user_data,
		       gpointer                 *ret,
		       GError                  **error)
{
	return Qnil;
}

static gchar *
_hg_object_gstate_to_cstr(hg_object_t              *object,
			  hg_quark_iterate_func_t   func,
			  gpointer                  user_data,
			  GError                  **error)
{
	return NULL;
}

static gboolean
_hg_object_gstate_gc_mark(hg_object_t           *object,
			  hg_gc_iterate_func_t   func,
			  gpointer               user_data,
			  GError               **error)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object;
	GError *err = NULL;
	gboolean retval = TRUE;

	if (!func(gstate->qpath, user_data, &err))
		goto finalize;

  finalize:
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		} else {
			g_warning("%s: %s (code: %d)",
				  __PRETTY_FUNCTION__,
				  err->message,
				  err->code);
		}
		g_error_free(err);
		retval = FALSE;
	}

	return retval;
}

static gboolean
_hg_object_gstate_compare(hg_object_t             *o1,
			  hg_object_t             *o2,
			  hg_quark_compare_func_t  func,
			  gpointer                 user_data)
{
	return FALSE;
}


/*< public >*/
/**
 * hg_gstate_new:
 * @mem:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_gstate_new(hg_mem_t *mem,
	      gpointer *ret)
{
	hg_quark_t retval;
	hg_gstate_t *gstate = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil);

	retval = hg_object_new(mem, (gpointer *)&gstate, HG_TYPE_GSTATE, 0);

	if (ret)
		*ret = gstate;
	else
		hg_mem_unlock_object(mem, retval);

	return retval;
}

/**
 * hg_gstate_set_path:
 * @gstate:
 * @qpath:
 *
 * FIXME
 */
void
hg_gstate_set_path(hg_gstate_t *gstate,
		   hg_quark_t   qpath)
{
	hg_return_if_fail (gstate != NULL);
	hg_return_if_fail (HG_IS_QPATH (qpath));

	gstate->qpath = qpath;
}

/**
 * hg_gstate_get_path:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_gstate_get_path(hg_gstate_t *gstate)
{
	hg_return_val_if_fail (gstate != NULL, Qnil);

	return gstate->qpath;
}