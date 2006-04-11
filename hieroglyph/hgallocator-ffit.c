/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator-ffit.c
 * Copyright (C) 2006 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <at@gclab.org>
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
#include <config.h>
#endif

#include <string.h>
#include "hgallocator-ffit.h"
#include "hgallocator-private.h"
#include "hgmem.h"
#include "hgbtree.h"
#include "hgstring.h"


#define BTREE_N_NODE	6


typedef struct _HieroGlyphAllocatorRelocateInfo		HgAllocatorRelocateInfo;
typedef struct _HieroGlyphAllocatorFFitPrivate		HgAllocatorFFitPrivate;
typedef struct _HieroGlyphAllocatorFFitSnapshotPrivate	HgAllocatorFFitSnapshotPrivate;
typedef struct _HieroGlyphMemFFitBlock			HgMemFFitBlock;


struct _HieroGlyphAllocatorRelocateInfo {
	HgBTree *tree;
	HgMemRelocateInfo *info;
};

struct _HieroGlyphAllocatorFFitPrivate {
	GList   *free_block_list;
	GList   *last_free_block;
	HgBTree *used_block_list;
};

struct _HieroGlyphAllocatorFFitSnapshotPrivate {
	GList *restorable_block_list;
	GList *unrestorable_block_list;
};

struct _HieroGlyphMemFFitBlock {
	gint     heap_id;
	gsize    block_size;
	gsize    offset;
};


static gboolean       _hg_allocator_ffit_real_initialize        (HgMemPool         *pool,
								 gsize              prealloc);
static gboolean       _hg_allocator_ffit_real_destroy           (HgMemPool         *pool);
static gboolean       _hg_allocator_ffit_real_resize_pool       (HgMemPool         *pool,
								 gsize              size);
static gpointer       _hg_allocator_ffit_real_alloc             (HgMemPool         *pool,
								 gsize              size,
								 guint              flags);
static void           _hg_allocator_ffit_real_free              (HgMemPool         *pool,
								 gpointer           data);
static gpointer       _hg_allocator_ffit_real_resize            (HgMemObject       *object,
								 gsize              size);
static gboolean       _hg_allocator_ffit_real_garbage_collection(HgMemPool         *pool);
static void           _hg_allocator_ffit_real_gc_mark           (HgMemPool         *pool);
static HgMemSnapshot *_hg_allocator_ffit_real_save_snapshot     (HgMemPool         *pool);
static gboolean       _hg_allocator_ffit_real_restore_snapshot  (HgMemPool         *pool,
								 HgMemSnapshot     *snapshot);
static void           _hg_allocator_ffit_snapshot_real_free     (gpointer           data);
static void           _hg_allocator_ffit_snapshot_real_set_flags(gpointer           data,
								 guint              flags);
static void           _hg_allocator_ffit_snapshot_real_relocate (gpointer           data,
								 HgMemRelocateInfo *info);
static gpointer       _hg_allocator_ffit_snapshot_real_to_string(gpointer           data);


static HgAllocatorVTable __hg_allocator_ffit_vtable = {
	.initialize         = _hg_allocator_ffit_real_initialize,
	.destroy            = _hg_allocator_ffit_real_destroy,
	.resize_pool        = _hg_allocator_ffit_real_resize_pool,
	.alloc              = _hg_allocator_ffit_real_alloc,
	.free               = _hg_allocator_ffit_real_free,
	.resize             = _hg_allocator_ffit_real_resize,
	.garbage_collection = _hg_allocator_ffit_real_garbage_collection,
	.gc_mark            = _hg_allocator_ffit_real_gc_mark,
	.save_snapshot      = _hg_allocator_ffit_real_save_snapshot,
	.restore_snapshot   = _hg_allocator_ffit_real_restore_snapshot,
};

static HgObjectVTable __hg_snapshot_vtable = {
	.free      = _hg_allocator_ffit_snapshot_real_free,
	.set_flags = _hg_allocator_ffit_snapshot_real_set_flags,
	.relocate  = _hg_allocator_ffit_snapshot_real_relocate,
	.dup       = NULL,
	.copy      = NULL,
	.to_string = _hg_allocator_ffit_snapshot_real_to_string,
};


/*
 * Private Functions
 */
