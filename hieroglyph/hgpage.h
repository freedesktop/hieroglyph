/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgpage.h
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
#ifndef __HG_PAGE_H__
#define __HG_PAGE_H__

#include <hieroglyph/hgtypes.h>

G_BEGIN_DECLS


HgPage   *hg_page_new              (void);
HgPage   *hg_page_new_with_pagesize(HgPageSize  size);
HgPage   *hg_page_new_with_size    (gdouble     width,
				    gdouble     height);
void      hg_page_destroy          (HgPage     *page);
gboolean  hg_page_get_size         (HgPageSize  size,
				    gdouble    *width,
				    gdouble    *height);
void      hg_page_append_node      (HgPage     *page,
				    HgRender   *render);


G_END_DECLS

#endif /* __HG_PAGE_H__ */
