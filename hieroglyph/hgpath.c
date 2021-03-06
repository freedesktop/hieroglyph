/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgpath.c
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

#include "hgmem.h"
#include "hgreal.h"
#include "hgpath.h"

#include "hgpath.proto.h"

#define HG_PATH_MAX	1500

typedef struct _hg_path_bbox_private_t {
	hg_path_bbox_t bbox1;
	hg_path_bbox_t bbox2;
	hg_bool_t      initialized:1;
	hg_bool_t      all_moveto:1;
} hg_path_bbox_private_t;


HG_DEFINE_VTABLE_WITH (path, NULL, NULL, NULL);

/*< private >*/
static hg_usize_t
_hg_object_path_get_capsulated_size(void)
{
	return HG_ALIGNED_TO_POINTER (sizeof (hg_path_t));
}

static guint
_hg_object_path_get_allocation_flags(void)
{
	return HG_MEM_FLAGS_DEFAULT;
}

static hg_bool_t
_hg_object_path_initialize(hg_object_t *object,
			   va_list      args)
{
	hg_path_t *path = (hg_path_t *)object;

	path->length = 0;
	/* initialize node with Qnil first
	 * to avoid a fail on checking the mem id.
	 * when GC is running.
	 */
	path->qnode = Qnil;
	path->qnode = hg_mem_alloc(path->o.mem,
				   sizeof (hg_path_node_t) * HG_PATH_MAX,
				   NULL);
	if (path->qnode == Qnil)
		return FALSE;

	return TRUE;
}

static hg_quark_t
_hg_object_path_copy(hg_object_t             *object,
		     hg_quark_iterate_func_t  func,
		     hg_pointer_t             user_data,
		     hg_pointer_t            *ret)
{
	hg_path_t *path = (hg_path_t *)object, *p = NULL;
	hg_path_node_t *on, *nn;
	hg_quark_t retval;

	hg_return_val_if_fail (object->type == HG_TYPE_PATH, Qnil, HG_e_typecheck);

	if (object->on_copying != Qnil)
		return object->on_copying;

	object->on_copying = retval = hg_path_new(path->o.mem, (hg_pointer_t *)&p);
	if (retval != Qnil) {
		p->length = path->length;
		on = HG_MEM_LOCK (path->o.mem, path->qnode);
		nn = HG_MEM_LOCK (path->o.mem, p->qnode);
		if (on == NULL ||
		    nn == NULL) {
			goto bail;
		}
		memcpy(nn, on, sizeof (hg_path_node_t) * path->length);
		hg_mem_unlock_object(path->o.mem, path->qnode);
		hg_mem_unlock_object(path->o.mem, p->qnode);

		if (ret)
			*ret = p;
		else
			hg_mem_unlock_object(path->o.mem, retval);
	}
	goto finalize;
  bail:
	if (p) {
		hg_object_free(p->o.mem, retval);
		retval = Qnil;
	}
  finalize:
	
	return retval;
}

static hg_char_t *
_hg_object_path_to_cstr(hg_object_t             *object,
			hg_quark_iterate_func_t  func,
			hg_pointer_t             user_data)
{
	return NULL;
}

static hg_bool_t
_hg_object_path_gc_mark(hg_object_t *object)
{
	hg_path_t *path = (hg_path_t *)object;

	hg_return_val_if_fail (object->type == HG_TYPE_PATH, FALSE, HG_e_typecheck);

	hg_debug(HG_MSGCAT_GC, "path: marking node");

	return hg_mem_gc_mark(path->o.mem, path->qnode);
}

static hg_bool_t
_hg_object_path_compare(hg_object_t             *o1,
			hg_object_t             *o2,
			hg_quark_compare_func_t  func,
			hg_pointer_t             user_data)
{
	return FALSE;
}