/*
 * allocator
 */
#define _hg_allocator_compute_block_size__inline(__hg_acb_size, __hg_acb_ret) \
	G_STMT_START {							\
		gsize __hg_acbt = (__hg_acb_size);			\
		(__hg_acb_ret) = 0;					\
		while (__hg_acbt >= 2) {				\
			__hg_acbt /= 2;					\
			(__hg_acb_ret)++;				\
		}							\
		if ((1 << (__hg_acb_ret)) < (__hg_acb_size))		\
			(__hg_acb_ret)++;				\
		(__hg_acb_ret) = 1 << (__hg_acb_ret);			\
	} G_STMT_END

/* first fit memory allocator */
static gboolean
_hg_allocator_ffit_real_initialize(HgMemPool *pool,
				   gsize      prealloc)
{
	HgAllocatorFFitPrivate *priv;
	HgHeap *heap;
	HgMemFFitBlock *block;
	gsize total_heap_size;

	_hg_allocator_compute_block_size__inline(prealloc, total_heap_size);
	priv = g_new0(HgAllocatorFFitPrivate, 1);
	if (priv == NULL)
		return FALSE;

	heap = hg_heap_new(pool, total_heap_size);
	if (heap == NULL)
		return FALSE;

	pool->total_heap_size = pool->initial_heap_size = total_heap_size;
	pool->used_heap_size = 0;
	g_ptr_array_add(pool->heap_list, heap);

	pool->allocator->private = priv;

	block = g_new(HgMemFFitBlock, 1);
	block->heap_id = heap->serial;
	block->block_size = total_heap_size;
	block->offset = 0;
	priv->free_block_list = g_list_append(NULL, block);
	priv->last_free_block = priv->free_block_list;
	priv->used_block_list = hg_btree_new(BTREE_N_NODE);

	return TRUE;
}

static void
_hg_allocator_ffit_traverse_for_destroy(gpointer key,
					gpointer val,
					gpointer data)
{
	GList *l = data;
	HgMemObject *obj = val;
	HgObject *hobj = (HgObject *)obj->data;

	if (hobj->id == HG_OBJECT_ID && hobj->vtable && hobj->vtable->free)
		l = g_list_append(l, obj->data);
}

static gboolean
_hg_allocator_ffit_real_destroy(HgMemPool *pool)
{
	HgAllocatorFFitPrivate *priv = pool->allocator->private;
	GList *l, *free_list;

	free_list = g_list_alloc(); /* dummy */
	hg_btree_foreach(priv->used_block_list, _hg_allocator_ffit_traverse_for_destroy, free_list);
	for (l = g_list_next(free_list); l != NULL; l = g_list_next(l)) {
		hg_mem_free(l->data);
	}
	g_list_free(free_list);
	hg_btree_destroy(priv->used_block_list);
	for (l = priv->free_block_list; l != NULL; l = g_list_next(l)) {
		g_free(l->data);
	}
	g_list_free(priv->free_block_list);
	g_free(priv);

	return TRUE;
}

static gpointer
_hg_allocator_ffit_real_alloc(HgMemPool *pool,
			      gsize      size,
			      guint      flags)
{
	HgAllocatorFFitPrivate *priv = pool->allocator->private;
	HgMemFFitBlock *block;
	GList *l;
	gsize min_size;
	gsize block_size;
	HgMemObject *obj = NULL;
	HgHeap *heap;

	_hg_allocator_compute_block_size__inline(sizeof (HgMemObject), min_size);
	_hg_allocator_compute_block_size__inline(sizeof (HgMemObject) + size, block_size);
	/* try to find a object from free block list */
	for (l = priv->free_block_list; l != NULL; l = g_list_next(l)) {
		block = l->data;

		if (block->block_size >= block_size) {
			if (block->block_size - block_size <= min_size)
				block_size = block->block_size;
			heap = g_ptr_array_index(pool->heap_list, block->heap_id);
			obj = (gpointer)(gsize)heap->heaps + block->offset;
			obj->id = HG_MEM_HEADER;
			obj->subid = priv->used_block_list;
			obj->heap_id = block->heap_id;
			obj->pool = pool;
			obj->block_size = block_size;
			obj->flags = flags;
			hg_btree_add(priv->used_block_list, obj, obj);

			block->offset += block_size;
			block->block_size -= block_size;
			pool->used_heap_size += block_size;
			heap->used_heap_size += block_size;
			if (block->block_size == 0) {
				g_free(block);
				if (l == priv->last_free_block)
					priv->last_free_block = g_list_previous(priv->last_free_block);
				priv->free_block_list = g_list_delete_link(priv->free_block_list, l);
			}

			return obj->data;
		}
	}

	return NULL;
}

