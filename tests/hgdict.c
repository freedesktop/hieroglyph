/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgdict.c
 * Copyright (C) 2010-2011 Akira TAGOH
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

#include "main.h"
#include "hgmem.h"
#include "hgdict-private.h"
#include "hgdict.h"


void tree2string(hg_mem_t   *mem,
		 hg_quark_t  qnode,
		 GString    *string,
		 hg_bool_t    with_nth);

hg_mem_t *mem = NULL;
hg_object_vtable_t *vtable = NULL, *node_vtable = NULL;

/** common **/
void
setup(void)
{
	hg_object_init();
	HG_DICT_INIT;
	mem = hg_mem_spool_new(HG_MEM_TYPE_LOCAL, 100000);
	vtable = hg_object_dict_get_vtable();
	node_vtable = hg_object_dict_node_get_vtable();
	_hg_dict_node_set_size(2);
}

void
teardown(void)
{
	hg_char_t *e = hieroglyph_test_pop_error();

	if (e) {
		g_print("E: %s\n", e);
		g_free(e);
	}
	hg_mem_spool_destroy(mem);
}

void
tree2string(hg_mem_t   *mem,
	    hg_quark_t  qnode,
	    GString    *string,
	    hg_bool_t    with_nth)
{
	hg_dict_node_t *qnode_node = NULL;
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;
	static hg_int_t depth = 0;
	hg_usize_t i;

	if (qnode == Qnil) {
		g_string_append_c(string, '.');
		return;
	}
	g_string_append_c(string, '(');

	HG_DICT_NODE_LOCK (mem, qnode, qnode, "");

	depth++;
	if (with_nth)
		g_string_append_printf(string, "[%ld]", qnode_node->n_data);
	for (i = 0; i < qnode_node->n_data; i++) {
		tree2string(qnode_node->o.mem, qnode_nodes[i], string, with_nth);
		g_string_append_printf(string, "%ld", qnode_vals[i]);
	}
	tree2string(qnode_node->o.mem, qnode_nodes[qnode_node->n_data], string, with_nth);

	HG_DICT_NODE_UNLOCK (mem, qnode, qnode);

	g_string_append_c(string, ')');
	depth--;
}

static hg_bool_t
_mark(hg_mem_t     *mem,
      hg_quark_t    qkey,
      hg_quark_t    qval,
      hg_pointer_t  data)
{
	hg_int_t *x = data;

	x[qkey] = qval + 1;

	return TRUE;
}

/** test cases **/
TDEF (get_capsulated_size)
{
	hg_usize_t size;

	size = vtable->get_capsulated_size();
	fail_unless(size == sizeof (hg_dict_t), "Obtaining the different size: expect: %lx actual: %lx", sizeof (hg_dict_t), size);
	size = node_vtable->get_capsulated_size();
	fail_unless(size == sizeof (hg_dict_node_t), "Obtaining the different size: expect: %lx actual: %lx", sizeof (hg_dict_node_t), size);
} TEND

static hg_bool_t
_gc_func(hg_mem_t     *mem,
	 hg_pointer_t  data)
{
	hg_dict_t *d = data;

	if (data == NULL)
		return TRUE;

	return hg_mem_gc_mark(d->o.mem, d->o.self);
}

TDEF (gc_mark)
{
	hg_quark_t q HG_GNUC_UNUSED;
	hg_dict_t *d;
	hg_size_t size = 0;
	hg_mem_t *m = hg_mem_spool_new(HG_MEM_TYPE_LOCAL, 65535);
	hg_usize_t i;

	q = hg_dict_new(m, 10, TRUE, (hg_pointer_t *)&d);
	hg_dict_add(d, 0, 0, FALSE);
	hg_mem_spool_set_gc_procedure(m, _gc_func, d);
	size = hg_mem_spool_run_gc(m);
	fail_unless(size == 0, "missing something for marking: %ld bytes freed [take 1]", size);
	for (i = 1; i < 256; i++) {
		hg_dict_add(d, i, i, FALSE);
	}
	size = hg_mem_spool_run_gc(m);
	fail_unless(size == 0, "missing something for marking: %ld bytes freed [take 2]", size);
	size = hg_mem_spool_run_gc(m);
	fail_unless(size == 0, "missing something for marking: %ld bytes freed [take 3]", size);
} TEND

TDEF (hg_dict_new)
{
	hg_quark_t q;

	q = hg_dict_new(mem, 0, TRUE, NULL);
	fail_unless(q != Qnil, "Unexpected result to create 0-sized dict.");
	hg_mem_free(mem, q);
	q = hg_dict_new(mem, HG_DICT_MAX_SIZE + 1, TRUE, NULL);
	fail_unless(q == Qnil, "Unable to create a dict object.");
	g_free(hieroglyph_test_pop_error());
} TEND

