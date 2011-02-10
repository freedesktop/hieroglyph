/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator.c
 * Copyright (C) 2006-2011 Akira TAGOH
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

#include <stdlib.h>
#include <string.h>
/* GLib is still needed for the mutex lock */
#include <glib.h>
#include "hgerror.h"
#include "hgmessages.h"
#include "hgquark.h"
#include "hgallocator.h"
#include "hgallocator-private.h"

#include "hgallocator.proto.h"

/* memory block size per 1 bit in the bitmap */
#define BLOCK_SIZE		32

/* memory block size per 1 bit in the bitmap */
#define BLOCK_SIZE		32

static hg_mem_vtable_t __hg_allocator_vtable = {
	.initialize        = _hg_allocator_initialize,
	.finalize          = _hg_allocator_finalize,
	.expand_heap       = _hg_allocator_expand_heap,
	.get_max_heap_size = _hg_allocator_get_max_heap_size,
	.alloc             = _hg_allocator_alloc,
	.realloc           = _hg_allocator_realloc,
	.free              = _hg_allocator_free,
	.lock_object       = _hg_allocator_lock_object,
	.unlock_object     = _hg_allocator_unlock_object,
	.gc_init           = _hg_allocator_gc_init,
	.gc_mark           = _hg_allocator_gc_mark,
	.gc_finish         = _hg_allocator_gc_finish,
	.save_snapshot     = _hg_allocator_save_snapshot,
	.restore_snapshot  = _hg_allocator_restore_snapshot,
	.destroy_snapshot  = _hg_allocator_destroy_snapshot,
};
G_LOCK_DEFINE_STATIC (bitmap);
G_LOCK_DEFINE_STATIC (allocator);

/*< private >*/

HG_INLINE_FUNC hg_usize_t
hg_allocator_get_page_size(void)
{
	static hg_usize_t retval = ((1LL << (HG_ALLOC_TYPE_BIT_INDEX_END - HG_ALLOC_TYPE_BIT_INDEX + 1)) - 1);

	return retval;
}

HG_INLINE_FUNC hg_usize_t
hg_allocator_get_max_page(void)
{
	static hg_usize_t retval = ((1LL << (HG_ALLOC_TYPE_BIT_PAGE_END - HG_ALLOC_TYPE_BIT_PAGE + 1)) - 1);

	return retval;
}

HG_INLINE_FUNC hg_pointer_t
hg_allocator_get_allocated_object(hg_allocator_block_t *block)
{
	return (hg_pointer_t)((hg_char_t *)(block) + HG_ALIGNED_TO_POINTER (sizeof (hg_allocator_block_t)));
}

/** bitmap operation **/
HG_INLINE_FUNC hg_allocator_bitmap_t *
_hg_allocator_bitmap_new(hg_usize_t size)
{
	hg_allocator_bitmap_t *retval;
	hg_usize_t aligned_size, bitmap_size;
	hg_int_t page, max_page = hg_allocator_get_max_page();

	hg_return_val_if_fail (size > 0, NULL);
	hg_return_val_if_fail (size <= hg_allocator_get_page_size() * BLOCK_SIZE, NULL);

	aligned_size = HG_ALIGNED_TO (size, BLOCK_SIZE);
	bitmap_size = HG_ALIGNED_TO (aligned_size / BLOCK_SIZE, sizeof (guint32));
	retval = g_new0(hg_allocator_bitmap_t, 1);
	if (retval) {
		retval->bitmaps = g_new0(hg_uint_t *, max_page);
		retval->size = g_new0(gsize, max_page);
		retval->last_index = g_new0(hg_quark_t, max_page);
		page = _hg_allocator_bitmap_get_free_page(retval);
		if (!retval->bitmaps ||
		    !retval->size ||
		    !retval->last_index ||
		    page < 0) {
			_hg_allocator_bitmap_destroy(retval);
			return NULL;
		}
		retval->bitmaps[page] = g_new0(hg_uint_t, bitmap_size / sizeof (guint32));
		retval->size[page] = bitmap_size;
	}

	return retval;
}

HG_INLINE_FUNC hg_int_t
_hg_allocator_bitmap_get_free_page(hg_allocator_bitmap_t *bitmap)
{
	hg_int_t i, max_page = hg_allocator_get_max_page();

	for (i = 0; i < max_page; i++) {
		if (!bitmap->bitmaps[i])
			return i;
	}

	return -1;
}

HG_INLINE_FUNC void
_hg_allocator_bitmap_destroy(hg_pointer_t data)
{
	hg_allocator_bitmap_t *bitmap = data;
	hg_usize_t i, max_page = hg_allocator_get_max_page();

	if (!data)
		return;
	for (i = 0; i < max_page; i++) {
		g_free(bitmap->bitmaps[i]);
	}
	g_free(bitmap->bitmaps);
	g_free(bitmap->size);
	g_free(bitmap->last_index);
	g_free(bitmap);
}

