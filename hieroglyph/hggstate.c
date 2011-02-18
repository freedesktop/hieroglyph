/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hggsate.c
 * Copyright (C) 2006-2011 Akira TAGOH
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

#include <string.h>
#include "hgarray.h"
#include "hgint.h"
#include "hgmem.h"
#include "hgreal.h"
#include "hggstate.h"

#include "hggstate.proto.h"

HG_DEFINE_VTABLE_WITH (gstate, NULL, NULL, NULL);

/*< private >*/
static hg_usize_t
_hg_object_gstate_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_gstate_t));
}

static hg_uint_t
_hg_object_gstate_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static hg_bool_t
_hg_object_gstate_initialize(hg_object_t *object,
			     va_list      args)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object;

	gstate->qpath = Qnil;
	gstate->qclippath = Qnil;
	gstate->qdashpattern = Qnil;

	return TRUE;
}

static hg_quark_t
_hg_object_gstate_copy(hg_object_t             *object,
		       hg_quark_iterate_func_t  func,
		       hg_pointer_t             user_data,
		       hg_pointer_t            *ret)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object, *g = NULL;
	hg_quark_t retval;

	hg_return_val_if_fail (object->type == HG_TYPE_GSTATE, Qnil, HG_e_typecheck);

	if (object->on_copying != Qnil)
		return object->on_copying;

	object->on_copying = retval = hg_gstate_new(gstate->o.mem, (hg_pointer_t *)&g);
	if (retval != Qnil) {
		memcpy(&g->ctm, &gstate->ctm, sizeof (hg_gstate_t) - sizeof (hg_object_t) - (sizeof (hg_quark_t) * 3));
		if (gstate->qpath == Qnil) {
			g->qpath = Qnil;
		} else {
			g->qpath = func(gstate->qpath, user_data, NULL);
			if (g->qpath == Qnil) {
				hg_debug(HG_MSGCAT_GSTATE, "Unable to copy the path object");
				goto bail;
			}
			hg_mem_reserved_spool_remove(gstate->o.mem,
						     g->qpath);
		}
		if (gstate->qclippath == Qnil) {
			g->qclippath = Qnil;
		} else {
			g->qclippath = func(gstate->qclippath, user_data, NULL);
			if (g->qclippath == Qnil) {
				hg_debug(HG_MSGCAT_GSTATE, "Unable to copy the clippath object");
				goto bail;
			}
			hg_mem_reserved_spool_remove(gstate->o.mem,
						     g->qclippath);
		}
		if (gstate->qdashpattern == Qnil) {
			g->qdashpattern = Qnil;
		} else {
			g->qdashpattern = func(gstate->qdashpattern, user_data, NULL);
			if (g->qdashpattern == Qnil) {
				hg_debug(HG_MSGCAT_GSTATE, "Unable to copy the dashpattern object");
				goto bail;
			}
			hg_mem_reserved_spool_remove(gstate->o.mem,
						     g->qdashpattern);
		}

		if (ret)
			*ret = g;
		else
			hg_mem_unlock_object(g->o.mem, retval);
	}
	goto finalize;
  bail:
	if (g) {
		hg_object_free(g->o.mem, retval);

		retval = Qnil;
	}
  finalize:
	object->on_copying = Qnil;

	return retval;
}

static hg_char_t *
_hg_object_gstate_to_cstr(hg_object_t             *object,
			  hg_quark_iterate_func_t  func,
			  hg_pointer_t             user_data)
{
	return g_strdup("-gstate-");
}

static hg_bool_t
_hg_object_gstate_gc_mark(hg_object_t          *object,
			  hg_gc_iterate_func_t  func,
			  hg_pointer_t          user_data)
{
	hg_gstate_t *gstate = (hg_gstate_t *)object;

	if (!func(gstate->qpath, user_data))
		return FALSE;
	if (!func(gstate->qclippath, user_data))
		return FALSE;
	if (!func(gstate->qdashpattern, user_data))
		return FALSE;

	return TRUE;
}