static void
_hg_allocator_ffit_real_free(HgMemPool *pool,
			     gpointer   data)
{
	HgAllocatorFFitPrivate *priv = pool->allocator->private;
	HgMemFFitBlock *block;
	HgMemObject *obj;
	HgHeap *heap;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL ||
	    (obj->subid != priv->used_block_list &&
	     hg_btree_find(priv->used_block_list, obj) == NULL)) {
		g_warning("[BUG] Unknown object %p is given for destroying.", data);
		return;
	} else {
		heap = g_ptr_array_index(pool->heap_list, obj->heap_id);
		hg_btree_remove(priv->used_block_list, obj);
		block = g_new(HgMemFFitBlock, 1);
		block->heap_id = obj->heap_id;
		block->block_size = obj->block_size;
		block->offset = (gsize)obj - (gsize)heap->heaps;
		pool->used_heap_size -= obj->block_size;
		heap->used_heap_size -= obj->block_size;
		if (priv->last_free_block) {
			GList *tmp = g_list_alloc();

			priv->last_free_block->next = tmp;
			tmp->prev = priv->last_free_block;
			tmp->data = block;
			priv->last_free_block = tmp;
		} else {
			priv->last_free_block = priv->free_block_list = g_list_append(priv->free_block_list, block);
		}
	}
}

static void
_hg_allocator_ffit_traverse_for_relocation(gpointer key,
					   gpointer val,
					   gpointer data)
{
	HgMemObject *obj = val, *new_obj;
	HgObject *hobj;
	HgAllocatorRelocateInfo *info = data;

	if ((gsize)obj >= info->info->start &&
	    (gsize)obj <= info->info->end) {
		new_obj = (HgMemObject *)((gsize)obj + info->info->diff);
		hg_btree_add(info->tree, new_obj, new_obj);
		hobj = (HgObject *)new_obj->data;
#ifdef DEBUG_GC
		g_print("DEBUG:  Relocating %p -> %p\n", obj, new_obj);
#endif /* DEBUG_GC */
		if (hobj->id == HG_OBJECT_ID && hobj->vtable && hobj->vtable->relocate) {
			hobj->vtable->relocate(hobj, info->info);
		}
	} else {
		hg_btree_add(info->tree, obj, obj);
	}
}

static void
_hg_allocator_ffit_relocate(HgMemPool         *pool,
			    HgMemRelocateInfo *info)
{
	HgAllocatorFFitPrivate *priv = pool->allocator->private;
	HgAllocatorRelocateInfo ainfo;
	gsize header_size = sizeof (HgMemObject);
	HgBTree *new_tree = hg_btree_new(BTREE_N_NODE);
	GList *list;
	HgMemObject *obj, *new_obj;
	HgObject *hobj;
	gpointer p;

	/* relocate the addresses in the root node */
	for (list = pool->root_node; list != NULL; list = g_list_next(list)) {
		if ((gsize)list->data >= info->start &&
		    (gsize)list->data <= info->end) {
			list->data = (gpointer)((gsize)list->data + info->diff);
		} else {
			/* object that is targetted for relocation will relocates
			 * their member variables later. so we need to ensure
			 * the relocation for others.
			 */
			hobj = (HgObject *)list->data;
			if (hobj->id == HG_OBJECT_ID && hobj->vtable && hobj->vtable->relocate) {
				hobj->vtable->relocate(hobj, info);
			}
		}
	}
	/* relocate the addresses in the stack */
	for (p = _hg_stack_start; p > _hg_stack_end; p--) {
		if ((*(gsize *)p - header_size) >= info->start &&
		    (*(gsize *)p - header_size) <= info->end) {
			if ((obj = hg_btree_find(priv->used_block_list, (gpointer)(*(gsize *)p - header_size))) != NULL) {
				new_obj = (HgMemObject *)((gsize)obj + info->diff);
				*(gsize *)p = (gsize)new_obj->data;
			}
		}
		if (*(gsize *)p >= info->start &&
		    *(gsize *)p <= info->end) {
			if ((obj = hg_btree_find(priv->used_block_list, (gpointer)*(gsize *)p)) != NULL) {
				new_obj = (HgMemObject *)((gsize)obj + info->diff);
				*(gsize *)p = (gsize)new_obj;
			}
		}
	}
	/* tell the object to update the allocated address */
	ainfo.tree = new_tree;
	ainfo.info = info;
	hg_btree_foreach(priv->used_block_list, _hg_allocator_ffit_traverse_for_relocation, &ainfo);
	hg_btree_destroy(priv->used_block_list);
	priv->used_block_list = new_tree;
}