HG_INLINE_FUNC hg_int_t
_hg_allocator_bitmap_add_page(hg_allocator_bitmap_t *bitmap,
			      hg_usize_t             size)
{
	hg_int_t page;

	page = _hg_allocator_bitmap_get_free_page(bitmap);
	if (page >= 0) {
		if (!_hg_allocator_bitmap_add_page_to(bitmap, page, size))
			return -1;
	}

	return page;
}

HG_INLINE_FUNC hg_bool_t
_hg_allocator_bitmap_add_page_to(hg_allocator_bitmap_t *bitmap,
				 hg_int_t               page,
				 hg_usize_t             size)
{
	hg_usize_t aligned_size, bitmap_size;

	hg_return_val_if_fail (size > 0 && size <= hg_allocator_get_page_size() * BLOCK_SIZE, FALSE);
	hg_return_val_if_fail (page >= 0 && page < hg_allocator_get_max_page(), FALSE);
	hg_return_val_if_fail (bitmap->bitmaps[page] == NULL, FALSE);

	G_LOCK (bitmap);

	aligned_size = HG_ALIGNED_TO (size, BLOCK_SIZE);
	bitmap_size = HG_ALIGNED_TO (aligned_size / BLOCK_SIZE, sizeof (hg_uint_t));
	if (page >= 0) {
		bitmap->bitmaps[page] = g_new0(hg_uint_t, bitmap_size / sizeof (hg_uint_t));
		bitmap->size[page] = bitmap_size;
	}

	G_UNLOCK (bitmap);

	return TRUE;
}

HG_INLINE_FUNC hg_quark_t
_hg_allocator_bitmap_alloc(hg_allocator_bitmap_t *bitmap,
			   hg_usize_t             size)
{
	hg_usize_t aligned_size;
	hg_int_t page;
	hg_uint_t i, j, idx = 0;
	hg_bool_t retry = FALSE;

	aligned_size = HG_ALIGNED_TO (size, BLOCK_SIZE) / BLOCK_SIZE;
	page = bitmap->last_page;

	hg_debug(HG_MSGCAT_ALLOC, "%ld blocks required", aligned_size);
#if defined(HG_DEBUG)
	_hg_allocator_bitmap_dump(bitmap, page);
#endif
  find_free_page:
	idx = bitmap->last_index[page];
  find_free_bitmap:
	for (i = idx, j = i + 1; i < bitmap->size[page]; i++, j += 2) {
		if (_hg_allocator_bitmap_range_mark(bitmap,
						    page,
						    &j,
						    aligned_size)) {
			bitmap->last_index[page] = ((i + 1) >= bitmap->size[page] ? 0 : i);
			bitmap->last_page = page;

			hg_debug(HG_MSGCAT_ALLOC, "allocated at [page: %d, index: %d]", page, i + 1);

			return _hg_allocator_quark_build(page, i + 1);
		} else {
			i = j;
		}
	}
	if (idx != 0) {
		if (!retry) {
			/* retry to find out a free space at the beginning again */
			retry = TRUE;
			idx = 0;
			goto find_free_bitmap;
		}
	}
	page++;
	if (page >= hg_allocator_get_max_page())
		page = 0;
	if (page != bitmap->last_page) {
		retry = FALSE;
		goto find_free_page;
	}

	return Qnil;
}

HG_INLINE_FUNC hg_quark_t
_hg_allocator_bitmap_realloc(hg_allocator_bitmap_t *bitmap,
			     hg_quark_t             index_,
			     hg_usize_t             old_size,
			     hg_usize_t             size)
{
	hg_quark_t i;
	hg_size_t aligned_size, old_aligned_size, required_size;
	hg_int_t page;
	hg_uint_t idx;

	hg_return_val_if_fail (index_ != Qnil, Qnil);
	hg_return_val_if_fail (size > 0, Qnil);

	aligned_size = HG_ALIGNED_TO (size, BLOCK_SIZE) / BLOCK_SIZE;
	old_aligned_size = HG_ALIGNED_TO (old_size, BLOCK_SIZE) / BLOCK_SIZE;
	page = _hg_allocator_quark_get_page(index_);
	idx = _hg_allocator_quark_get_index(index_);

	hg_debug(HG_MSGCAT_ALLOC, "%ld blocks to be reallocated from %ld blocks at index %ld [page: %d, idx: %u]", aligned_size, old_aligned_size, index_, page, idx);
#if defined(HG_DEBUG)
	_hg_allocator_bitmap_dump(bitmap, page);
#endif
	required_size = aligned_size - old_aligned_size;
	if (required_size < 0) {
		G_LOCK (bitmap);

		for (i = idx + aligned_size; i < (idx + old_aligned_size); i++)
			_hg_allocator_bitmap_clear(bitmap, page, i);

		G_UNLOCK (bitmap);

		return index_;
	}
	if (idx + old_aligned_size < bitmap->size[page] &&
	    !_hg_allocator_bitmap_is_marked(bitmap, page, idx + old_aligned_size)) {
		required_size--;
		for (i = idx + old_aligned_size + 1; required_size > 0 && i < bitmap->size[page]; i++) {
			if (!_hg_allocator_bitmap_is_marked(bitmap, page, i)) {
				required_size--;
			} else {
				break;
			}
		}
	}
	if (required_size == 0) {
		G_LOCK (bitmap);

		for (i = idx + old_aligned_size; i < (idx + aligned_size); i++)
			_hg_allocator_bitmap_mark(bitmap, page, i);

		G_UNLOCK (bitmap);

		return index_;
	}
	hg_debug(HG_MSGCAT_ALLOC, "Unable to reallocate. falling back to simply allocate the new memory.");

	return _hg_allocator_bitmap_alloc(bitmap, size);
}

