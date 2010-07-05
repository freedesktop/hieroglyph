/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgstack.c
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

#include <stdlib.h>
#include "hgerror.h"
#include "hgmem.h"
#include "hgstack-private.h"
#include "hgstack.h"


static hg_list_t *_hg_list_new  (hg_mem_t   *mem);
static void       _hg_list_free (hg_mem_t   *mem,
                                 hg_list_t  *list);
static gboolean   _hg_stack_push(hg_stack_t *stack,
                                 hg_quark_t  quark);


HG_DEFINE_VTABLE (stack)

/*< private >*/
static gsize
_hg_object_stack_get_capsulated_size(void)
{
	return hg_mem_aligned_size (sizeof (hg_stack_t));
}

static gboolean
_hg_object_stack_initialize(hg_object_t *object,
			    va_list      args)
{
	hg_stack_t *stack = (hg_stack_t *)object;

	stack->max_depth = va_arg(args, gsize);
	stack->stack = NULL;
	stack->last_stack = NULL;
	stack->depth = 0;
	stack->validate_depth = TRUE;

	return TRUE;
}

static void
_hg_object_stack_free(hg_object_t *object)
{
	hg_stack_t *stack = (hg_stack_t *)object;

	_hg_list_free(stack->o.mem, stack->stack);
}

static hg_list_t *
_hg_list_new(hg_mem_t *mem)
{
	hg_quark_t self;
	hg_list_t *l = NULL;

	self = hg_mem_alloc(mem, sizeof (hg_list_t), (gpointer *)&l);
	if (self == Qnil)
		return NULL;

	l->self = self;
	l->data = Qnil;
	l->next = l->prev = NULL;

	return l;
}

static void
_hg_list_free(hg_mem_t  *mem,
	      hg_list_t *list)
{
	hg_list_t *l, *tmp = NULL;

	for (l = list; l != NULL; l = tmp) {
		tmp = l->next;
		hg_mem_free(mem, l->self);
	}
}

static gboolean
_hg_stack_push(hg_stack_t *stack,
	       hg_quark_t  quark)
{
	hg_list_t *l = _hg_list_new(stack->o.mem);

	if (l == NULL)
		return FALSE;

	l->data = quark;
	if (stack->stack == NULL) {
		stack->stack = stack->last_stack = l;
	} else {
		stack->last_stack->next = l;
		l->prev = stack->last_stack;
		stack->last_stack = l;
	}
	stack->depth++;

	return TRUE;
}

/*< public >*/
/**
 * hg_stack_new:
 * @mem:
 * @max_depth:
 *
 * FIXME
 *
 * Returns:
 */
hg_stack_t *
hg_stack_new(hg_mem_t *mem,
	     gsize     max_depth)
{
	hg_stack_t *retval;
	hg_quark_t self;

	hg_return_val_if_fail (mem != NULL, Qnil);
	hg_return_val_if_fail (max_depth > 0, Qnil);

	self = hg_object_new(mem, (gpointer *)&retval, HG_TYPE_STACK, 0, max_depth);
	if (self != Qnil) {
		retval->self = self;
	}

	return retval;
}

/**
 * hg_stack_set_validation:
 * @stack:
 * @flag:
 *
 * FIXME
 */
void
hg_stack_set_validation(hg_stack_t *stack,
			gboolean    flag)
{
	hg_return_if_fail (stack != NULL);

	stack->validate_depth = (flag == TRUE);
}

/**
 * hg_stack_depth:
 * @stack:
 *
 * FIXME
 *
 * Returns:
 */
gsize
hg_stack_depth(hg_stack_t *stack)
{
	hg_return_val_if_fail (stack != NULL, 0);

	return stack->depth;
}

/**
 * hg_stack_push:
 * @stack:
 * @quark:
 *
 * FIXME
 *
 * Returns:
 */
gboolean
hg_stack_push(hg_stack_t *stack,
	      hg_quark_t  quark)
{
	hg_return_val_if_fail (stack != NULL, FALSE);

	if (stack->validate_depth &&
	    stack->depth >= stack->max_depth)
		return FALSE;

	return _hg_stack_push(stack, quark);
}