static gpointer
_hg_allocator_ffit_real_resize(HgMemObject *object,
			       gsize        size)
{
	gsize block_size, min_block_size;
	HgMemFFitBlock *block;
	HgMemPool *pool = object->pool;
	HgAllocatorFFitPrivate *priv = pool->allocator->private;

	HG_SET_STACK_END;

	_hg_allocator_compute_block_size__inline(sizeof (HgMemObject) + size, block_size);
	_hg_allocator_compute_block_size__inline(sizeof (HgMemObject), min_block_size);
	if (block_size > object->block_size) {
		gpointer p;

		p = hg_mem_alloc_with_flags(pool, block_size, object->flags);
		HG_SET_STACK_END_AGAIN;
		if (p == NULL) {
			return NULL;
		} else {
			HgMemRelocateInfo info;

			info.start = (gsize)object;
			info.end = (gsize)object;
			info.diff = (gsize)p - (gsize)object->data;
			memcpy(p, object->data, object->block_size - sizeof (HgMemObject));
			_hg_allocator_ffit_relocate(pool, &info);

			return p;
		}
	} else if (block_size < object->block_size &&
		   (object->block_size - block_size) > min_block_size) {
		HgHeap *heap = g_ptr_array_index(pool->heap_list, object->heap_id);

		block = g_new(HgMemFFitBlock, 1);
		block->heap_id = object->heap_id;
		block->block_size = object->block_size - block_size;
		block->offset = (gsize)object - (gsize)heap->heaps + block_size;
		object->block_size = block_size;
		pool->used_heap_size -= block->block_size;
		heap->used_heap_size -= block->block_size;
		if (priv->last_free_block) {
			GList *tmp = g_list_alloc();

			priv->last_free_block->next = tmp;
			tmp->prev = priv->last_free_block;
			tmp->data = block;
			priv->last_free_block = tmp;
		} else {
			priv->last_free_block = priv->free_block_list = g_list_append(priv->free_block_list, block);
		}
	}

	return object->data;
}

static gboolean
_hg_allocator_ffit_real_resize_pool(HgMemPool *pool,
				    gsize      size)
{
	HgAllocatorFFitPrivate *priv = pool->allocator->private;
	HgHeap *heap;
	gsize block_size;
	HgMemFFitBlock *block;

	_hg_allocator_compute_block_size__inline(sizeof (HgMemObject) + size, block_size);
#ifdef DEBUG_GC
	g_print("DEBUG: %s going to be growed up.\n", pool->name);
#endif /* DEBUG_GC */
	if (pool->initial_heap_size > block_size) {
		block_size = pool->initial_heap_size;
	} else {
		/* it may be a good idea to allocate much more memory
		 * because block_size will be used soon.
		 * then need to be resized again.
		 */
		block_size *= 2;
	}
	heap = hg_heap_new(pool, block_size);
	if (heap == NULL)
		return FALSE;
	pool->total_heap_size += block_size;

	block = g_new(HgMemFFitBlock, 1);
	block->heap_id = heap->serial;
	block->block_size = block_size;
	block->offset = 0;
	g_ptr_array_add(pool->heap_list, heap);
	/* it would be a good idea to prepend the block into the free block list
	 * so that there are no free blocks anymore.
	 */
	priv->free_block_list = g_list_prepend(priv->free_block_list, block);
	if (priv->last_free_block == NULL)
		priv->last_free_block = priv->free_block_list;

	return TRUE;
}