HG_INLINE_FUNC void
_hg_allocator_bitmap_free(hg_allocator_bitmap_t *bitmap,
			  hg_quark_t             index_,
			  hg_usize_t             size)
{
	hg_quark_t i;
	hg_usize_t aligned_size;
	hg_int_t page;
	hg_uint_t idx;

	hg_return_if_fail (index_ > 0);
	hg_return_if_fail (size > 0);

	aligned_size = HG_ALIGNED_TO (size, BLOCK_SIZE) / BLOCK_SIZE;
	page = _hg_allocator_quark_get_page(index_);
	idx = _hg_allocator_quark_get_index(index_);

	hg_return_if_fail ((idx + aligned_size - 1) <= bitmap->size[page]);

	G_LOCK (bitmap);

	for (i = idx; i < (idx + aligned_size); i++)
		_hg_allocator_bitmap_clear(bitmap, page, i);
	bitmap->last_index[page] = i - 1;

	G_UNLOCK (bitmap);

	hg_debug(HG_MSGCAT_ALLOC, "Freed %ld blocks at index %ld [page: %d, idx: %u]", aligned_size, index_, page, idx);
#if defined(HG_DEBUG)
	_hg_allocator_bitmap_dump(bitmap, page);
#endif
}

HG_INLINE_FUNC void
_hg_allocator_bitmap_mark(hg_allocator_bitmap_t *bitmap,
			  hg_int_t               page,
			  hg_uint_t              index_)
{
	hg_return_if_fail (page >= 0 && page < hg_allocator_get_max_page());
	hg_return_if_fail (index_ > 0 && index_ <= bitmap->size[page]);
	hg_return_if_fail (!_hg_allocator_bitmap_is_marked(bitmap, page, index_));

	bitmap->bitmaps[page][(index_ - 1) / sizeof (hg_uint_t)] |= 1 << ((index_ - 1) % sizeof (hg_uint_t));
}

HG_INLINE_FUNC void
_hg_allocator_bitmap_clear(hg_allocator_bitmap_t *bitmap,
			   hg_int_t               page,
			   hg_uint_t              index_)
{
	hg_return_if_fail (page >= 0 && page < hg_allocator_get_max_page());
	hg_return_if_fail (index_ > 0 && index_ <= bitmap->size[page]);

	bitmap->bitmaps[page][(index_ - 1) / sizeof (hg_uint_t)] &= ~(1 << ((index_ - 1) % sizeof (hg_uint_t)));
}

HG_INLINE_FUNC hg_bool_t
_hg_allocator_bitmap_is_marked(hg_allocator_bitmap_t *bitmap,
			       hg_int_t               page,
			       hg_uint_t              index_)
{
	hg_return_val_if_fail (page >= 0 && page < hg_allocator_get_max_page(), FALSE);
	hg_return_val_if_fail (index_ > 0 && index_ <= bitmap->size[page], FALSE);

	return bitmap->bitmaps[page][(index_ - 1) / sizeof (hg_uint_t)] & 1 << ((index_ - 1) % sizeof (hg_uint_t));
}

HG_INLINE_FUNC hg_bool_t
_hg_allocator_bitmap_range_mark(hg_allocator_bitmap_t *bitmap,
				hg_int_t               page,
				hg_uint_t             *index_,
				hg_usize_t             size)
{
	hg_usize_t required_size = size, j;

	if (!_hg_allocator_bitmap_is_marked(bitmap, page, *index_)) {
		required_size--;
		for (j = *index_; required_size > 0 && j < bitmap->size[page]; j++) {
			if (!_hg_allocator_bitmap_is_marked(bitmap, page, j + 1)) {
				required_size--;
			} else {
				break;
			}
		}
		if (required_size == 0) {
			G_LOCK (bitmap);

			for (j = *index_; j < (*index_ + size); j++)
				_hg_allocator_bitmap_mark(bitmap, page, j);

			G_UNLOCK (bitmap);

			return TRUE;
		} else {
			*index_ = j;
		}
	} else {
		(*index_)--;
	}

	return FALSE;
}