/**
 * hg_stack_pop:
 * @stack:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_stack_pop(hg_stack_t  *stack,
	     GError     **error)
{
	hg_list_t *l;
	hg_quark_t retval;

	hg_return_val_with_gerror_if_fail (stack != NULL, Qnil, error);

	if (stack->last_stack == NULL)
		return Qnil;

	l = stack->last_stack;
	stack->last_stack = l->prev;
	if (l->prev) {
		l->prev->next = NULL;
		l->prev = NULL;
	}
	if (stack->stack == l) {
		stack->stack = NULL;
	}
	retval = l->data;
	hg_mem_free(stack->o.mem, l->self);
	stack->depth--;

	return retval;
}

/**
 * hg_stack_clear:
 * @stack:
 *
 * FIXME
 */
void
hg_stack_clear(hg_stack_t *stack)
{
	hg_return_if_fail (stack != NULL);

	_hg_list_free(stack->o.mem, stack->stack);
	stack->stack = stack->last_stack = NULL;
	stack->depth = 0;
}

/**
 * hg_stack_index:
 * @stack:
 * @index:
 * @error:
 *
 * FIXME
 *
 * Returns:
 */
hg_quark_t
hg_stack_index(hg_stack_t  *stack,
	       gsize        index,
	       GError     **error)
{
	hg_list_t *l;

	hg_return_val_with_gerror_if_fail (stack != NULL, Qnil, error);
	hg_return_val_with_gerror_if_fail (index < stack->depth, Qnil, error);

	for (l = stack->last_stack; index > 0; l = l->prev, index--);

	return l->data;
}

/**
 * hg_stack_roll:
 * @stack:
 * @n_blocks:
 * @n_times:
 * @error:
 *
 * FIXME
 */
void
hg_stack_roll(hg_stack_t  *stack,
	      gsize        n_blocks,
	      gssize       n_times,
	      GError     **error)
{
	hg_list_t *notargeted_before, *notargeted_after, *beginning, *ending;
	gsize n, i;

	hg_return_with_gerror_if_fail (stack != NULL, error);
	hg_return_with_gerror_if_fail (n_blocks <= stack->depth, error);

	if (n_blocks == 0 ||
	    n_times == 0)
		return;
	n = abs(n_times) % n_blocks;
	if (n != 0) {
		/* find the place that isn't targeted for roll */
		for (notargeted_before = stack->last_stack, i = n_blocks;
		     i > 0;
		     notargeted_before = notargeted_before->prev, i--);
		if (!notargeted_before)
			notargeted_after = stack->stack;
		else
			notargeted_after = notargeted_before->next;
		/* try to find the place to cut off */
		if (n_times > 0) {
			for (beginning = stack->last_stack; n > 1; beginning = beginning->prev, n--);
			ending = beginning->prev;
		} else {
			for (ending = notargeted_after; n > 1; ending = ending->next, n--);
			beginning = ending->next;
		}
		stack->last_stack->next = notargeted_after;
		stack->last_stack->next->prev = stack->last_stack;
		if (notargeted_before) {
			notargeted_before->next = beginning;
			notargeted_before->next->prev = notargeted_before;
		} else {
			stack->stack = beginning;
			stack->stack->prev = NULL;
		}
		stack->last_stack = ending;
		stack->last_stack->next = NULL;
	}
}

/**
 * hg_stack_foreach:
 * @stack:
 * @func:
 * @data:
 * @error:
 *
 * FIXME
 */
void
hg_stack_foreach(hg_stack_t                *stack,
		 hg_array_traverse_func_t   func,
		 gpointer                   data,
		 GError                   **error)
{
	hg_list_t *l;

	hg_return_if_fail (stack != NULL);
	hg_return_if_fail (func != NULL);

	for (l = stack->last_stack; l != NULL; l = l->prev) {
		if (!func(stack->o.mem, l->data, data, error))
			break;
	}
}