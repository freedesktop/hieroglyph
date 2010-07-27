/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgtypes.h
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
#ifndef __HIEROGLYPH_HGTYPES_H__
#define __HIEROGLYPH_HGTYPES_H__

#include <hieroglyph/hgmacros.h>

G_BEGIN_DECLS

#define Qnil	(hg_quark_t)0

/* hgquark.h */
typedef gint64				hg_quark_t;

/* hgmem.h */
typedef struct _hg_mem_t		hg_mem_t;
typedef struct _hg_mem_vtable_t		hg_mem_vtable_t;
typedef struct _hg_allocator_data_t	hg_allocator_data_t;

typedef struct _hg_bs_template_t	hg_bs_template_t;
typedef enum _hg_system_encoding_t	hg_system_encoding_t;

/* hgmem.h */
struct _hg_mem_vtable_t {
	gpointer   (* initialize)      (void);
	void       (* finalize)        (hg_allocator_data_t *data);
	gboolean   (* resize_heap)     (hg_allocator_data_t *data,
					gsize                size);
	hg_quark_t (* alloc)           (hg_allocator_data_t *data,
					gsize                size,
					gpointer            *ret);
	hg_quark_t (* realloc)         (hg_allocator_data_t *data,
					hg_quark_t           quark,
					gsize                size,
					gpointer            *ret);
	void       (* free)            (hg_allocator_data_t *data,
					hg_quark_t           quark);
	gpointer   (* lock_object)     (hg_allocator_data_t *data,
					hg_quark_t           quark);
	void       (* unlock_object)   (hg_allocator_data_t *data,
					hg_quark_t           quark);
	gboolean   (* gc_mark)         (hg_allocator_data_t *data,
					hg_quark_t           quark);
	gboolean   (* collect_garbage) (hg_allocator_data_t *data,
					hg_quark_t          *root);
};
struct _hg_allocator_data_t {
	gsize total_size;
	gsize used_size;
};

/* hgdict.h */
typedef struct _hg_dict_t		hg_dict_t;

/* hgname.h */
typedef struct _hg_name_t		hg_name_t;

/* hgobject.h */
typedef gboolean (* hg_gc_iterate_func_t)	(hg_quark_t   qdata,
						 gpointer     user_data,
						 GError     **error);
/* hgscanner.h */
typedef struct _hg_scanner_t		hg_scanner_t;

/* hgvm.h */
typedef struct _hg_vm_t			hg_vm_t;
typedef enum _hg_vm_langlevel_t		hg_vm_langlevel_t;

enum _hg_vm_langlevel_t {
	HG_LANG_LEVEL_1 = 0,
	HG_LANG_LEVEL_2,
	HG_LANG_LEVEL_3,
	HG_LANG_LEVEL_END
};

/**/
struct _hg_bs_template_t {
	union {
		guint8 is_executable:1;
		guint8 type:7;
	} h;
	guint8 is_zero;
};

enum _hg_system_encoding_t {
	HG_enc_abs = 0,
	HG_enc_add,
	HG_enc_aload,
	HG_enc_anchorsearch,
	HG_enc_and,
	HG_enc_arc,
	HG_enc_arcn,
	HG_enc_arct,
	HG_enc_arcto,
	HG_enc_array,

	HG_enc_ashow,
	HG_enc_astore,
	HG_enc_awidthshow,
	HG_enc_begin,
	HG_enc_bind,
	HG_enc_bitshift,
	HG_enc_ceiling,
	HG_enc_charpath,
	HG_enc_clear,
	HG_enc_cleartomark,

	HG_enc_clip,
	HG_enc_clippath,
	HG_enc_closepath,
	HG_enc_concat,
	HG_enc_concatmatrix,
	HG_enc_copy,
	HG_enc_count,
	HG_enc_counttomark,
	HG_enc_currentcmykcolor,
	HG_enc_currentdash,

	HG_enc_currentdict,
	HG_enc_currentfile,
	HG_enc_currentfont,
	HG_enc_currentgray,
	HG_enc_currentgstate,
	HG_enc_currenthsbcolor,
	HG_enc_currentlinecap,
	HG_enc_currentlinejoin,
	HG_enc_currentlinewidth,
	HG_enc_currentmatrix,

	HG_enc_currentpoint,
	HG_enc_currentrgbcolor,
	HG_enc_currentshared,
	HG_enc_curveto,
	HG_enc_cvi,
	HG_enc_cvlit,
	HG_enc_cvn,
	HG_enc_cvr,
	HG_enc_cvrs,
	HG_enc_cvs,