HG_INLINE_FUNC hg_allocator_bitmap_t *
_hg_allocator_bitmap_copy(hg_allocator_bitmap_t *bitmap)
{
	hg_allocator_bitmap_t *retval = _hg_allocator_bitmap_new(bitmap->size[0] * BLOCK_SIZE);
	hg_usize_t i, max_page = hg_allocator_get_max_page();

	G_LOCK (bitmap);

	for (i = 0; i < max_page; i++) {
		if (bitmap->bitmaps[i]) {
			if (!retval->bitmaps[i]) {
				retval->bitmaps[i] = g_new0(guint32, bitmap->size[i] / sizeof (guint32));
				bitmap->size[i] = bitmap->size[i];
			}
			memcpy(retval->bitmaps[i], bitmap->bitmaps[i], bitmap->size[i]);
		}
		retval->size[i] = bitmap->size[i];
		retval->last_index[i] = bitmap->last_index[i];
	}

	G_UNLOCK (bitmap);

	return retval;
}

HG_INLINE_FUNC void
_hg_allocator_bitmap_dump(hg_allocator_bitmap_t *bitmap,
			  hg_int_t               page)
{
	hg_usize_t i;

	if (hg_message_is_enabled(HG_MSGCAT_BITMAP)) {
		hg_debug(HG_MSGCAT_BITMAP, "bitmap[%d]: %ld blocks allocated", page, bitmap->size[page]);
		hg_debug(HG_MSGCAT_BITMAP, "        :         1         2         3         4         5         6         7");
		hg_debug(HG_MSGCAT_BITMAP, "        :12345678901234567890123456789012345678901234567890123456789012345678901234567890");
		for (i = 0; i < bitmap->size[page]; i++) {
			if (i % 70 == 0) {
				if (i != 0) {
					hg_debug0(HG_MSGCAT_BITMAP,
						  HG_MSG_FLAG_NO_PREFIX,
						  "");
				}
				hg_debug0(HG_MSGCAT_BITMAP,
					  HG_MSG_FLAG_NO_LINEFEED,
					  "%08lx:", i);
			}
			hg_debug0(HG_MSGCAT_BITMAP,
				  HG_MSG_FLAG_NO_LINEFEED|HG_MSG_FLAG_NO_PREFIX,
				  "%d", _hg_allocator_bitmap_is_marked(bitmap, page, i + 1) ? 1 : 0);
		}
		hg_debug0(HG_MSGCAT_BITMAP, HG_MSG_FLAG_NO_PREFIX, "");
	}
}

/** allocator **/
static hg_pointer_t
_hg_allocator_initialize(void)
{
	hg_allocator_private_t *retval;

	retval = g_new0(hg_allocator_private_t, 1);

	return retval;
}

static void
_hg_allocator_finalize(hg_allocator_data_t *data)
{
	hg_allocator_private_t *priv;
	hg_usize_t i, max_page = hg_allocator_get_max_page();

	if (!data)
		return;

	priv = (hg_allocator_private_t *)data;
	if (priv->heaps) {
		for (i = 0; i < max_page; i++) {
			g_free(priv->heaps[i]);
		}
		g_free(priv->heaps);
	}
	_hg_allocator_bitmap_destroy(priv->bitmap);

	g_free(priv);
}

static hg_bool_t
_hg_allocator_expand_heap(hg_allocator_data_t *data,
			  hg_usize_t           size)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;
	hg_int_t page = 0;

	if (!priv->heaps) {
		priv->heaps = g_new0(hg_pointer_t, hg_allocator_get_max_page());
		if (!priv->heaps)
			return FALSE;
	}
	if (priv->bitmap) {
		if ((page = _hg_allocator_bitmap_add_page(priv->bitmap, size)) < 0)
			return FALSE;
	} else {
		priv->bitmap = _hg_allocator_bitmap_new(size);
		if (!priv->bitmap)
			return FALSE;
	}
	priv->heaps[page] = g_malloc(priv->bitmap->size[page] * BLOCK_SIZE);
	if (!priv->heaps[page])
		return FALSE;
	data->total_size = priv->bitmap->size[page] * BLOCK_SIZE;

	return TRUE;
}

static hg_usize_t
_hg_allocator_get_max_heap_size(hg_allocator_data_t *data)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;
	hg_usize_t retval = 0, i, max_page = hg_allocator_get_max_page();

	for (i = 0; i < max_page; i++) {
		retval = MAX (retval, priv->bitmap->size[i]);
	}

	return retval * BLOCK_SIZE;
}