static hg_bool_t
_hg_path_add(hg_path_t      *path,
	     hg_path_type_t  type,
	     hg_real_t       x,
	     hg_real_t       y)
{
	hg_path_node_t *node;
	hg_real_t cx = 0.0, cy = 0.0;
	hg_bool_t retval = TRUE;

	hg_return_val_if_fail (path != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (type < HG_PATH_END, FALSE, HG_e_rangecheck);
	hg_return_val_if_fail (path->length < HG_PATH_MAX, FALSE, HG_e_limitcheck);

	if (path->length == 0) {
		switch (type) {
		    case HG_PATH_CURVETO:
		    case HG_PATH_LINETO:
		    case HG_PATH_RCURVETO:
		    case HG_PATH_RMOVETO:
		    case HG_PATH_RLINETO:
			    /* /nocurrentpoint would be raised. */
			    hg_error_return (HG_STATUS_FAILED, HG_e_nocurrentpoint);
		    case HG_PATH_CLOSEPATH:
			    /* no errors */
			    hg_error_return (HG_STATUS_SUCCESS, 0);
		    default:
			    break;
		}
	}

	hg_return_val_if_lock_fail (node,
				    path->o.mem,
				    path->qnode,
				    FALSE);

	if (path->length > 0) {
		cx = node[path->length - 1].cx;
		cy = node[path->length - 1].cy;
	}

	switch (type) {
	    case HG_PATH_SETBBOX:
		    break;
	    case HG_PATH_CURVETO:
	    case HG_PATH_LINETO:
	    case HG_PATH_MOVETO:
		    node[path->length].dx = x;
		    node[path->length].dy = y;
		    node[path->length].cx = x;
		    node[path->length].cy = y;
		    break;
	    case HG_PATH_RCURVETO:
	    case HG_PATH_RMOVETO:
	    case HG_PATH_RLINETO:
		    node[path->length].dx = x;
		    node[path->length].dy = y;
		    node[path->length].cx = cx + x;
		    node[path->length].cy = cy + y;
		    break;
	    case HG_PATH_CLOSEPATH:
		    if (node[path->length - 1].type == HG_PATH_CLOSEPATH) {
			    /* nothing to do */
			    goto finalize;
		    }
		    node[path->length].cx = node[0].dx;
		    node[path->length].cy = node[0].dy;
		    break;
	    case HG_PATH_UCACHE:
		    break;
	    default:
		    hg_warning("%s: Unknown path type: %d",
			       __PRETTY_FUNCTION__, type);
		    retval = FALSE;
		    goto finalize;
	}

	node[path->length].type = type;
	path->length++;
  finalize:
	hg_mem_unlock_object(path->o.mem, path->qnode);

	return retval;
}

static void
_hg_path_bbox_nop(hg_pointer_t user_data)
{
}

static void
_hg_path_bbox_moveto(hg_pointer_t user_data,
		     hg_real_t    x,
		     hg_real_t    y)
{
	hg_path_bbox_private_t *priv = (hg_path_bbox_private_t *)user_data;

	if (!priv->initialized) {
		priv->bbox1.llx = priv->bbox2.llx = x;
		priv->bbox1.lly = priv->bbox2.lly = y;
		priv->initialized = TRUE;
	}
	if (x < priv->bbox1.llx) {
		priv->bbox1.llx = x;
		if (priv->all_moveto)
			priv->bbox2.llx = x;
	}
	if (y < priv->bbox1.lly) {
		priv->bbox1.lly = y;
		if (priv->all_moveto)
			priv->bbox2.lly = y;
	}
	if (x > priv->bbox1.urx) {
		priv->bbox1.urx = x;
		if (priv->all_moveto)
			priv->bbox2.urx = x;
	}
	if (y > priv->bbox1.ury) {
		priv->bbox1.ury = y;
		if (priv->all_moveto)
			priv->bbox2.ury = y;
	}
}

static void
_hg_path_bbox_lineto(hg_pointer_t user_data,
		     hg_real_t    x,
		     hg_real_t    y)
{
	hg_path_bbox_private_t *priv = (hg_path_bbox_private_t *)user_data;

	priv->all_moveto = FALSE;
	/* update since only last moveto are ignored in any case */
	memcpy(&priv->bbox2, &priv->bbox1, sizeof (hg_path_bbox_t));
	if (x < priv->bbox1.llx) {
		priv->bbox1.llx = priv->bbox2.llx = x;
	}
	if (y < priv->bbox1.lly) {
		priv->bbox1.lly = priv->bbox2.lly = y;
	}
	if (x > priv->bbox1.urx) {
		priv->bbox1.urx = priv->bbox2.urx = x;
	}
	if (y > priv->bbox1.ury) {
		priv->bbox1.ury = priv->bbox2.ury = y;
	}
}

static void
_hg_path_bbox_curveto(hg_pointer_t user_data,
		      hg_real_t    x1,
		      hg_real_t    y1,
		      hg_real_t    x2,
		      hg_real_t    y2,
		      hg_real_t    x3,
		      hg_real_t    y3)
{
	hg_path_bbox_private_t *priv = (hg_path_bbox_private_t *)user_data;

	priv->all_moveto = FALSE;
	/* update since only last moveto are ignored in any case */
	memcpy(&priv->bbox2, &priv->bbox1, sizeof (hg_path_bbox_t));
	if (x1 < priv->bbox1.llx) {
		priv->bbox1.llx = priv->bbox2.llx = x1;
	}
	if (y1 < priv->bbox1.lly) {
		priv->bbox1.lly = priv->bbox2.lly = y1;
	}
	if (x1 > priv->bbox1.urx) {
		priv->bbox1.urx = priv->bbox2.urx = x1;
	}
	if (y1 > priv->bbox1.ury) {
		priv->bbox1.ury = priv->bbox2.ury = y1;
	}
	if (x2 < priv->bbox1.llx) {
		priv->bbox1.llx = priv->bbox2.llx = x2;
	}
	if (y2 < priv->bbox1.lly) {
		priv->bbox1.lly = priv->bbox2.lly = y2;
	}
	if (x2 > priv->bbox1.urx) {
		priv->bbox1.urx = priv->bbox2.urx = x2;
	}
	if (y2 > priv->bbox1.ury) {
		priv->bbox1.ury = priv->bbox2.ury = y2;
	}
	if (x3 < priv->bbox1.llx) {
		priv->bbox1.llx = priv->bbox2.llx = x3;
	}
	if (y3 < priv->bbox1.lly) {
		priv->bbox1.lly = priv->bbox2.lly = y3;
	}
	if (x3 > priv->bbox1.urx) {
		priv->bbox1.urx = priv->bbox2.urx = x3;
	}
	if (y3 > priv->bbox1.ury) {
		priv->bbox1.ury = priv->bbox2.ury = y3;
	}
}

/*< public >*/
/**
 * hg_path_new:
 * @mem:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_path_new(hg_mem_t     *mem,
	    hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_path_t *path = NULL;

	hg_return_val_if_fail (mem != NULL, Qnil, HG_e_VMerror);

	retval = hg_object_new(mem, (hg_pointer_t *)&path, HG_TYPE_PATH, 0);
	if (retval != Qnil) {
		if (ret)
			*ret = path;
		else
			hg_mem_unlock_object(mem, retval);
	} else {
		hg_error_return_val (Qnil, HG_STATUS_FAILED, HG_e_limitcheck);
	}

	hg_error_return_val (retval, HG_STATUS_SUCCESS, 0);
}

/**
 * hg_path_get_current_point:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_get_current_point(hg_path_t *path,
			  hg_real_t *x,
			  hg_real_t *y)
{
	hg_path_node_t *node;

	hg_return_val_if_fail (path != NULL, FALSE, HG_e_typecheck);

	if (path->length == 0)
		hg_error_return (HG_STATUS_FAILED, HG_e_nocurrentpoint);

	hg_return_val_if_lock_fail (node,
				    path->o.mem,
				    path->qnode,
				    FALSE);

	if (x)
		*x = node[path->length - 1].cx;
	if (y)
		*y = node[path->length - 1].cy;

	hg_mem_unlock_object(path->o.mem, path->qnode);

	hg_error_return (HG_STATUS_SUCCESS, 0);
}

/**
 * hg_path_reverse:
 * @path:
 *
 * FIXME
 *
 * Returns:
 */
#if 0
hg_quark_t
hg_path_reverse(hg_path_t    *path,
		hg_pointer_t *ret)
{
	hg_quark_t retval;
	hg_path_t *new_path;
	hg_path_node_t *node, *new_node;
	hg_bool_t closed = FALSE;

	hg_return_val_if_fail (path != NULL, Qnil, HG_e_typecheck);
	hg_return_val_if_fail (path->length > 0, Qnil, HG_e_VMerror);

	retval = hg_path_new(path->o.mem, (hg_pointer_t *)&new_path);
	if (retval != Qnil) {
		hg_size_t i;

		node = HG_MEM_LOCK (path->o.mem, path->qnode, NULL);
		new_node = HG_MEM_LOCK (new_path->o.mem, new_path->qnode, NULL);
		if (node == NULL || new_node == NULL) {
			retval = Qnil;
			goto finalize;
		}

		if (node[path->length - 1].type == HG_PATH_CLOSEPATH)
			closed = TRUE;

		i = path->length - 1;
		new_node[new_path->length].type = HG_PATH_MOVETO;
		new_node[new_path->length].dx = node[i].cx;
		new_node[new_path->length].dy = node[i].cy;
		new_node[new_path->length].cx = node[i].cx;
		new_node[new_path->length].cy = node[i].cy;
		new_path->length++;

		for (; i >= 0; i++) {
			new_node[new_path->length].type = node[i].type;

			switch (node[i].type) {
			    case HG_PATH_MOVETO:
			    case HG_PATH_LINETO:
				    new_node[new_path->length].dx = node[i - 1].cx;
				    new_node[new_path->length].dy = node[i - 1].cy;
				    new_node[new_path->length].cx = node[i - 1].cx;
				    new_node[new_path->length].cy = node[i - 1].cy;
				    break;
			    case HG_PATH_RMOVETO:
			    case HG_PATH_RLINETO:
				    new_node[new_path->length].dx = -node[i].dx;
				    new_node[new_path->length].dy = -node[i].dy;
				    new_node[new_path->length].cx = node[i].cx - node[i].dx;
				    new_node[new_path->length].cy = node[i].cx - node[i].dy;
				    break;
			    case HG_PATH_CURVETO:
			}
		}

		if (ret) {
			*ret = new_path;
		} else {
		  finalize:
			hg_mem_unlock_object(path->o.mem, retval);
		}
		hg_mem_unlock_object(path->o.mem, path->qnode);
		hg_mem_unlock_object(new_path->o.mem, new_path->qnode);
	}

	return retval;
}
#endif

/**
 * hg_path_close:
 * @path:
 *
 * FIXME
 */
hg_bool_t
hg_path_close(hg_path_t *path)
{
	return _hg_path_add(path, HG_PATH_CLOSEPATH, 0.0, 0.0);
}

/**
 * hg_path_moveto:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_moveto(hg_path_t *path,
	       hg_real_t  x,
	       hg_real_t  y)
{
	return _hg_path_add(path, HG_PATH_MOVETO, x, y);
}

/**
 * hg_path_rmoveto:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_rmoveto(hg_path_t *path,
		hg_real_t  x,
		hg_real_t  y)
{
	return _hg_path_add(path, HG_PATH_RMOVETO, x, y);
}

/**
 * hg_path_lineto:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_lineto(hg_path_t *path,
	       hg_real_t  x,
	       hg_real_t  y)
{
	return _hg_path_add(path, HG_PATH_LINETO, x, y);
}

/**
 * hg_path_rlineto:
 * @path:
 * @x:
 * @y:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_rlineto(hg_path_t *path,
		hg_real_t  x,
		hg_real_t  y)
{
	return _hg_path_add(path, HG_PATH_RLINETO, x, y);
}

/**
 * hg_path_curveto:
 * @path:
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 * @x3:
 * @y3:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_curveto(hg_path_t *path,
		hg_real_t  x1,
		hg_real_t  y1,
		hg_real_t  x2,
		hg_real_t  y2,
		hg_real_t  x3,
		hg_real_t  y3)
{
	hg_bool_t retval = FALSE;

	retval |= !_hg_path_add(path, HG_PATH_CURVETO, x1, y1);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, x2, y2);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, x3, y3);

	return !retval;
}

/**
 * hg_path_rcurveto:
 * @path:
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 * @x3:
 * @y3:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_rcurveto(hg_path_t *path,
		 hg_real_t  x1,
		 hg_real_t  y1,
		 hg_real_t  x2,
		 hg_real_t  y2,
		 hg_real_t  x3,
		 hg_real_t  y3)
{
	hg_bool_t retval = FALSE;

	retval |= !_hg_path_add(path, HG_PATH_RCURVETO, x1, y1);
	retval |= !_hg_path_add(path, HG_PATH_RCURVETO, x2, y2);
	retval |= !_hg_path_add(path, HG_PATH_RCURVETO, x3, y3);

	return !retval;
}

/**
 * hg_path_arc:
 * @path:
 * @x:
 * @y:
 * @r:
 * @angle1:
 * @angle2:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_arc(hg_path_t *path,
	    hg_real_t  x,
	    hg_real_t  y,
	    hg_real_t  r,
	    hg_real_t  angle1,
	    hg_real_t  angle2)
{
	hg_real_t r_sin_a1, r_cos_a1;
	hg_real_t r_sin_a2, r_cos_a2;
	hg_real_t h;
	hg_bool_t retval = hg_path_get_current_point(path, NULL, NULL);

	if (angle1 < 0.0)
		angle1 = 2 * G_PI - angle1;
	if (angle2 < 0.0)
		angle2 = 2 * G_PI - angle2;
	while (angle2 < angle1)
		angle2 += 2 * G_PI;

	r_sin_a1 = r * sin(angle1);
	r_cos_a1 = r * cos(angle1);
	r_sin_a2 = r * sin(angle2);
	r_cos_a2 = r * cos(angle2);
	h = 4.0 / 3.0 * tan((angle2 - angle1) / 4.0);

	if (retval) {
		retval = !_hg_path_add(path, HG_PATH_LINETO, r_cos_a1 + x, r_sin_a1 + y);
	} else {
		retval = !_hg_path_add(path, HG_PATH_MOVETO, r_cos_a1 + x, r_sin_a1 + y);
	}

	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a1 + x - h * r_sin_a1, r_sin_a1 + y + h * r_cos_a1);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a2 + x + h * r_sin_a2, r_sin_a2 + y - h * r_cos_a2);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a2 + x, r_sin_a2 + y);

	return !retval;
}

/**
 * hg_path_arcn:
 * @path:
 * @x:
 * @y:
 * @r:
 * @angle1:
 * @angle2:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_arcn(hg_path_t *path,
	     hg_real_t  x,
	     hg_real_t  y,
	     hg_real_t  r,
	     hg_real_t  angle1,
	     hg_real_t  angle2)
{
	hg_real_t r_sin_a1, r_cos_a1;
	hg_real_t r_sin_a2, r_cos_a2;
	hg_real_t h;
	hg_bool_t retval = hg_path_get_current_point(path, NULL, NULL);

	if (angle1 < 0.0)
		angle1 = 2 * G_PI - angle1;
	if (angle2 < 0.0)
		angle2 = 2 * G_PI - angle2;
	while (angle2 > angle1)
		angle2 -= 2 * G_PI;

	r_sin_a1 = r * sin(angle1);
	r_cos_a1 = r * cos(angle1);
	r_sin_a2 = r * sin(angle2);
	r_cos_a2 = r * cos(angle2);
	h = 4.0 / 3.0 * tan((angle1 - angle2) / 4.0);

	if (retval) {
		retval = !_hg_path_add(path, HG_PATH_LINETO, r_cos_a1 + x, r_sin_a1 + y);
	} else {
		retval = !_hg_path_add(path, HG_PATH_MOVETO, r_cos_a1 + x, r_sin_a1 + y);
	}

	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a1 + x - h * r_sin_a1, r_sin_a1 + y + h * r_cos_a1);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a2 + x + h * r_sin_a2, r_sin_a2 + y - h * r_cos_a2);
	retval |= !_hg_path_add(path, HG_PATH_CURVETO, r_cos_a2 + x, r_sin_a2 + y);

	return !retval;
}

/**
 * hg_path_arcto:
 * @path:
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 * @r:
 * @tp:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_arcto(hg_path_t *path,
	      hg_real_t  x1,
	      hg_real_t  y1,
	      hg_real_t  x2,
	      hg_real_t  y2,
	      hg_real_t  r,
	      hg_real_t  tp[4])
{
	hg_real_t x0 = 0.0, y0 = 0.0;
	hg_bool_t retval = !hg_path_get_current_point(path, &x0, &y0);

	if (!retval) {
		/* calculate the tangent points */
		/* a = <x0,y0>, b = <x1,y1>, c = <x2,y2> */
		hg_real_t d_ab_x = x0 - x1, d_ab_y = y0 - y1;
		hg_real_t d_bc_x = x2 - x1, d_bc_y = y2 - y1;
		hg_real_t _ab_ = sqrt(d_ab_x * d_ab_x + d_ab_y * d_ab_y);
		hg_real_t _bc_ = sqrt(d_bc_x * d_bc_x + d_bc_y * d_bc_y);
		hg_real_t a_o_c = d_ab_x * d_bc_x + d_ab_y * d_bc_y;
		hg_real_t a_x_c = d_ab_y * d_bc_x - d_bc_y * d_ab_x;
		hg_real_t common = r * (_ab_ * _bc_ + a_o_c);
		hg_real_t _t1b_ = common * _bc_ / (_ab_ * a_x_c);
		hg_real_t _t2b_ = common * _ab_ / (_bc_ * a_x_c);
		hg_real_t ca = fabs(_t1b_ / _bc_);
		hg_real_t cc = fabs(_t2b_ / _ab_);
		hg_real_t t1x = d_ab_x * ca + x1;
		hg_real_t t1y = d_ab_y * ca + y1;
		hg_real_t t2x = d_bc_x * cc + x1;
		hg_real_t t2y = d_bc_y * cc + y1;

		if (tp) {
			tp[0] = t1x;
			tp[1] = t1y;
			tp[2] = t2x;
			tp[3] = t2y;
		}

		if (HG_REAL_IS_ZERO (t1y * x0 - y0 * t1x)) {
			/* points to the same place */
		} else {
			retval = !_hg_path_add(path, HG_PATH_LINETO, t1x, t1y);
		}
		/* XXX: two the control points */
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, t2x, t2y);
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, t2x, t2y);
		/* XXX: end */
		retval |= !_hg_path_add(path, HG_PATH_CURVETO, t2x, t2y);
	} else {
		hg_error_return (HG_STATUS_FAILED, HG_e_nocurrentpoint);
	}

	return !retval;
}