TDEF (hg_dict_add)
{
	hg_dict_t *dict;
	hg_quark_t q;
	GString *string = g_string_new(NULL);
	hg_usize_t i, size;
	const hg_char_t *test = "(((((.0.1.)2(.3.4.)5(.6.7.))8((.9.10.)11(.12.13.)14(.15.16.))17((.18.19.)20(.21.22.)23(.24.25.)))26(((.27.28.)29(.30.31.)32(.33.34.))35((.36.37.)38(.39.40.)41(.42.43.))44((.45.46.)47(.48.49.)50(.51.52.)))53(((.54.55.)56(.57.58.)59(.60.61.))62((.63.64.)65(.66.67.)68(.69.70.))71((.72.73.)74(.75.76.)77(.78.79.))))80((((.81.82.)83(.84.85.)86(.87.88.))89((.90.91.)92(.93.94.)95(.96.97.))98((.99.100.)101(.102.103.)104(.105.106.)))107(((.108.109.)110(.111.112.)113(.114.115.))116((.117.118.)119(.120.121.)122(.123.124.))125((.126.127.)128(.129.130.)131(.132.133.)))134(((.135.136.)137(.138.139.)140(.141.142.))143((.144.145.)146(.147.148.)149(.150.151.))152((.153.154.)155(.156.157.)158(.159.160.))))161((((.162.163.)164(.165.166.)167(.168.169.))170((.171.172.)173(.174.175.)176(.177.178.))179((.180.181.)182(.183.184.)185(.186.187.)))188(((.189.190.)191(.192.193.)194(.195.196.))197((.198.199.)200(.201.202.)203(.204.205.))206((.207.208.)209(.210.211.)212(.213.214.)))215(((.216.217.)218(.219.220.)221(.222.223.))224((.225.226.)227(.228.229.)230(.231.232.))233((.234.235.)236(.237.238.)239(.240.241.))242((.243.244.)245(.246.247.)248(.249.250.)251(.252.253.254.255.)))))";
	hg_int_t xx[256];

	q = hg_dict_new(mem, 256, TRUE, (hg_pointer_t *)&dict);
	fail_unless(q != Qnil, "Unable to create a dict object.");
	hg_dict_add(dict, 0, 0, FALSE);
	tree2string(mem, dict->qroot, string, TRUE);
	fail_unless(strcmp(string->str, "([1].0.)") == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", "([1].0.)", string->str);
	hg_dict_add(dict, 1, 1, FALSE);
	g_string_erase(string, 0, -1);
	tree2string(mem, dict->qroot, string, TRUE);
	fail_unless(strcmp(string->str, "([2].0.1.)") == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", "([2].0.1.)", string->str);
	hg_dict_add(dict, 2, 2, FALSE);
	hg_dict_add(dict, 3, 3, FALSE);
	g_string_erase(string, 0, -1);
	tree2string(mem, dict->qroot, string, TRUE);
	fail_unless(strcmp(string->str, "([4].0.1.2.3.)") == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", "([4].0.1.2.3.)", string->str);
	hg_dict_add(dict, 4, 4, FALSE);
	g_string_erase(string, 0, -1);
	tree2string(mem, dict->qroot, string, TRUE);
	fail_unless(strcmp(string->str, "([1]([2].0.1.)2([2].3.4.))") == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", "([1]([2].0.1.)2([2].3.4.))", string->str);
	g_string_erase(string, 0, -1);
	for (i = 5; i < 256; i++) {
		hg_dict_add(dict, i, i, FALSE);
	}
	tree2string(mem, dict->qroot, string, FALSE);
	fail_unless(strcmp(string->str, test) == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", test, string->str);
	fail_unless(hg_dict_length(dict) == 256, "Unexpected result in the size of the tree");

	memset(xx, 0, sizeof (hg_int_t) * 256);
	hg_dict_foreach(dict, _mark, xx);
	for (i = 0; i < 256; i++) {
		fail_unless(xx[i] != 0, "Unexpected result in traversing the tree at %lx", i);
	}
	for (i = 0; i < 256; i++) {
		fail_unless(hg_dict_lookup(dict, i) == i, "Unexpected result to find out: key %lx", i);
	}

	for (i = 0; i < 256; i++) {
		g_string_erase(string, 0, -1);
		tree2string(mem, dict->qroot, string, TRUE);
#if 0
		g_print("%lx: %s\n", i, string->str);
#endif
		hg_dict_remove(dict, i);
		fail_unless(HG_ERROR_IS_SUCCESS0 (), "Unexpected result to remove a node: %lx", i);
	}
	size = hg_dict_length(dict);
	fail_unless(size == 0, "Unexpected result in the size of the tree after removing all: actual: %lx", size);
} TEND

TDEF (hg_dict_remove)
{
} TEND

TDEF (hg_dict_lookup)
{
	hg_dict_t *dict;
	hg_quark_t q, t, t2;

	q = hg_dict_new(mem, 2, TRUE, (hg_pointer_t *)&dict);
	fail_unless(q != Qnil, "Unable to create a dict object.");
	t = hg_quark_new(HG_TYPE_BOOL, TRUE);
	hg_quark_set_acl(&t, HG_ACL_READABLE|HG_ACL_WRITABLE|HG_ACL_EXECUTABLE|HG_ACL_ACCESSIBLE);
	fail_unless(hg_quark_is_readable(t), "Failed for prechecking read permission");
	fail_unless(hg_quark_is_writable(t), "Failed for prechecking write permission");
	fail_unless(hg_quark_is_executable(t), "Failed for prechecking execute permission");
	hg_dict_add(dict, t, t, FALSE);
	t2 = hg_dict_lookup(dict, t);
	fail_unless(t == t2, "Unexpected result in lookup: expected: %lx, actual: %lx", t, t2);
	fail_unless(hg_quark_is_readable(t2), "Failed for post-checking read permission");
	fail_unless(hg_quark_is_writable(t2), "Failed for post-checking write permission");
	fail_unless(hg_quark_is_executable(t2), "Failed for post-checking execute permission");
} TEND

TDEF (hg_dict_foreach)
{
	/* should be done the above */
} TEND

TDEF (hg_dict_length)
{
	/* should be done the above */
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgdict.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_set_timeout(tc, 60);

	T (get_capsulated_size);
	T (gc_mark);
	T (hg_dict_new);
	T (hg_dict_add);
	T (hg_dict_remove);
	T (hg_dict_lookup);
	T (hg_dict_foreach);
	T (hg_dict_length);

	suite_add_tcase(s, tc);

	return s;
}