static hg_quark_t
_hg_allocator_alloc(hg_allocator_data_t *data,
		    hg_usize_t           size,
		    hg_uint_t            flags,
		    hg_pointer_t        *ret)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *block;
	hg_usize_t obj_size;
	hg_quark_t index_, retval = Qnil;

	priv = (hg_allocator_private_t *)data;

	obj_size = HG_ALIGNED_TO_POINTER (sizeof (hg_allocator_block_t) + size);
	index_ = _hg_allocator_bitmap_alloc(priv->bitmap, obj_size);
	if (index_ != Qnil) {
		/* Update the used size */
		data->used_size += obj_size;

		block = _hg_allocator_get_internal_block(priv, index_, TRUE);
		block->size = obj_size;
		block->age = priv->snapshot_age;
		block->is_restorable = flags & HG_MEM_RESTORABLE ? TRUE : FALSE;
		retval = index_;
		/* NOTE: No unlock yet here.
		 *       any objects are supposed to be unlocked
		 *       when it's entered into the stack where PostScript
		 *       VM manages. otherwise it will be swept by GC.
		 */
		if (ret)
			*ret = hg_allocator_get_allocated_object(block);
		else
			_hg_allocator_real_unlock_object(block);
	}

	return retval;
}

static hg_quark_t
_hg_allocator_realloc(hg_allocator_data_t *data,
		      hg_quark_t           qdata,
		      hg_usize_t           size,
		      hg_pointer_t        *ret)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *block, *new_block;
	hg_usize_t obj_size;
	hg_quark_t index_ = Qnil, retval = Qnil;

	G_LOCK (allocator);

	priv = (hg_allocator_private_t *)data;

	obj_size = HG_ALIGNED_TO_POINTER (sizeof (hg_allocator_block_t) + size);
	block = _hg_allocator_real_lock_object(data, qdata);
	if (HG_LIKELY (block)) {
		volatile gint make_sure_if_no_referrer;

		make_sure_if_no_referrer = g_atomic_int_get(&block->lock_count);
		hg_return_val_after_eval_if_fail (make_sure_if_no_referrer == 1, Qnil, _hg_allocator_real_unlock_object(block));

		index_ = _hg_allocator_bitmap_realloc(priv->bitmap, qdata, block->size, obj_size);
		if (index_ != Qnil) {
			/* Update the used size */
			data->used_size += (obj_size - block->size);

			new_block = _hg_allocator_get_internal_block(priv, index_, FALSE);
			if (new_block != block) {
				memcpy(new_block, block, block->size);

				new_block->lock_count = make_sure_if_no_referrer;

				_hg_allocator_bitmap_free(priv->bitmap, qdata, block->size);
			}
			new_block->size = obj_size;
			retval = index_;

			if (ret)
				*ret = hg_allocator_get_allocated_object(new_block);
			else
				_hg_allocator_real_unlock_object(new_block);
		} else {
			_hg_allocator_real_unlock_object(block);
		}
	} else {
#if defined(HG_DEBUG)
		hg_warning("%lx isn't the allocated object.\n", qdata);
#endif
	}

	G_UNLOCK (allocator);

	return retval;
}

static void
_hg_allocator_free(hg_allocator_data_t *data,
		   hg_quark_t           index_)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *block;

	priv = (hg_allocator_private_t *)data;
	block = _hg_allocator_real_lock_object(data, index_);
	if (HG_LIKELY (block)) {
		/* Update the used size */
		data->used_size -= block->size;

		_hg_allocator_bitmap_free(priv->bitmap, index_, block->size);
	} else {
#if defined(HG_DEBUG)
		hg_warning("%lx isn't the allocated object.\n", index_);
#endif
	}
}

HG_INLINE_FUNC hg_pointer_t
_hg_allocator_get_internal_block(hg_allocator_private_t *priv,
				 hg_quark_t              index_,
				 hg_bool_t               initialize)
{
	hg_int_t page;
	hg_uint_t idx;

	page = _hg_allocator_quark_get_page(index_);
	idx = _hg_allocator_quark_get_index(index_);

	return _hg_allocator_get_internal_block_from_page_and_index(priv, page, idx, initialize);
}

HG_INLINE_FUNC hg_pointer_t
_hg_allocator_get_internal_block_from_page_and_index(hg_allocator_private_t *priv,
						     hg_int_t                page,
						     hg_uint_t               idx,
						     hg_bool_t               initialize)
{
	hg_allocator_block_t *retval;

	if (page >= hg_allocator_get_max_page() ||
	    idx > priv->bitmap->size[page]) {
		hg_warning("Invalid quark to access: [page: %d, index: %d]", page, idx);
		return NULL;
	}
	retval = (hg_allocator_block_t *)((hg_quark_t)priv->heaps[page] + ((idx - 1) * BLOCK_SIZE));
	if (initialize) {
		memset(retval, 0, sizeof (hg_allocator_block_t));
		retval->lock_count = 1;
	}

	return retval;
}