static gint
_hg_allocator_ffit_compare(gconstpointer a,
			   gconstpointer b)
{
	const HgMemFFitBlock *aa = a, *bb = b;

	if (aa->heap_id == bb->heap_id)
		return aa->offset - bb->offset;

	return aa->heap_id - bb->heap_id;
}

static void
_hg_allocator_ffit_traverse_for_gc(gpointer key,
				   gpointer val,
				   gpointer data)
{
	GList *l = data;
	HgMemObject *obj = val;

	if (!hg_mem_is_gc_mark(obj) && !hg_mem_is_locked(obj)) {
#ifdef DEBUG_GC
		G_STMT_START {
			HgObject *hobj = (HgObject *)obj->data;
			g_print("DEBUG: being freed: %p\n", obj);
			g_print("DEBUG:   Details:\n");
			if (hobj->id == HG_OBJECT_ID) {
				g_print("DEBUG:     vtable: %p\n", hobj->vtable);
			}
		} G_STMT_END;
#endif /* DEBUG_GC */
		l = g_list_append(l, obj->data);
	} else {
#ifdef DEBUG_GC
		g_print("DEBUG: unmarking %p\n", val);
#endif /* DEBUG_GC */
		hg_mem_gc_unmark(obj);
	}
}

static void
_hg_allocator_ffit_traverse_for_gc_destroyed(gpointer key,
					     gpointer val,
					     gpointer data)
{
	GList *l = data;
	HgMemObject *obj = val;
	HgObject *hobj = (HgObject *)obj->data;

	if (!hg_mem_is_gc_mark(obj)) {
#ifdef DEBUG_GC
		G_STMT_START {
			g_print("DEBUG: being freed: %p\n", obj);
			g_print("DEBUG:   Details:\n");
			if (hobj->id == HG_OBJECT_ID) {
				g_print("DEBUG:     vtable: %p\n", hobj->vtable);
			}
		} G_STMT_END;
#endif /* DEBUG_GC */
		if (hobj->id == HG_OBJECT_ID && hobj->vtable && hobj->vtable->free)
			l = g_list_append(l, obj->data);
	} else {
#ifdef DEBUG_GC
		g_print("DEBUG: unmarking %p\n", val);
#endif /* DEBUG_GC */
		hg_mem_gc_unmark(obj);
	}
}

static gboolean
_hg_allocator_ffit_real_garbage_collection(HgMemPool *pool)
{
	HgAllocatorFFitPrivate *priv = pool->allocator->private;
	GList *l, *ll, *free_list;
	HgMemFFitBlock *block1, *block2;
	gboolean retval = FALSE;

	if (!pool->destroyed) {
		if (!pool->use_gc)
			return FALSE;
		/* keep a mark to avoid collecting garbages */
		pool->allocator->vtable->gc_mark(pool);
	}
	/* do the garbage collection */
	free_list = g_list_alloc(); /* dummy */
	if (!pool->destroyed) {
		hg_btree_foreach(priv->used_block_list, _hg_allocator_ffit_traverse_for_gc, free_list);
	} else {
		hg_btree_foreach(priv->used_block_list, _hg_allocator_ffit_traverse_for_gc_destroyed, free_list);
	}
	for (l = g_list_next(free_list); l != NULL; l = g_list_next(l)) {
		hg_mem_free(l->data);
	}
	g_list_free(free_list);
	if (!pool->destroyed) {
		/* this is actually not a GC. just merge the free spaces to one or more */
		priv->free_block_list = g_list_sort(priv->free_block_list, _hg_allocator_ffit_compare);
		for (l = priv->free_block_list; l != NULL; l = g_list_next(l)) {
		  loop_top:
			ll = g_list_next(l);
			if (ll == NULL)
				break;
			block1 = l->data;
			block2 = ll->data;
			if (block1->heap_id == block2->heap_id &&
			    (block1->offset + block1->block_size) == block2->offset) {
				block1->block_size += block2->block_size;
				g_free(block2);
				priv->free_block_list = g_list_delete_link(priv->free_block_list, ll);
				l = g_list_previous(l);

				retval = TRUE;
				if (l == NULL) {
					l = priv->free_block_list;
					goto loop_top;
				}
			}
		}
		priv->last_free_block = g_list_last(priv->free_block_list);
	}

	return retval;
}