static hg_bool_t
_hg_object_gstate_compare(hg_object_t             *o1,
			  hg_object_t             *o2,
			  hg_quark_compare_func_t  func,
			  hg_pointer_t             user_data)
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
hg_gstate_new(hg_mem_t     *mem,
	      hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_gstate_t *gstate = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);

	retval = hg_object_new(mem, (hg_pointer_t *)&gstate, HG_TYPE_GSTATE, 0);

	if (ret)
		*ret = gstate;
	else
		hg_mem_unlock_object(mem, retval);

	return retval;
}

/**
 * hg_gstate_set_ctm:
 * @gstate:
 * @matrix:
 *
 * FIXME
 */
void
hg_gstate_set_ctm(hg_gstate_t *gstate,
		  hg_matrix_t *matrix)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (matrix != NULL, HG_e_typecheck);

	memcpy(&gstate->ctm, matrix, sizeof (hg_matrix_t));
}

/**
 * hg_gstate_get_ctm:
 * @gstate:
 * @matrix:
 *
 * FIXME
 */
void
hg_gstate_get_ctm(hg_gstate_t *gstate,
		  hg_matrix_t *matrix)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (matrix != NULL, HG_e_typecheck);

	memcpy(matrix, &gstate->ctm, sizeof (hg_matrix_t));
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
	hg_uint_t mem_id;

	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (HG_IS_QPATH (qpath), HG_e_typecheck);

	mem_id = hg_quark_get_mem_id(gstate->o.self);

	hg_return_if_fail (hg_quark_has_mem_id(qpath, mem_id), HG_e_VMerror);

	gstate->qpath = qpath;
	hg_mem_reserved_spool_remove(gstate->o.mem, qpath);
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
	hg_return_val_if_fail (gstate != NULL, Qnil, HG_e_typecheck);

	return gstate->qpath;
}

/**
 * hg_gstate_set_clippath:
 * @gstate:
 * @qpath:
 *
 * FIXME
 */
void
hg_gstate_set_clippath(hg_gstate_t *gstate,
		       hg_quark_t   qpath)
{
	hg_uint_t mem_id;

	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (HG_IS_QPATH (qpath), HG_e_typecheck);

	mem_id = hg_quark_get_mem_id(gstate->o.self);

	hg_return_if_fail (hg_quark_has_mem_id(qpath, mem_id), HG_e_VMerror);

	gstate->qclippath = qpath;
	hg_mem_reserved_spool_remove(gstate->o.mem, qpath);
}

/**
 * hg_gstate_get_clippath:
 * @gstate:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_gstate_get_clippath(hg_gstate_t *gstate)
{
	hg_return_val_if_fail (gstate != NULL, Qnil, HG_e_typecheck);

	return gstate->qclippath;
}

/**
 * hg_gstate_set_rgbcolor:
 * @gstate:
 * @red:
 * @green:
 * @blue:
 *
 * FIXME
 */
void
hg_gstate_set_rgbcolor(hg_gstate_t *gstate,
		       hg_real_t    red,
		       hg_real_t    green,
		       hg_real_t    blue)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);

	gstate->color.type = HG_COLOR_RGB;
	gstate->color.is.rgb.red = red;
	gstate->color.is.rgb.green = green;
	gstate->color.is.rgb.blue = blue;
}

/**
 * hg_gstate_set_hsbcolor:
 * @gstate:
 * @hue:
 * @saturation:
 * @brightness:
 *
 * FIXME
 */
void
hg_gstate_set_hsbcolor(hg_gstate_t *gstate,
		       hg_real_t    hue,
		       hg_real_t    saturation,
		       hg_real_t    brightness)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);

	gstate->color.type = HG_COLOR_HSB;
	gstate->color.is.hsb.hue = hue;
	gstate->color.is.hsb.saturation = saturation;
	gstate->color.is.hsb.brightness = brightness;
}

/**
 * hg_gstate_set_gray:
 * @gstate:
 * @gray:
 *
 * FIXME
 */