HG_INLINE_FUNC hg_pointer_t
_hg_allocator_real_lock_object(hg_allocator_data_t *data,
			       hg_quark_t           index_)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *retval = NULL;
	hg_int_t old_val;
	hg_int_t page;
	hg_uint_t idx;

	page = _hg_allocator_quark_get_page(index_);
	idx = _hg_allocator_quark_get_index(index_);

	hg_debug(HG_MSGCAT_ALLOC, "Locking index %ld [page: %d, idx: %u]", index_, page, idx);
	priv = (hg_allocator_private_t *)data;
	if (_hg_allocator_bitmap_is_marked(priv->bitmap, page, idx)) {
		/* XXX: maybe better validate the block size too? */
		retval = _hg_allocator_get_internal_block_from_page_and_index(priv, page, idx, FALSE);
		old_val = g_atomic_int_exchange_and_add((int *)&retval->lock_count, 1);
	}

	return retval;
}

static hg_pointer_t
_hg_allocator_lock_object(hg_allocator_data_t *data,
			  hg_quark_t           index_)
{
	hg_allocator_block_t *retval = NULL;

	G_LOCK (allocator);

	retval = _hg_allocator_real_lock_object(data, index_);

	G_UNLOCK (allocator);
	if (retval)
		return hg_allocator_get_allocated_object(retval);

	return NULL;
}

HG_INLINE_FUNC void
_hg_allocator_real_unlock_object(hg_allocator_block_t *block)
{
	hg_int_t old_val;

	hg_return_if_fail (block != NULL);
	hg_return_if_fail (block->lock_count > 0);

  retry_atomic_decrement:
	old_val = g_atomic_int_get(&block->lock_count);
	if (!g_atomic_int_compare_and_exchange((int *)&block->lock_count, old_val, old_val - 1))
		goto retry_atomic_decrement;
}

static void
_hg_allocator_unlock_object(hg_allocator_data_t *data,
			    hg_quark_t           index_)
{
	hg_allocator_private_t *priv;
	hg_allocator_block_t *retval = NULL;
	hg_int_t page;
	hg_uint_t idx;

	priv = (hg_allocator_private_t *)data;

	page = _hg_allocator_quark_get_page(index_);
	idx = _hg_allocator_quark_get_index(index_);

	if (_hg_allocator_bitmap_is_marked(priv->bitmap, page, idx)) {
		/* XXX: maybe better validate the block size too? */
		retval = _hg_allocator_get_internal_block_from_page_and_index(priv, page, idx, FALSE);
		_hg_allocator_real_unlock_object(retval);
	}
}

static hg_bool_t
_hg_allocator_gc_init(hg_allocator_data_t *data)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;
	hg_usize_t i, max_page = hg_allocator_get_max_page();

	if (HG_UNLIKELY (priv->slave_bitmap)) {
		hg_warning("GC is already ongoing.");
		return FALSE;
	}
	priv->slave_bitmap = _hg_allocator_bitmap_new(priv->bitmap->size[0] * BLOCK_SIZE);
	for (i = 1; i < max_page; i++) {
		if (priv->bitmap->bitmaps[i]) {
			_hg_allocator_bitmap_add_page_to(priv->slave_bitmap,
							 i,
							 priv->bitmap->size[i] * BLOCK_SIZE);
		}
	}
	priv->slave.total_size = data->total_size;
	priv->slave.used_size = 0;

	return TRUE;
}

static hg_error_t
_hg_allocator_gc_mark(hg_allocator_data_t  *data,
		      hg_quark_t            index_)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;
	hg_allocator_block_t *block;
	hg_int_t page;
	hg_uint_t idx;
	hg_usize_t aligned_size;

	/* this isn't the object in the targeted spool */
	if (!priv->slave_bitmap)
		return HG_ERROR_ (HG_STATUS_SUCCESS, 0);

	block = _hg_allocator_real_lock_object(data, index_);
	if (block) {
		page = _hg_allocator_quark_get_page(index_);
		idx = _hg_allocator_quark_get_index(index_);
		aligned_size = HG_ALIGNED_TO (block->size, BLOCK_SIZE) / BLOCK_SIZE;
		if (_hg_allocator_bitmap_range_mark(priv->slave_bitmap, page, &idx, aligned_size)) {
			hg_debug(HG_MSGCAT_GC, "Marked index %ld, size: %ld", index_, aligned_size);
#if defined(HG_DEBUG)
			_hg_allocator_bitmap_dump(priv->slave_bitmap, page);
#endif
			priv->slave.used_size += block->size;
		} else {
			hg_debug(HG_MSGCAT_GC, "index %ld already marked", index_);
		}
		_hg_allocator_real_unlock_object(block);
	} else {
		hg_critical("%lx is not an allocated object from this spool", index_);

		return HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
	}

	return HG_ERROR_ (HG_STATUS_SUCCESS, 0);
}