static void
_hg_allocator_ffit_real_gc_mark(HgMemPool *pool)
{
	HgAllocatorFFitPrivate *priv = pool->allocator->private;

	HG_SET_STACK_END;

	G_STMT_START {
		gpointer p;
		HgMemObject *obj;
		gsize header_size = sizeof (HgMemObject);
		GList *list;

		/* trace the root node */
		for (list = pool->root_node; list != NULL; list = g_list_next(list)) {
			hg_mem_get_object__inline(list->data, obj);
			if (obj == NULL) {
				g_warning("Invalid object %p in the root node.", list->data);
			} else {
#ifdef DEBUG_GC
				g_print("DEBUG: marking %p from root node\n", obj);
#endif /* DEBUG_GC */
				if (!hg_mem_is_gc_mark(obj))
					hg_mem_gc_mark(obj);
#ifdef DEBUG_GC
				else {
					g_print("DEBUG: already marked %p\n", obj);
				}
#endif /* DEBUG_GC */
			}
		}
		/* trace the stack */
		for (p = _hg_stack_start; p > _hg_stack_end; p--) {
			if ((obj = hg_btree_find(priv->used_block_list, (gpointer)(*(gsize *)p - header_size))) != NULL) {
#ifdef DEBUG_GC
				g_print("DEBUG: marking %p from stack\n", obj);
#endif /* DEBUG_GC */
				if (!hg_mem_is_gc_mark(obj))
					hg_mem_gc_mark(obj);
#ifdef DEBUG_GC
				else {
					g_print("DEBUG: already marked %p\n", obj);
				}
#endif /* DEBUG_GC */
			}
		}
	} G_STMT_END;
}


/*
 * snapshot
 */
static void
_hg_allocator_ffit_snapshot_real_free(gpointer data)
{
	HgMemSnapshot *snapshot = data;
	HgAllocatorFFitSnapshotPrivate *priv = snapshot->private;
	gint i;
	HgHeap *heap;

	if (priv->restorable_block_list)
		g_list_free(priv->restorable_block_list);
	if (priv->unrestorable_block_list)
		g_list_free(priv->unrestorable_block_list);
	if (snapshot->heap_list) {
		for (i = 0; i < snapshot->n_heaps; i++) {
			heap = g_ptr_array_index(snapshot->heap_list, i);
			hg_heap_free(heap);
		}
		g_ptr_array_free(snapshot->heap_list, TRUE);
	}
}

