/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgfile.h
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
#ifndef __HIEROGLYPH_HGFILE_H__
#define __HIEROGLYPH_HGFILE_H__

#include <hieroglyph/hgobject.h>
#include <hieroglyph/hgstring.h>

G_BEGIN_DECLS

typedef enum _hg_file_io_t			hg_file_io_t;
typedef enum _hg_file_mode_t			hg_file_mode_t;
typedef enum _hg_file_pos_t			hg_file_pos_t;
typedef enum _hg_file_backend_t			hg_file_backend_t;
typedef struct _hg_file_master_t		hg_file_master_t;
typedef struct _hg_file_vtable_t		hg_file_vtable_t;
typedef struct _hg_file_t			hg_file_t;
typedef struct _hg_file_io_data_t		hg_file_io_data_t;
typedef struct _hg_file_mmapped_io_data_t	hg_file_mmapped_io_data_t;

enum _hg_file_io_t {
	HG_FILE_IO_FILE = 0,
	HG_FILE_IO_STDIN,
	HG_FILE_IO_STDOUT,
	HG_FILE_IO_STDERR,
	HG_FILE_IO_LINEEDIT,
	HG_FILE_IO_STATEMENTEDIT,
	HG_FILE_IO_END
};
enum _hg_file_mode_t {
	HG_FILE_IO_MODE_READ      = 1 << 0,
	HG_FILE_IO_MODE_WRITE     = 1 << 1,
	HG_FILE_IO_MODE_READWRITE = HG_FILE_IO_MODE_READ|HG_FILE_IO_MODE_WRITE,
	HG_FILE_IO_MODE_APPEND    = 1 << 2,
	HG_FILE_IO_MODE_END       = 1 << 3,
};
enum _hg_file_pos_t {
	HG_FILE_POS_BEGIN   = SEEK_SET,
	HG_FILE_POS_CURRENT = SEEK_CUR,
	HG_FILE_POS_END     = SEEK_END
};
struct _hg_file_vtable_t {
	gboolean (* open)      (hg_file_t      *file,
				gpointer        user_data,
				GError        **error);
	void     (* close)     (hg_file_t      *file,
				gpointer        user_data,
				GError        **error);
	gssize   (* read)      (hg_file_t      *file,
				gpointer        user_data,
				gpointer        buffer,
				gsize           size,
				gsize           n,
				GError        **error);
	gssize   (* write)     (hg_file_t      *file,
				gpointer        user_data,
				gconstpointer   buffer,
				gsize           size,
				gsize           n,
				GError        **error);
	gboolean (* flush)     (hg_file_t      *file,
				gpointer        user_data,
				GError        **error);
	gssize   (* seek)      (hg_file_t      *file,
				gpointer        user_data,
				gssize          offset,
				hg_file_pos_t   whence,
				GError        **error);
	gboolean (* is_eof)    (hg_file_t      *file,
				gpointer        user_data);
	void     (* clear_eof) (hg_file_t      *file,
				gpointer        user_data);
};
struct _hg_file_t {
	hg_object_t        o;
	hg_quark_t         qfilename;
	hg_file_io_t       io_type;
	hg_file_mode_t     mode;
	hg_file_vtable_t  *vtable;
	gsize              size;
	gsize              position;
	gsize              line;
	gboolean           is_closed;
	hg_file_io_data_t *data;
	gpointer           user_data;
};
struct _hg_file_io_data_t {
	hg_quark_t self;
	gboolean   is_eof;
	gint       fd;
	gpointer   mmapped_buffer;
};


hg_object_vtable_t *hg_object_file_get_vtable(void);
hg_quark_t          hg_file_new              (hg_mem_t          *mem,
					      const gchar       *name,
					      hg_file_mode_t     mode,
					      GError           **error,
					      gpointer          *ret);
hg_quark_t          hg_file_new_with_vtable  (hg_mem_t          *mem,
					      const gchar       *name,
					      hg_file_mode_t     mode,
					      hg_file_vtable_t  *vtable,
					      gpointer           user_data,
					      GError           **error,
					      gpointer          *ret);
hg_quark_t          hg_file_new_with_string  (hg_mem_t          *mem,
					      const gchar       *name,
					      hg_file_mode_t     mode,
					      hg_string_t       *in,
					      hg_string_t       *out,
					      GError           **error,
					      gpointer          *ret);
gssize              hg_file_read             (hg_file_t         *file,
					      gpointer           buffer,
					      gsize              size,
					      gsize              n,
					      GError           **error);
gssize              hg_file_write            (hg_file_t         *file,
					      gpointer           buffer,
					      gsize              size,
					      gsize              n,
					      GError           **error);
gboolean            hg_file_flush            (hg_file_t         *file,
					      GError           **error);
gssize              hg_file_seek             (hg_file_t         *file,
					      gssize             offset,
					      hg_file_pos_t      whence,
					      GError           **error);
gboolean            hg_file_is_eof           (hg_file_t         *file);
void                hg_file_clear_eof        (hg_file_t         *file);
gssize              hg_file_append_printf    (hg_file_t         *file,
					      gchar const       *format,
					      ...);
gssize              hg_file_append_vprintf   (hg_file_t         *file,
					      gchar const       *format,
					      va_list            args);

G_END_DECLS

#endif /* __HIEROGLYPH_HGFILE_H__ */