static hg_error_t
_hg_allocator_gc_finish(hg_allocator_data_t *data,
			hg_error_t           error)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;

	if (HG_UNLIKELY (!priv->slave_bitmap)) {
		hg_warning("GC isn't yet started.");
		return HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
	}

	G_LOCK (allocator);

	/* XXX: this has to be dropped in the future */
	if (HG_ERROR_IS_SUCCESS (error)) {
		hg_usize_t used_size;
		hg_uint_t i, j, max_page = hg_allocator_get_max_page();

		used_size = data->used_size - priv->slave.used_size;

		hg_debug(HG_MSGCAT_GC, "Marking the locked objects");

		/* give aid to the locked blocks */
		for (i = 0; used_size > 0 && i < max_page; i++) {
			for (j = 0; used_size > 0 && j < priv->bitmap->size[i]; j++) {
				if (_hg_allocator_bitmap_is_marked(priv->bitmap, i, j + 1)) {
					hg_allocator_block_t *block;
					hg_usize_t aligned_size;

					block = _hg_allocator_get_internal_block_from_page_and_index((hg_allocator_private_t *)data,
												     i, j + 1, FALSE);

					if (block == NULL) {
						error = HG_ERROR_ (HG_STATUS_FAILED, HG_e_VMerror);
						break;
					}
					if (!_hg_allocator_bitmap_is_marked(priv->slave_bitmap, i, j + 1)) {
						used_size -= block->size;
						if (block->lock_count > 0) {
							hg_warning("[BUG] locked block without references [page: %d, index: %d, size: %ld, count: %d]\n",
								   i, j + 1, block->size, block->lock_count);
#if defined (HG_DEBUG)
							abort();
#endif
							error = _hg_allocator_gc_mark(data, _hg_allocator_quark_build(i, j + 1));
							if (!HG_ERROR_IS_SUCCESS (error))
								break;
						}
					}
					aligned_size = HG_ALIGNED_TO (block->size, BLOCK_SIZE) / BLOCK_SIZE;
					j += aligned_size - 1;
				}
			}
		}
	}

	if (HG_ERROR_IS_SUCCESS (error)) {
		hg_debug(HG_MSGCAT_GC, "%ld -> %ld (%ld bytes freed) / %ld",
			 data->used_size, priv->slave.used_size,
			 data->used_size - priv->slave.used_size,
			 data->total_size);
		_hg_allocator_bitmap_destroy(priv->bitmap);
		priv->bitmap = priv->slave_bitmap;
		priv->slave_bitmap = NULL;
		data->used_size = priv->slave.used_size;
	} else {
		hg_debug(HG_MSGCAT_GC, "the garbage collection failed");
		_hg_allocator_bitmap_destroy(priv->slave_bitmap);
		priv->slave_bitmap = NULL;
	}

	G_UNLOCK (allocator);

	return error;
}

static hg_allocator_snapshot_data_t *
_hg_allocator_save_snapshot(hg_allocator_data_t *data)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;
	hg_allocator_snapshot_private_t *snapshot;
	hg_usize_t i, max_page = hg_allocator_get_max_page();

	G_LOCK (allocator);

	snapshot = g_new0(hg_allocator_snapshot_private_t, 1);
	if (snapshot) {
		snapshot->bitmap = _hg_allocator_bitmap_copy(priv->bitmap);
		snapshot->heaps = g_new0(gpointer, max_page);
		for (i = 0; i < max_page; i++) {
			if (priv->heaps[i]) {
				snapshot->heaps[i] = g_malloc(priv->bitmap->size[i] * BLOCK_SIZE);
				memcpy(snapshot->heaps[i], priv->heaps[i], priv->bitmap->size[i] * BLOCK_SIZE);
			}
		}
		snapshot->age = priv->snapshot_age++;
	}

	G_UNLOCK (allocator);

	return (hg_allocator_snapshot_data_t *)snapshot;
}