	HG_enc_cvx,
	HG_enc_def,
	HG_enc_defineusername,
	HG_enc_dict,
	HG_enc_div,
	HG_enc_dtransform,
	HG_enc_dup,
	HG_enc_end,
	HG_enc_eoclip,
	HG_enc_eofill,

	HG_enc_eoviewclip,
	HG_enc_eq,
	HG_enc_exch,
	HG_enc_exec,
	HG_enc_exit,
	HG_enc_file,
	HG_enc_fill,
	HG_enc_findfont,
	HG_enc_flattenpath,
	HG_enc_floor,

	HG_enc_flush,
	HG_enc_flushfile,
	HG_enc_for,
	HG_enc_forall,
	HG_enc_ge,
	HG_enc_get,
	HG_enc_getinterval,
	HG_enc_grestore,
	HG_enc_gsave,
	HG_enc_gstate,

	HG_enc_gt,
	HG_enc_identmatrix,
	HG_enc_idiv,
	HG_enc_idtransform,
	HG_enc_if,
	HG_enc_ifelse,
	HG_enc_image,
	HG_enc_imagemask,
	HG_enc_index,
	HG_enc_ineofill,

	HG_enc_infill,
	HG_enc_initviewclip,
	HG_enc_inueofill,
	HG_enc_inufill,
	HG_enc_invertmatrix,
	HG_enc_itransform,
	HG_enc_known,
	HG_enc_le,
	HG_enc_length,
	HG_enc_lineto,

	HG_enc_load,
	HG_enc_loop,
	HG_enc_lt,
	HG_enc_makefont,
	HG_enc_matrix,
	HG_enc_maxlength,
	HG_enc_mod,
	HG_enc_moveto,
	HG_enc_mul,
	HG_enc_ne,

	HG_enc_neg,
	HG_enc_newpath,
	HG_enc_not,
	HG_enc_null,
	HG_enc_or,
	HG_enc_pathbbox,
	HG_enc_pathforall,
	HG_enc_pop,
	HG_enc_print,
	HG_enc_printobject,

	HG_enc_put,
	HG_enc_putinterval,
	HG_enc_rcurveto,
	HG_enc_read,
	HG_enc_readhexstring,
	HG_enc_readline,
	HG_enc_readstring,
	HG_enc_rectclip,
	HG_enc_rectfill,
	HG_enc_rectstroke,

	HG_enc_rectviewclip,
	HG_enc_repeat,
	HG_enc_restore,
	HG_enc_rlineto,
	HG_enc_rmoveto,
	HG_enc_roll,
	HG_enc_rotate,
	HG_enc_round,
	HG_enc_save,
	HG_enc_scale,

	HG_enc_scalefont,
	HG_enc_search,
	HG_enc_selectfont,
	HG_enc_setbbox,
	HG_enc_setcachedevice,
	HG_enc_setcachedevice2,
	HG_enc_setcharwidth,
	HG_enc_setcmykcolor,
	HG_enc_setdash,
	HG_enc_setfont,

	HG_enc_setgray,
	HG_enc_setgstate,
	HG_enc_sethsbcolor,
	HG_enc_setlinecap,
	HG_enc_setlinejoin,
	HG_enc_setlinewidth,
	HG_enc_setmatrix,
	HG_enc_setrgbcolor,
	HG_enc_setshared,
	HG_enc_shareddict,

	HG_enc_show,
	HG_enc_showpage,
	HG_enc_stop,
	HG_enc_stopped,
	HG_enc_store,
	HG_enc_string,
	HG_enc_stringwidth,
	HG_enc_stroke,
	HG_enc_strokepath,
	HG_enc_sub,

	HG_enc_systemdict,
	HG_enc_token,
	HG_enc_transform,
	HG_enc_translate,
	HG_enc_truncate,
	HG_enc_type,
	HG_enc_uappend,
	HG_enc_ucache,
	HG_enc_ueofill,
	HG_enc_ufill,

	HG_enc_undef,
	HG_enc_upath,
	HG_enc_userdict,
	HG_enc_ustroke,
	HG_enc_viewclip,
	HG_enc_viewclippath,
	HG_enc_where,
	HG_enc_widthshow,
	HG_enc_write,
	HG_enc_writehexstring,

	HG_enc_writeobject,
	HG_enc_writestring,
	HG_enc_wtranslation,
	HG_enc_xor,
	HG_enc_xshow,
	HG_enc_xyshow,
	HG_enc_yshow,
	HG_enc_FontDirectory,
	HG_enc_SharedFontDirectory,
	HG_enc_Courier,

