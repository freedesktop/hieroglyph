/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgallocator.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hgallocator.h"
#include "hgallocator-private.h"
#include "main.h"


hg_mem_vtable_t *vtable = NULL;

/** common **/
void
setup(void)
{
	vtable = hg_allocator_get_vtable();
}

void
teardown(void)
{
}

/** test cases **/
TDEF (initialize)
{
} TEND

TDEF (finalize)
{
} TEND

TDEF (resize_heap)
{
} TEND

TDEF (alloc)
{
} TEND

TDEF (free)
{
} TEND

TDEF (lock_object)
{
} TEND

TDEF (unlock_object)
{
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_allocator_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (initialize);
	T (finalize);
	T (resize_heap);
	T (alloc);
	T (free);
	T (lock_object);
	T (unlock_object);

	suite_add_tcase(s, tc);

	return s;
}