void
hg_gstate_set_graycolor(hg_gstate_t *gstate,
			hg_real_t    gray)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);

	gstate->color.type = HG_COLOR_GRAY;
	gstate->color.is.rgb.red = gray;
	gstate->color.is.rgb.green = gray;
	gstate->color.is.rgb.blue = gray;
}

/**
 * hg_gstate_set_linewidth:
 * @gstate:
 * @width:
 *
 * FIXME
 */
void
hg_gstate_set_linewidth(hg_gstate_t *gstate,
			hg_real_t    width)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);

	gstate->linewidth = width;
}

/**
 * hg_gstate_set_linecap:
 * @gstate:
 * @linecap:
 *
 * FIXME
 */
void
hg_gstate_set_linecap(hg_gstate_t  *gstate,
		      hg_linecap_t  linecap)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (linecap >= 0 && linecap < HG_LINECAP_END, HG_e_rangecheck);

	gstate->linecap = linecap;
}

/**
 * hg_gstate_set_linejoin:
 * @gstate:
 * @linejoin:
 *
 * FIXME
 */
void
hg_gstate_set_linejoin(hg_gstate_t   *gstate,
		       hg_linejoin_t  linejoin)
{
	hg_return_if_fail (gstate != NULL, HG_e_typecheck);
	hg_return_if_fail (linejoin >= 0 && linejoin < HG_LINEJOIN_END, HG_e_rangecheck);

	gstate->linejoin = linejoin;
}

/**
 * hg_gstate_set_miterlimit:
 * @gstate:
 * @miterlen:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_set_miterlimit(hg_gstate_t *gstate,
			 hg_real_t    miterlen)
{
	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);

	if (miterlen < 1.0)
		return FALSE;

	gstate->miterlen = miterlen;

	return TRUE;
}

/**
 * hg_gstate_set_dash:
 * @gstate:
 * @qpattern:
 * @offset:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_gstate_set_dash(hg_gstate_t *gstate,
		   hg_quark_t   qpattern,
		   hg_real_t    offset)
{
	hg_array_t *a;
	hg_mem_t *mem;
	hg_uint_t id;
	hg_usize_t i;
	hg_bool_t is_zero = TRUE;

	hg_return_val_if_fail (gstate != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (HG_IS_QARRAY (qpattern), FALSE, HG_e_typecheck);

	/* initialize hg_errno to estimate properly */
	hg_errno = 0;

	id = hg_quark_get_mem_id(qpattern);

	hg_return_val_if_fail ((mem = hg_mem_spool_get(id)) != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_lock_fail (a, mem, qpattern, FALSE);

	if (hg_array_length(a) > 11) {
		hg_debug(HG_MSGCAT_GSTATE, "Array size for dash pattern is too big");
		hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_limitcheck);
		goto finalize;
	}
	if (hg_array_length(a) > 0) {
		for (i = 0; i < hg_array_length(a); i++) {
			hg_quark_t q = hg_array_get(a, i);

			if (!HG_ERROR_IS_SUCCESS0 ())
				return FALSE;
			if (HG_IS_QINT (q)) {
				if (HG_INT (q) != 0)
					is_zero = FALSE;
			} else if (HG_IS_QREAL (q)) {
				if (!HG_REAL_IS_ZERO (q))
					is_zero = FALSE;
			} else {
				hg_debug(HG_MSGCAT_GSTATE, "Dash pattern contains non-numeric.");
				hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_typecheck);
				goto finalize;
			}
		}
		if (is_zero) {
			hg_debug(HG_MSGCAT_GSTATE, "No patterns in Array");
			hg_errno = HG_ERROR_ (HG_STATUS_FAILED, HG_e_rangecheck);
			goto finalize;
		}
	}

	gstate->qdashpattern = qpattern;
	gstate->dash_offset = offset;
  finalize:
	hg_mem_unlock_object(mem, qpattern);

	return HG_ERROR_IS_SUCCESS0 ();
}