	HG_enc_Courier_Bold,
	HG_enc_Courier_BoldOblique,
	HG_enc_Courier_Oblique,
	HG_enc_Helvetica,
	HG_enc_Helvetica_Bold,
	HG_enc_Helvetica_BoldOblique,
	HG_enc_Helvetica_Oblique,
	HG_enc_Symbol,
	HG_enc_Times_Bold,
	HG_enc_Times_BoldItalic,

	HG_enc_Times_Italic,
	HG_enc_Times_Roman,
	HG_enc_execuserobject,
	HG_enc_currentcolor,
	HG_enc_currentcolorspace,
	HG_enc_currentglobal,
	HG_enc_execform,
	HG_enc_filter,
	HG_enc_findresource,
	HG_enc_globaldict,

	HG_enc_makepattern,
	HG_enc_setcolor,
	HG_enc_setcolorspace,
	HG_enc_setglobal,
	HG_enc_setpagedevice,
	HG_enc_setpattern,

	HG_enc_sym_eq = 256,
	HG_enc_sym_eqeq,
	HG_enc_ISOLatin1Encoding,
	HG_enc_StandardEncoding,

	HG_enc_sym_left_square_bracket,
	HG_enc_sym_right_square_bracket,
	HG_enc_atan,
	HG_enc_banddevice,
	HG_enc_bytesavailable,
	HG_enc_cachestatus,
	HG_enc_closefile,
	HG_enc_colorimage,
	HG_enc_condition,
	HG_enc_copypage,

	HG_enc_cos,
	HG_enc_countdictstack,
	HG_enc_countexecstack,
	HG_enc_cshow,
	HG_enc_currentblackgeneration,
	HG_enc_currentcacheparams,
	HG_enc_currentcolorscreen,
	HG_enc_currentcolortransfer,
	HG_enc_currentcontext,
	HG_enc_currentflat,

	HG_enc_currenthalftone,
	HG_enc_currenthalftonephase,
	HG_enc_currentmiterlimit,
	HG_enc_currentobjectformat,
	HG_enc_currentpacking,
	HG_enc_currentscreen,
	HG_enc_currentstrokeadjust,
	HG_enc_currenttransfer,
	HG_enc_currentundercolorremoval,
	HG_enc_defaultmatrix,

	HG_enc_definefont,
	HG_enc_deletefile,
	HG_enc_detach,
	HG_enc_deviceinfo,
	HG_enc_dictstack,
	HG_enc_echo,
	HG_enc_erasepage,
	HG_enc_errordict,
	HG_enc_execstack,
	HG_enc_executeonly,

	HG_enc_exp,
	HG_enc_false,
	HG_enc_filenameforall,
	HG_enc_fileposition,
	HG_enc_fork,
	HG_enc_framedevice,
	HG_enc_grestoreall,
	HG_enc_handleerror,
	HG_enc_initclip,
	HG_enc_initgraphics,

	HG_enc_initmatrix,
	HG_enc_instroke,
	HG_enc_inustroke,
	HG_enc_join,
	HG_enc_kshow,
	HG_enc_ln,
	HG_enc_lock,
	HG_enc_log,
	HG_enc_mark,
	HG_enc_monitor,

	HG_enc_noaccess,
	HG_enc_notify,
	HG_enc_nulldevice,
	HG_enc_packedarray,
	HG_enc_quit,
	HG_enc_rand,
	HG_enc_rcheck,
	HG_enc_readonly,
	HG_enc_realtime,
	HG_enc_renamefile,

	HG_enc_renderbands,
	HG_enc_resetfile,
	HG_enc_reversepath,
	HG_enc_rootfont,
	HG_enc_rrand,
	HG_enc_run,
	HG_enc_scheck,
	HG_enc_setblackgeneration,
	HG_enc_setcachelimit,
	HG_enc_setcacheparams,

	HG_enc_setcolorscreen,
	HG_enc_setcolortransfer,
	HG_enc_setfileposition,
	HG_enc_setflat,
	HG_enc_sethalftone,
	HG_enc_sethalftonephase,
	HG_enc_setmiterlimit,
	HG_enc_setobjectformat,
	HG_enc_setpacking,
	HG_enc_setscreen,

	HG_enc_setstrokeadjust,
	HG_enc_settransfer,
	HG_enc_setucacheparams,
	HG_enc_setundercolorremoval,
	HG_enc_sin,
	HG_enc_sqrt,
	HG_enc_srand,
	HG_enc_stack,
	HG_enc_status,
	HG_enc_statusdict,

