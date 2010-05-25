/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgobject.c
 * Copyright (C) 2005-2010 Akira TAGOH
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

#include <string.h>
#include "hgerror.h"
#include "hgint.h"
#include "hgmem.h"
#include "hgobject.h"


static hg_quark_t _hg_object_new(hg_mem_t          *mem,
				 hg_object_type_t   type,
				 gsize              size,
				 hg_object_t      **ret);


static hg_object_vtable_t *vtables[HG_TYPE_END];
static gboolean is_initialized = FALSE;

/*< private >*/
static hg_quark_t
_hg_object_new(hg_mem_t          *mem,
	       hg_object_type_t   type,
	       gsize              size,
	       hg_object_t      **ret)
{
	hg_quark_t retval;
	hg_object_t *object;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	retval = hg_mem_alloc(mem, sizeof (hg_object_t) > size ? sizeof (hg_object_t) : size, (gpointer *)&object);
	if (retval != Qnil) {
		memset(object, 0, sizeof (hg_object_t));
		object->t.x.type = HG_OBJECT_MASK_TYPE (type);

		if (ret)
			*ret = object;
	}

	return retval;
}

/*< public >*/
/**
 * hg_object_init:
 *
 * FIXME
 */
void
hg_object_init(void)
{
	if (!is_initialized) {
		hg_object_vtable_t *v;

		is_initialized = TRUE;

		v = hg_object_int_get_vtable();
		vtables[HG_TYPE_INT] = v;
	}
}

/**
 * hg_object_fini:
 *
 * FIXME
 */
void
hg_object_fini(void)
{
	is_initialized = FALSE;
}

/**
 * hg_object_new:
 * @mem:
 * @type:
 * @preallocated_size:
 *
 * FIXME
 *
 * Returns:
 */
hg_object_t *
hg_object_new(hg_mem_t         *mem,
	      hg_object_type_t  type,
	      gsize             preallocated_size,
	      ...)
{
	hg_object_vtable_t *v;
	hg_object_t *retval = NULL;
	hg_quark_t index;
	gsize size;
	va_list ap;

	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (type < HG_TYPE_END, NULL);
	hg_return_val_if_fail (vtables[type] != NULL, NULL);

	v = vtables[type];
	size = v->get_capsulated_size();
	index = _hg_object_new(mem, type, size + preallocated_size, &retval);

	va_start(ap, preallocated_size);

	v->initialize(mem, retval, ap);

	va_end(ap);

	return retval;
}

/**
 * hg_object_free:
 *
 * FIXME
 */
void
hg_object_free(hg_mem_t   *mem,
	       hg_quark_t  index)
{
	hg_object_t *object;
	hg_object_vtable_t *v;

	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (index != Qnil);

	object = hg_mem_lock_object(mem, index);
	hg_return_if_fail (object->t.x.type < HG_TYPE_END);

	v = vtables[object->t.x.type];
	v->free(mem, object);

	hg_mem_unlock_object(mem, index);
	hg_mem_free(mem, index);
}