static void
_hg_allocator_ffit_snapshot_real_set_flags(gpointer data,
					   guint    flags)
{
	HgMemSnapshot *snapshot = data;
	HgAllocatorFFitSnapshotPrivate *priv = snapshot->private;
	GList *list;
	HgMemObject *obj;

	hg_mem_get_object__inline(snapshot->private, obj);
	if (!hg_mem_is_flags__inline(obj, flags))
		hg_mem_add_flags__inline(obj, flags, TRUE);

	for (list = priv->restorable_block_list; list != NULL; list = g_list_next(list)) {
		obj = list->data;

		if (obj->id != HG_MEM_HEADER) {
			g_warning("[BUG] Invalid object %p was given to set a flags in snapshot (restorable).", list->data);
		} else {
#ifdef DEBUG_GC
			G_STMT_START {
				if ((flags & HG_FL_MARK) != 0) {
					g_print("DEBUG: %s: marking snapshot %p from restorable blocks\n", __FUNCTION__, obj);
				}
			} G_STMT_END;
#endif /* DEBUG_GC */
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
	for (list = priv->unrestorable_block_list; list != NULL; list = g_list_next(list)) {
		obj = list->data;

		if (obj->id != HG_MEM_HEADER) {
			g_warning("[BUG] Invalid object %p was given to set a flags in snapshot (unrestorable).", list->data);
		} else {
#ifdef DEBUG_GC
			G_STMT_START {
				if ((flags & HG_FL_MARK) != 0) {
					g_print("DEBUG: %s: marking snapshot %p from unrestorable blocks\n", __FUNCTION__, obj);
				}
			} G_STMT_END;
#endif /* DEBUG_GC */
			if (!hg_mem_is_flags__inline(obj, flags))
				hg_mem_add_flags__inline(obj, flags, TRUE);
		}
	}
}

static void
_hg_allocator_ffit_snapshot_real_relocate(gpointer           data,
					  HgMemRelocateInfo *info)
{
	HgMemSnapshot *snapshot = data;
	HgAllocatorFFitSnapshotPrivate *priv;
	GList *list;
	HgMemObject *obj;
	HgObject *hobj;
	HgHeap *heap, *snapheap;

	if ((gsize)snapshot->private >= info->start &&
	    (gsize)snapshot->private <= info->end) {
		snapshot->private = (gpointer)((gsize)snapshot->private + info->diff);
	}
	priv = snapshot->private;

	for (list = priv->restorable_block_list; list != NULL; list = g_list_next(list)) {
		if ((gsize)list->data >= info->start &&
		    (gsize)list->data <= info->end) {
			list->data = (gpointer)((gsize)list->data + info->diff);
			obj = list->data;
			heap = g_ptr_array_index(obj->pool->heap_list, obj->heap_id);
			snapheap = g_ptr_array_index(snapshot->heap_list, obj->heap_id);
			hobj = (HgObject *)((gsize)snapheap->heaps + (gsize)obj - (gsize)heap->heaps);
			if (hobj->id == HG_OBJECT_ID && hobj->vtable && hobj->vtable->relocate) {
				hobj->vtable->relocate(hobj, info);
			}
		}
	}
	for (list = priv->unrestorable_block_list; list != NULL; list = g_list_next(list)) {
		if ((gsize)list->data >= info->start &&
		    (gsize)list->data <= info->end) {
			list->data = (gpointer)((gsize)list->data + info->diff);
		}
	}
}

static void
_hg_allocator_ffit_snapshot_traverse_save(gpointer key,
					  gpointer val,
					  gpointer data)
{
	HgMemSnapshot *snapshot = data;
	HgAllocatorFFitSnapshotPrivate *priv = snapshot->private;
	HgMemObject *obj = val;

	if (obj->id != HG_MEM_HEADER) {
		g_warning("[BUG] Invalid object %p was given to make a snapshot.", val);
	} else {
		if (hg_mem_is_restorable(obj)) {
			HgHeap *origheap = g_ptr_array_index(obj->pool->heap_list, obj->heap_id);
			HgHeap *heap = g_ptr_array_index(snapshot->heap_list, obj->heap_id);

			priv->restorable_block_list = g_list_append(priv->restorable_block_list,
								    val);
			memcpy(heap->heaps + ((gsize)obj - (gsize)origheap->heaps),
			       obj,
			       obj->block_size);
		} else {
			priv->unrestorable_block_list = g_list_append(priv->unrestorable_block_list,
								      val);
		}
	}
}

static gpointer
_hg_allocator_ffit_snapshot_real_to_string(gpointer data)
{
	HgMemObject *obj;
	HgString *retval;

	hg_mem_get_object__inline(data, obj);
	if (obj == NULL)
		return NULL;
	retval = hg_string_new(obj->pool, 7);
	hg_string_append(retval, "-save-", -1);

	return retval;
}

static HgMemSnapshot *
_hg_allocator_ffit_real_save_snapshot(HgMemPool *pool)
{
	HgMemSnapshot *retval;
	HgAllocatorFFitPrivate *priv = pool->allocator->private;
	HgAllocatorFFitSnapshotPrivate *new_priv;
	gint i;

	retval = hg_mem_alloc(pool, sizeof (HgMemSnapshot));
	if (retval == NULL) {
		g_warning("Failed to create a snapshot.");
		return NULL;
	}
	retval->object.id = HG_OBJECT_ID;
	retval->object.state = hg_mem_pool_get_default_access_mode(pool);
	/* set NULL to avoid the call before finishing an initialization. */
	retval->object.vtable = NULL;

	retval->id = (gsize)pool;
	retval->heap_list = g_ptr_array_new();
	if (retval->heap_list == NULL) {
		g_warning("Failed to create a snapshot.");
		return NULL;
	}
	retval->n_heaps = 0;
	for (i = 0; i < pool->n_heaps; i++) {
		HgHeap *heap;
		HgHeap *origheap = g_ptr_array_index(pool->heap_list, i);

		heap = hg_heap_new(pool, origheap->total_heap_size);
		if (heap == NULL) {
			g_warning("Failed to create a snapshot.");
			return NULL;
		}
		heap->serial = origheap->serial;
		/* need to decrease the number of heap in pool */
		pool->n_heaps--;
		retval->n_heaps++;
		g_ptr_array_add(retval->heap_list, heap);
	}
	retval->private = new_priv = hg_mem_alloc(pool, sizeof (HgAllocatorFFitSnapshotPrivate));
	if (retval->private == NULL) {
		g_warning("Failed to create a snapshot.");
		return NULL;
	}
	new_priv->restorable_block_list = NULL;
	new_priv->unrestorable_block_list = NULL;

	retval->object.vtable = &__hg_snapshot_vtable;

	hg_btree_foreach(priv->used_block_list,
			 _hg_allocator_ffit_snapshot_traverse_save,
			 retval);

	return retval;
}

static void
_hg_allocator_ffit_snapshot_traverse_complex_object(gpointer key,
						    gpointer val,
						    gpointer data)
{
	gboolean *has_complex = data;
	HgMemObject *obj = val;

	if (obj->id != HG_MEM_HEADER) {
		g_warning("[BUG] Invalid object %p was given to be figured the complex object out.", val);
	} else {
		if (!hg_mem_is_gc_mark(obj) &&
		    hg_mem_is_complex_mark(obj))
			*has_complex = TRUE;
	}
}

static gboolean
_hg_allocator_ffit_real_restore_snapshot(HgMemPool     *pool,
					 HgMemSnapshot *snapshot)
{
	GList *list;
	gboolean has_complex = FALSE;
	HgAllocatorFFitSnapshotPrivate *snappriv = snapshot->private;
	HgAllocatorFFitPrivate *priv = pool->allocator->private;

	g_return_val_if_fail (snapshot->id == (gsize)pool, FALSE);

	/* collect the unnecessary objects first */
	hg_mem_garbage_collection(pool);

	/* mark to check if there are another complex object allocated. */
	for (list = snappriv->restorable_block_list; list != NULL; list = g_list_next(list)) {
		HgMemObject *obj = list->data;
		HgHeap *heap = g_ptr_array_index(pool->heap_list, obj->heap_id);
		HgHeap *snapheap = g_ptr_array_index(snapshot->heap_list, obj->heap_id);
		gsize offset = (gsize)obj - (gsize)heap->heaps;

		if (obj->id != HG_MEM_HEADER) {
			g_warning("Invalid object %p was given to set a flags in snapshot.", list->data);
		} else {
			memcpy((gpointer)((gsize)heap->heaps + offset),
			       (gpointer)((gsize)snapheap->heaps + offset),
			       obj->block_size);
#ifdef DEBUG_GC
			g_print("DEBUG: marking %p from snapshot (restorable).\n", obj);
#endif /* DEBUG_GC */
			if (!hg_mem_is_gc_mark(obj))
				hg_mem_gc_mark(obj);
		}
	}
	for (list = snappriv->unrestorable_block_list; list != NULL; list = g_list_next(list)) {
		HgMemObject *obj = list->data;

		if (obj->id != HG_MEM_HEADER) {
			g_warning("Invalid object %p was given to set a flags in snapshot.", list->data);
		} else {
#ifdef DEBUG_GC
			g_print("DEBUG: marking %p from snapshot (unrestorable).\n", obj);
#endif /* DEBUG_GC */
			if (!hg_mem_is_gc_mark(obj))
				hg_mem_gc_mark(obj);
		}
	}
	hg_btree_foreach(priv->used_block_list,
			 _hg_allocator_ffit_snapshot_traverse_complex_object,
			 &has_complex);
	if (!has_complex) {
		hg_mem_garbage_collection(pool);
	}

	return has_complex == FALSE;
}

/*
 * Public Functions
 */
HgAllocatorVTable *
hg_allocator_ffit_get_vtable(void)
{
	return &__hg_allocator_ffit_vtable;
}