static gboolean
_hg_allocator_restore_snapshot(hg_allocator_data_t          *data,
			       hg_allocator_snapshot_data_t *snapshot,
			       GHashTable                   *references)
{
	hg_allocator_private_t *priv = (hg_allocator_private_t *)data;
	hg_allocator_snapshot_private_t *spriv = (hg_allocator_snapshot_private_t *)snapshot;
	hg_uint_t i, j, max_page = hg_allocator_get_max_page();
	hg_bool_t retval = FALSE;

	if (spriv->age >= priv->snapshot_age)
		return FALSE;

	G_LOCK (allocator);

	for (i = 0; i < max_page; i++) {
		for (j = 0; j < priv->bitmap->size[i]; j++) {
			hg_bool_t f1, f2;
			hg_usize_t aligned_size;

			f1 = _hg_allocator_bitmap_is_marked(priv->bitmap, i, j + 1);
			f2 = j >= spriv->bitmap->size[i] ? FALSE : _hg_allocator_bitmap_is_marked(spriv->bitmap, i, j + 1);
			if (f1 && f2) {
				hg_allocator_block_t *b1, *b2;
				hg_usize_t idx = j * BLOCK_SIZE;

				b1 = _hg_allocator_get_internal_block_from_page_and_index(priv, i, j + 1, FALSE);
				b2 = (hg_allocator_block_t *)((hg_quark_t)spriv->heaps[i] + idx);
				if (b1->size != b2->size ||
				    b1->age != b2->age) {
					if (g_hash_table_lookup_extended(references,
									 HGQUARK_TO_POINTER (_hg_allocator_quark_build(i, j + 1)),
									 NULL,
									 NULL) ||
					    b1->lock_count > 0) {
						hg_debug(HG_MSGCAT_SNAPSHOT, "detected the block that has different size or different age at [page: %d, index: %d]: current:[age: %ld, size: %d] snapshot:[age: %ld, size: %d]",
							 i, j + 1, b1->size, b1->age, b2->size, b2->age);
						goto error;
					}
				}
				aligned_size = HG_ALIGNED_TO (b1->size, BLOCK_SIZE) / BLOCK_SIZE;
				j += aligned_size - 1;
			} else if (f1 && !f2) {
				hg_allocator_block_t *b1;

				b1 = _hg_allocator_get_internal_block_from_page_and_index(priv, i, j + 1, FALSE);
				if (g_hash_table_lookup_extended(references,
								 HGQUARK_TO_POINTER (_hg_allocator_quark_build(i, j + 1)),
								 NULL,
								 NULL) ||
				    b1->lock_count > 0) {
					hg_debug(HG_MSGCAT_SNAPSHOT, "detected newly allocated block after snapshot at [page: %d, index: %d]", i, j + 1);
					goto error;
				}
				aligned_size = HG_ALIGNED_TO (b1->size, BLOCK_SIZE) / BLOCK_SIZE;
				j += aligned_size - 1;
			} else if (!f1 && f2) {
				hg_allocator_block_t *b2;
				hg_usize_t k, check_size;
				hg_usize_t idx = j * BLOCK_SIZE;

				b2 = (hg_allocator_block_t *)((hg_quark_t)spriv->heaps[i] + idx);
				check_size = aligned_size = HG_ALIGNED_TO (b2->size, BLOCK_SIZE) / BLOCK_SIZE;
				check_size--;
				for (k = j + 1; check_size > 0 && k < priv->bitmap->size[i]; k++) {
					if (_hg_allocator_bitmap_is_marked(priv->bitmap, i, k + 1)) {
						hg_debug(HG_MSGCAT_SNAPSHOT, "detected newly allocated block at [page: %d, index: %ld, size: %ld]", i, k + 1, b2->size);
						goto error;
					}
					check_size--;
				}
				j += aligned_size - 1;
			}
		}
	}

	/* do not free the original heap.
	 * this might causes a segfault
	 * when referring the locked object.
	 * which means still keeping the real pointer.
	 */
	for (i = 0; i < max_page; i++) {
		for (j = 0; j < spriv->bitmap->size[i]; j++) {
			if (_hg_allocator_bitmap_is_marked(spriv->bitmap, i, j + 1)) {
				hg_allocator_block_t *block;
				hg_usize_t aligned_size;

				block = _hg_allocator_get_internal_block_from_page_and_index(priv, i, j + 1, FALSE);
				if (block == NULL)
					goto error;
				if (block->is_restorable) {
					memcpy((((hg_char_t *)priv->heaps[i]) + j * BLOCK_SIZE),
					       (((hg_char_t *)spriv->heaps[i]) + j * BLOCK_SIZE),
					       block->size);
				}
				aligned_size = HG_ALIGNED_TO (block->size, BLOCK_SIZE) / BLOCK_SIZE;
				j += aligned_size - 1;
			}
		}
	}
	_hg_allocator_bitmap_destroy(priv->bitmap);
	priv->bitmap = spriv->bitmap;
	for (i = 0; i < max_page; i++) {
		g_free(spriv->heaps[i]);
	}
	g_free(spriv->heaps);
	data->total_size = snapshot->total_size;
	data->used_size = snapshot->used_size;
	priv->snapshot_age = spriv->age;
	g_free(snapshot);
	retval = TRUE;

  error:
	G_UNLOCK (allocator);

	return retval;
}

static void
_hg_allocator_destroy_snapshot(hg_allocator_data_t          *data,
			       hg_allocator_snapshot_data_t *snapshot)
{
	hg_allocator_snapshot_private_t *spriv = (hg_allocator_snapshot_private_t *)snapshot;
	hg_usize_t i, max_page = hg_allocator_get_max_page();

	_hg_allocator_bitmap_destroy(spriv->bitmap);
	for (i = 0; i < max_page; i++) {
		g_free(spriv->heaps[i]);
	}
	g_free(spriv->heaps);
	g_free(spriv);
}

/*< public >*/
hg_mem_vtable_t *
hg_allocator_get_vtable(void)
{
	return &__hg_allocator_vtable;
}