	HG_enc_true,
	HG_enc_ucachestatus,
	HG_enc_undefinefont,
	HG_enc_usertime,
	HG_enc_ustrokepath,
	HG_enc_version,
	HG_enc_vmreclaim,
	HG_enc_vmstatus,
	HG_enc_wait,
	HG_enc_wcheck,

	HG_enc_xcheck,
	HG_enc_yield,
	HG_enc_defineuserobject,
	HG_enc_undefineuserobject,
	HG_enc_UserObjects,
	HG_enc_cleardictstack,
	HG_enc_A,
	HG_enc_B,
	HG_enc_C,
	HG_enc_D,

	HG_enc_E,
	HG_enc_F,
	HG_enc_G,
	HG_enc_H,
	HG_enc_I,
	HG_enc_J,
	HG_enc_K,
	HG_enc_L,
	HG_enc_M,
	HG_enc_N,

	HG_enc_O,
	HG_enc_P,
	HG_enc_Q,
	HG_enc_R,
	HG_enc_S,
	HG_enc_T,
	HG_enc_U,
	HG_enc_V,
	HG_enc_W,
	HG_enc_X,

	HG_enc_Y,
	HG_enc_Z,
	HG_enc_a,
	HG_enc_b,
	HG_enc_c,
	HG_enc_d,
	HG_enc_e,
	HG_enc_f,
	HG_enc_g,
	HG_enc_h,

	HG_enc_i,
	HG_enc_j,
	HG_enc_k,
	HG_enc_l,
	HG_enc_m,
	HG_enc_n,
	HG_enc_o,
	HG_enc_p,
	HG_enc_q,
	HG_enc_r,

	HG_enc_s,
	HG_enc_t,
	HG_enc_u,
	HG_enc_v,
	HG_enc_w,
	HG_enc_x,
	HG_enc_y,
	HG_enc_z,
	HG_enc_setvmthreshold,
	HG_enc_sym_begin_dict_mark,

	HG_enc_sym_end_dict_mark,
	HG_enc_currentcolorrendering,
	HG_enc_currentdevparams,
	HG_enc_currentoverprint,
	HG_enc_currentpagedevice,
	HG_enc_currentsystemparams,
	HG_enc_currentuserparams,
	HG_enc_defineresource,
	HG_enc_findencoding,
	HG_enc_gcheck,

	HG_enc_glyphshow,
	HG_enc_languagelevel,
	HG_enc_product,
	HG_enc_pstack,
	HG_enc_resourceforall,
	HG_enc_resourcestatus,
	HG_enc_revision,
	HG_enc_serialnumber,
	HG_enc_setcolorrendering,
	HG_enc_setdevparams,

	HG_enc_setoverprint,
	HG_enc_setsystemparams,
	HG_enc_setuserparams,
	HG_enc_startjob,
	HG_enc_undefineresource,
	HG_enc_GlobalFontDirectory,
	HG_enc_ASCII85Decode,
	HG_enc_ASCII85Encode,
	HG_enc_ASCIIHexDecode,
	HG_enc_ASCIIHexEncode,

	HG_enc_CCITTFaxDecode,
	HG_enc_CCITTFaxEncode,
	HG_enc_DCTDecode,
	HG_enc_DCTEncode,
	HG_enc_LZWDecode,
	HG_enc_LZWEncode,
	HG_enc_NullEncode,
	HG_enc_RunLengthDecode,
	HG_enc_RunLengthEncode,
	HG_enc_SubFileDecode,

	HG_enc_CIEBasedA,
	HG_enc_CIEBasedABC,
	HG_enc_DeviceCMYK,
	HG_enc_DeviceGray,
	HG_enc_DeviceRGB,
	HG_enc_Indexed,
	HG_enc_Pattern,
	HG_enc_Separation,
	HG_enc_CIEBasedDEF,
	HG_enc_CIEBasedDEFG,

	HG_enc_DeviceN,

	HG_enc_POSTSCRIPT_RESERVED_END,

	HG_enc_protected_arraytomark,
	HG_enc_protected_repeat_continue,
	HG_enc_protected_stopped_continue,

	HG_enc_private_forceput,
	HG_enc_private_odef,
	HG_enc_private_setglobal,
	HG_enc_private_undef,

	HG_enc_END
};


G_END_DECLS

#endif /* __HIEROGLYPH_HGTYPES_H__ */