/**
 * hg_path_operate:
 * @path:
 * @vtable:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_operate(hg_path_t                *path,
		hg_path_operate_vtable_t *vtable,
		hg_pointer_t              user_data)
{
	hg_path_node_t *node;
	hg_bool_t retval = TRUE;
	hg_usize_t i;

	hg_return_val_if_fail (path != NULL, FALSE, HG_e_typecheck);
	hg_return_val_if_fail (vtable != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vtable->new_path != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vtable->moveto != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vtable->lineto != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vtable->curveto != NULL, FALSE, HG_e_VMerror);
	hg_return_val_if_fail (vtable->close_path != NULL, FALSE, HG_e_VMerror);

	hg_return_val_if_lock_fail (node,
				    path->o.mem,
				    path->qnode,
				    FALSE);

	vtable->new_path(user_data);
	for (i = 0; i < path->length; i++) {
		switch (node[i].type) {
		    case HG_PATH_SETBBOX:
			    hg_fatal("XXX: not yet implemented.");
			    break;
		    case HG_PATH_RMOVETO:
			    if (vtable->rmoveto) {
				    vtable->rmoveto(user_data,
						    node[i].dx, node[i].dy);
				    break;
			    }
		    case HG_PATH_MOVETO:
			    vtable->moveto(user_data,
					   node[i].cx, node[i].cy);
			    break;
		    case HG_PATH_RLINETO:
			    if (vtable->rlineto) {
				    vtable->rlineto(user_data,
						    node[i].dx, node[i].dy);
				    break;
			    }
		    case HG_PATH_LINETO:
			    vtable->lineto(user_data,
					   node[i].cx, node[i].cy);
			    break;
		    case HG_PATH_RCURVETO:
			    if (vtable->rcurveto) {
				    vtable->rcurveto(user_data,
						     node[i].dx, node[i].dy,
						     node[i + 1].dx, node[i + 1].dy,
						     node[i + 2].dx, node[i + 2].dy);
				    i += 2;
				    break;
			    }
		    case HG_PATH_CURVETO:
			    vtable->curveto(user_data,
					    node[i].cx, node[i].cy,
					    node[i + 1].cx, node[i + 1].cy,
					    node[i + 2].cx, node[i + 2].cy);
			    i += 2;
			    break;
		    case HG_PATH_CLOSEPATH:
			    vtable->close_path(user_data);
			    break;
		    case HG_PATH_UCACHE:
			    hg_fatal("XXX: not yet implemented.");
			    break;
		    default:
			    hg_warning("%s: Unknown path type: %d",
				       __PRETTY_FUNCTION__, node[i].type);
			    retval = FALSE;
			    goto finalize;
		}
	}
  finalize:
	hg_mem_unlock_object(path->o.mem, path->qnode);

	return retval;
}

/**
 * hg_path_get_bbox:
 * @path:
 * @ignore_last_moveto:
 * @ret:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_bool_t
hg_path_get_bbox(hg_path_t      *path,
		 hg_bool_t       ignore_last_moveto,
		 hg_path_bbox_t *ret)
{
	hg_path_bbox_private_t priv;
	hg_path_operate_vtable_t vtable = {
		_hg_path_bbox_nop, /* new_path */
		_hg_path_bbox_nop, /* close_path */
		_hg_path_bbox_moveto, /* moveto */
		NULL, /* rmoveto */
		_hg_path_bbox_lineto, /* lineto */
		NULL, /* rlineto */
		_hg_path_bbox_curveto, /* curveto */
		NULL /* rcurveto */
	};
	hg_bool_t retval;

	hg_return_val_if_fail (path != NULL, FALSE, HG_e_typecheck);

	if (path->length == 0) {
		hg_debug(HG_MSGCAT_PATH, "no current point");

		hg_error_return (HG_STATUS_FAILED, HG_e_nocurrentpoint);
	}
	memset(&priv, 0, sizeof (hg_path_bbox_private_t));

	priv.initialized = FALSE;
	priv.all_moveto = TRUE;

	if ((retval = hg_path_operate(path, &vtable, &priv))) {
		if (ret) {
			if (ignore_last_moveto)
				memcpy(ret, &priv.bbox2, sizeof (hg_path_bbox_t));
			else
				memcpy(ret, &priv.bbox1, sizeof (hg_path_bbox_t));
		}
	}

	return retval;
}
