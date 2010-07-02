/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgbool.c
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

#include "hgmem.h"
#include "hgbtree-private.h"
#include "hgbtree.h"
#include "main.h"


void tree2string(hg_mem_t   *mem,
		 hg_quark_t  qnode,
		 GString    *string,
		 gboolean    with_nth);

hg_mem_t *mem = NULL;

/** common **/
void
setup(void)
{
	mem = hg_mem_new(65536);
}

void
teardown(void)
{
	gchar *e = hieroglyph_test_pop_error();

	if (e) {
		g_print("E: %s\n", e);
		g_free(e);
	}
	hg_mem_destroy(mem);
}

void
tree2string(hg_mem_t   *mem,
	    hg_quark_t  qnode,
	    GString    *string,
	    gboolean    with_nth)
{
	hg_btree_node_t *qnode_node = NULL;
	hg_quark_t *qnode_keys = NULL, *qnode_vals = NULL, *qnode_nodes = NULL;
	static gint depth = 0;
	gsize i;

	if (qnode == Qnil) {
		g_string_append_c(string, '.');
		return;
	}
	g_string_append_c(string, '(');

	HG_BTREE_NODE_LOCK (mem, qnode, qnode, "", NULL);

	depth++;
	if (with_nth)
		g_string_append_printf(string, "[%" G_GSIZE_FORMAT "]", qnode_node->n_data);
	for (i = 0; i < qnode_node->n_data; i++) {
		tree2string(qnode_node->mem, qnode_nodes[i], string, with_nth);
		g_string_append_printf(string, "%" G_GSIZE_FORMAT, qnode_vals[i]);
	}
	tree2string(qnode_node->mem, qnode_nodes[qnode_node->n_data], string, with_nth);

	HG_BTREE_NODE_UNLOCK (mem, qnode, qnode);

	g_string_append_c(string, ')');
	depth--;
}

static gboolean
_mark(hg_mem_t    *mem,
      hg_quark_t   qkey,
      hg_quark_t   qval,
      gpointer     data,
      GError     **error)
{
	gint *x = data;

	x[qkey] = qval + 1;

	return TRUE;
}

/** test cases **/
TDEF (hg_btree_new)
{
	hg_quark_t q;

	q = hg_btree_new(mem, 0, NULL);
	fail_unless(q == Qnil, "Unexpected result to create 0-sized btree.");
	g_free(hieroglyph_test_pop_error());
	q = hg_btree_new(mem, 2, NULL);
	fail_unless(q != Qnil, "Unable to create a btree object.");
	hg_btree_destroy(mem, q);
} TEND

