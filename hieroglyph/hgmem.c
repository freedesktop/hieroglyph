/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgmem.c
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
#endif /* HAVE_CONFIG_H */

#include <math.h>
#include "hgallocator.h"
#include "hgerror.h"
#include "hgquark.h"
#include "hgmem.h"
#include "hgmem-private.h"

static gint __hg_mem_id = 0;

/*< private >*/

/*< public >*/

/* memory management */
/**
 * hg_mem_new:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_mem_new(gsize size)
{
	return hg_mem_new_with_allocator(hg_allocator_get_vtable(), size);
}

/**
 * hg_mem_new_with_allocator:
 * @allocator:
 * @size:
 *
 * FIXME
 *
 * Returns:
 */
hg_mem_t *
hg_mem_new_with_allocator(hg_mem_vtable_t *allocator,
			  gsize            size)
{
	hg_mem_t *retval;

	hg_return_val_if_fail (allocator != NULL, NULL);
	hg_return_val_if_fail (allocator->initialize != NULL, NULL);
	hg_return_val_if_fail (size > 0, NULL);
	hg_return_val_if_fail (__hg_mem_id >= 0, NULL);

	retval = g_new0(hg_mem_t, 1);
	retval->allocator = allocator;
	retval->id = __hg_mem_id++;
	if (_hg_quark_type_bit_validate_bits(retval->id,
					     HG_QUARK_TYPE_BIT_MEM_ID,
					     HG_QUARK_TYPE_BIT_MEM_ID_END) != retval->id) {
		g_warning("too many memory spooler being created.");
		g_free(retval);

		return NULL;
	}
	retval->data = allocator->initialize();
	if (!retval->data) {
		hg_mem_destroy(retval);
		retval = NULL;
	} else {
		hg_mem_resize_heap(retval, size);
	}

	return retval;
}

/**
 * hg_mem_destroy:
 * @mem:
 *
 * FIXME
 */
void
hg_mem_destroy(gpointer data)
{
	hg_mem_t *mem;

	if (!data)
		return;

	mem = (hg_mem_t *)data;

	hg_return_if_fail (mem->allocator != NULL);
	hg_return_if_fail (mem->allocator->finalize != NULL);

	mem->allocator->finalize(mem->data);

	g_free(mem);
}

/**
 * hg_mem_resize_heap:
 * @mem:
 * @size:
 *
 * FIXME
 */
gboolean
hg_mem_resize_heap(hg_mem_t  *mem,
		   gsize      size)
{
	hg_return_val_if_fail (mem != NULL, FALSE);
	hg_return_val_if_fail (mem->allocator != NULL, FALSE);
	hg_return_val_if_fail (mem->allocator->resize_heap != NULL, FALSE);
	hg_return_val_if_fail (mem->data != NULL, FALSE);
	hg_return_val_if_fail (size > 0, FALSE);

	return mem->allocator->resize_heap(mem->data, size);
}

/**
 * hg_mem_alloc:
 * @mem:
 * @size:
 * @ret:
 *
 * FIXME:
 *
 * Returns:
 */
hg_quark_t
hg_mem_alloc(hg_mem_t *mem,
	     gsize     size,
	     gpointer *ret)
{
	hg_quark_t retval;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (mem->allocator != NULL, Qnil);
	hg_return_val_if_fail (mem->allocator->alloc != NULL, Qnil);
	hg_return_val_if_fail (mem->data != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	retval = mem->allocator->alloc(mem->data, size, ret);
	if (retval != Qnil) {
		hg_quark_set_mem_id(&retval, mem->id);
	}

	return retval;
}

/**
 * hg_mem_realloc:
 * @mem:
 * @qdata:
 * @size:
 * @ret:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_mem_realloc(hg_mem_t   *mem,
	       hg_quark_t  qdata,
	       gsize       size,
	       gpointer   *ret)
{
	hg_quark_t retval;

	if (qdata == Qnil)
		return hg_mem_alloc(mem, size, ret);

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (mem->allocator != NULL, Qnil);
	hg_return_val_if_fail (mem->allocator->realloc != NULL, Qnil);
	hg_return_val_if_fail (mem->data != NULL, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);
	hg_return_val_if_fail (hg_quark_has_same_mem_id(qdata, mem->id), Qnil);

	retval = mem->allocator->realloc(mem->data,
					 hg_quark_get_value(qdata),
					 size,
					 ret);
	if (retval != Qnil) {
		hg_quark_set_mem_id(&retval,
				    mem->id);
	}

	return retval;
}

/**
 * hg_mem_free:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_free(hg_mem_t   *mem,
	    hg_quark_t  qdata)
{
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (mem->allocator != NULL);
	hg_return_if_fail (mem->allocator->free != NULL);
	hg_return_if_fail (mem->data != NULL);

	if (qdata == Qnil)
		return;

	hg_return_if_fail (hg_quark_has_same_mem_id(qdata, mem->id));

	mem->allocator->free(mem->data,
			     hg_quark_get_value (qdata));
}

/**
 * hg_mem_lock_object:
 * @mem:
 * @qdata:
 *
 * FIXME
 *
 * Return:
 */
gpointer
hg_mem_lock_object(hg_mem_t   *mem,
		   hg_quark_t  qdata)
{
	hg_return_val_if_fail (mem != NULL, NULL);
	hg_return_val_if_fail (mem->allocator != NULL, NULL);
	hg_return_val_if_fail (mem->allocator->lock_object != NULL, NULL);
	hg_return_val_if_fail (mem->data != NULL, NULL);

	if (qdata == Qnil)
		return NULL;

	hg_return_val_if_fail (hg_quark_has_same_mem_id(qdata, mem->id), NULL);

	return mem->allocator->lock_object(mem->data,
					   hg_quark_get_value(qdata));
}

/**
 * hg_mem_unlock_object:
 * @mem:
 * @qdata:
 *
 * FIXME
 */
void
hg_mem_unlock_object(hg_mem_t   *mem,
		     hg_quark_t  qdata)
{
	hg_return_if_fail (mem != NULL);
	hg_return_if_fail (mem->allocator != NULL);
	hg_return_if_fail (mem->allocator->unlock_object != NULL);
	hg_return_if_fail (mem->data != NULL);

	if (qdata == Qnil)
		return;

	hg_return_if_fail (hg_quark_has_same_mem_id(qdata, mem->id));

	return mem->allocator->unlock_object(mem->data,
					     hg_quark_get_value (qdata));
}

/**
 * hg_mem_get_id:
 * @mem:
 *
 * FIXME
 *
 * Returns:
 */
gint
hg_mem_get_id(hg_mem_t *mem)
{
	hg_return_val_if_fail (mem != NULL, -1);

	return mem->id;
}