TDEF (hg_btree_add)
{
	hg_btree_t *tree;
	hg_quark_t q;
	GString *string = g_string_new(NULL);
	gsize i, size;
	const gchar *test = "(((((.0.1.)2(.3.4.)5(.6.7.))8((.9.10.)11(.12.13.)14(.15.16.))17((.18.19.)20(.21.22.)23(.24.25.)))26(((.27.28.)29(.30.31.)32(.33.34.))35((.36.37.)38(.39.40.)41(.42.43.))44((.45.46.)47(.48.49.)50(.51.52.)))53(((.54.55.)56(.57.58.)59(.60.61.))62((.63.64.)65(.66.67.)68(.69.70.))71((.72.73.)74(.75.76.)77(.78.79.))))80((((.81.82.)83(.84.85.)86(.87.88.))89((.90.91.)92(.93.94.)95(.96.97.))98((.99.100.)101(.102.103.)104(.105.106.)))107(((.108.109.)110(.111.112.)113(.114.115.))116((.117.118.)119(.120.121.)122(.123.124.))125((.126.127.)128(.129.130.)131(.132.133.)))134(((.135.136.)137(.138.139.)140(.141.142.))143((.144.145.)146(.147.148.)149(.150.151.))152((.153.154.)155(.156.157.)158(.159.160.))))161((((.162.163.)164(.165.166.)167(.168.169.))170((.171.172.)173(.174.175.)176(.177.178.))179((.180.181.)182(.183.184.)185(.186.187.)))188(((.189.190.)191(.192.193.)194(.195.196.))197((.198.199.)200(.201.202.)203(.204.205.))206((.207.208.)209(.210.211.)212(.213.214.)))215(((.216.217.)218(.219.220.)221(.222.223.))224((.225.226.)227(.228.229.)230(.231.232.))233((.234.235.)236(.237.238.)239(.240.241.))242((.243.244.)245(.246.247.)248(.249.250.)251(.252.253.254.255.)))))";
	gint xx[256];

	q = hg_btree_new(mem, 2, (gpointer *)&tree);
	fail_unless(q != Qnil, "Unable to create a btree object.");
	hg_btree_add(tree, 0, 0, NULL);
	tree2string(mem, tree->root, string, TRUE);
	fail_unless(strcmp(string->str, "([1].0.)") == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", "([1].0.)", string->str);
	hg_btree_add(tree, 1, 1, NULL);
	g_string_erase(string, 0, -1);
	tree2string(mem, tree->root, string, TRUE);
	fail_unless(strcmp(string->str, "([2].0.1.)") == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", "([2].0.1.)", string->str);
	hg_btree_add(tree, 2, 2, NULL);
	hg_btree_add(tree, 3, 3, NULL);
	g_string_erase(string, 0, -1);
	tree2string(mem, tree->root, string, TRUE);
	fail_unless(strcmp(string->str, "([4].0.1.2.3.)") == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", "([4].0.1.2.3.)", string->str);
	hg_btree_add(tree, 4, 4, NULL);
	g_string_erase(string, 0, -1);
	tree2string(mem, tree->root, string, TRUE);
	fail_unless(strcmp(string->str, "([1]([2].0.1.)2([2].3.4.))") == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", "([1]([2].0.1.)2([2].3.4.))", string->str);
	g_string_erase(string, 0, -1);
	for (i = 5; i < 256; i++) {
		hg_btree_add(tree, i, i, NULL);
	}
	tree2string(mem, tree->root, string, FALSE);
	fail_unless(strcmp(string->str, test) == 0, "Unexpected result in the tree structure: expect: %s, actual: %s", test, string->str);
	fail_unless(hg_btree_length(tree, NULL) == 256, "Unexpected result in the size of the tree");

	memset(xx, 0, sizeof (gint) * 256);
	hg_btree_foreach(tree, _mark, xx, NULL);
	for (i = 0; i < 256; i++) {
		fail_unless(xx[i] != 0, "Unexpected result in traversing the tree at %" G_GSIZE_FORMAT, i);
	}
	for (i = 0; i < 256; i++) {
		fail_unless(hg_btree_find(tree, i, NULL) == i, "Unexpected result to find out: key %" G_GSIZE_FORMAT, i);
	}

	for (i = 0; i < 256; i++) {
		GError *err = NULL;

		g_string_erase(string, 0, -1);
		tree2string(mem, tree->root, string, TRUE);
		g_print("%" G_GSIZE_FORMAT ": %s\n", i, string->str);
		hg_btree_remove(tree, i, &err);
		if (err) {
			g_print("%s\n", err->message);
		}
		fail_unless(err == NULL, "Unexpected result to remove a node: %" G_GSIZE_FORMAT, i);
	}
	size = hg_btree_length(tree, NULL);
	fail_unless(size == 0, "Unexpected result in the size of the tree after removing all: actual: %" G_GSIZE_FORMAT, size);
} TEND

TDEF (hg_btree_remove)
{
} TEND

TDEF (hg_btree_find)
{
	/* should be done the above */
} TEND

TDEF (hg_btree_foreach)
{
	/* should be done the above */
} TEND

TDEF (hg_btree_length)
{
	/* should be done the above */
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hgbtree.h");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_set_timeout(tc, 60);

	T (hg_btree_new);
	T (hg_btree_add);
	T (hg_btree_remove);
	T (hg_btree_find);
	T (hg_btree_foreach);
	T (hg_btree_length);

	suite_add_tcase(s, tc);

	return s;
}