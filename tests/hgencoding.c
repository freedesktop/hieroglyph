/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * hgencoding.c
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

#include <string.h>
#include "main.h"
#include "hgencoding.h"


/** common **/
void
setup(void)
{
}

void
teardown(void)
{
	gchar *e = hieroglyph_test_pop_error();

	if (e) {
		g_print("E: %s\n", e);
		g_free(e);
	}
}

/** test cases **/
TDEF (hg_encoding_init)
{
	fail_unless(hg_encoding_init(), "Unable to initialize the encodings");
	fail_unless(hg_encoding_lookup_system_encoding("abs") != HG_enc_END, "the encoding map isn't somehow initialized properly");
	hg_encoding_tini();
	fail_unless(hg_encoding_lookup_system_encoding("abs") == HG_enc_END, "the encoding map isn't somehow de-initialized properly");
	g_free(hieroglyph_test_pop_error());
} TEND

TDEF (hg_encoding_tini)
{
	/* can be done in hg_encoding_init testcase */
} TEND

TDEF (hg_encoding_get_system_encoding_name)
{
	const gchar *p;

#define TT(_n_)								\
	p = hg_encoding_get_system_encoding_name(HG_enc_ ## _n_);	\
	fail_unless(p != NULL, "Unable to obtain the system encoding name (" #_n_ ")");	\
	fail_unless(strcmp(p, #_n_) == 0, "Obtaining the wrong name for " #_n_ ")");

	TT (abs);
	TT (add);
	TT (aload);
	TT (anchorsearch);
	TT (and);
	TT (arc);
	TT (arcn);
	TT (arct);
	TT (arcto);
	TT (array);

	TT (ashow);
	TT (astore);
	TT (awidthshow);
	TT (begin);
	TT (bind);
	TT (bitshift);
	TT (ceiling);
	TT (charpath);
	TT (clear);
	TT (cleartomark);

	TT (clip);
	TT (clippath);
	TT (closepath);
	TT (concat);
	TT (concatmatrix);
	TT (copy);
	TT (count);
	TT (counttomark);
	TT (currentcmykcolor);
	TT (currentdash);

	TT (currentdict);
	TT (currentfile);
	TT (currentfont);
	TT (currentgray);
	TT (currentgstate);
	TT (currenthsbcolor);
	TT (currentlinecap);
	TT (currentlinejoin);
	TT (currentlinewidth);
	TT (currentmatrix);

	TT (currentpoint);
	TT (currentrgbcolor);
	TT (currentshared);
	TT (curveto);
	TT (cvi);
	TT (cvlit);
	TT (cvn);
	TT (cvr);
	TT (cvrs);
	TT (cvs);

	TT (cvx);
	TT (def);
	TT (defineusername);
	TT (dict);
	TT (div);
	TT (dtransform);
	TT (dup);
	TT (end);
	TT (eoclip);
	TT (eofill);

	TT (eoviewclip);
	TT (eq);
	TT (exch);
	TT (exec);
	TT (exit);
	TT (file);
	TT (fill);
	TT (findfont);
	TT (flattenpath);
	TT (floor);

	TT (flush);
	TT (flushfile);
	TT (for);
	TT (forall);
	TT (ge);
	TT (get);
	TT (getinterval);
	TT (grestore);
	TT (gsave);
	TT (gstate);

	TT (gt);
	TT (identmatrix);
	TT (idiv);
	TT (idtransform);
	TT (if);
	TT (ifelse);
	TT (image);
	TT (imagemask);
	TT (index);
	TT (ineofill);

	TT (infill);
	TT (initviewclip);
	TT (inueofill);
	TT (inufill);
	TT (invertmatrix);
	TT (itransform);
	TT (known);
	TT (le);
	TT (length);
	TT (lineto);

	TT (load);
	TT (loop);
	TT (lt);
	TT (makefont);
	TT (matrix);
	TT (maxlength);
	TT (mod);
	TT (moveto);
	TT (mul);
	TT (ne);

	TT (neg);
	TT (newpath);
	TT (not);
	TT (null);
	TT (or);
	TT (pathbbox);
	TT (pathforall);
	TT (pop);
	TT (print);
	TT (printobject);

	TT (put);
	TT (putinterval);
	TT (rcurveto);
	TT (read);
	TT (readhexstring);
	TT (readline);
	TT (readstring);
	TT (rectclip);
	TT (rectfill);
	TT (rectstroke);

	TT (rectviewclip);
	TT (repeat);
	TT (restore);
	TT (rlineto);
	TT (rmoveto);
	TT (roll);
	TT (rotate);
	TT (round);
	TT (save);
	TT (scale);

	TT (scalefont);
	TT (search);
	TT (selectfont);
	TT (setbbox);
	TT (setcachedevice);
	TT (setcachedevice2);
	TT (setcharwidth);
	TT (setcmykcolor);
	TT (setdash);
	TT (setfont);

	TT (setgray);
	TT (setgstate);
	TT (sethsbcolor);
	TT (setlinecap);
	TT (setlinejoin);
	TT (setlinewidth);
	TT (setmatrix);
	TT (setrgbcolor);
	TT (setshared);
	TT (shareddict);

	TT (show);
	TT (showpage);
	TT (stop);
	TT (stopped);
	TT (store);
	TT (string);
	TT (stringwidth);
	TT (stroke);
	TT (strokepath);
	TT (sub);

	TT (systemdict);
	TT (token);
	TT (transform);
	TT (translate);
	TT (truncate);
	TT (type);
	TT (uappend);
	TT (ucache);
	TT (ueofill);
	TT (ufill);

	TT (undef);
	TT (upath);
	TT (userdict);
	TT (ustroke);
	TT (viewclip);
	TT (viewclippath);
	TT (where);
	TT (widthshow);
	TT (write);
	TT (writehexstring);

	TT (writeobject);
	TT (writestring);
	TT (wtranslation);
	TT (xor);
	TT (xshow);
	TT (xyshow);
	TT (yshow);
	TT (FontDirectory);
	TT (SharedFontDirectory);
	TT (Courier);

	TT (Courier_Bold);
	TT (Courier_BoldOblique);
	TT (Courier_Oblique);
	TT (Helvetica);
	TT (Helvetica_Bold);
	TT (Helvetica_BoldOblique);
	TT (Helvetica_Oblique);
	TT (Symbol);
	TT (Times_Bold);
	TT (Times_BoldItalic);

	TT (Times_Italic);
	TT (Times_Roman);
	TT (execuserobject);
	TT (currentcolor);
	TT (currentcolorspace);
	TT (currentglobal);
	TT (execform);
	TT (filter);
	TT (findresource);
	TT (globaldict);

	TT (makepattern);
	TT (setcolor);
	TT (setcolorspace);
	TT (setglobal);
	TT (setpagedevice);
	TT (setpattern);

	TT (sym_eq);
	TT (sym_eqeq);
	TT (ISOLatin1Encoding);
	TT (StandardEncoding);

	TT (sym_left_square_bracket);
	TT (sym_right_square_bracket);
	TT (atan);
	TT (banddevice);
	TT (bytesavailable);
	TT (cachestatus);
	TT (closefile);
	TT (colorimage);
	TT (condition);
	TT (copypage);

	TT (cos);
	TT (countdictstack);
	TT (countexecstack);
	TT (cshow);
	TT (currentblackgeneration);
	TT (currentcacheparams);
	TT (currentcolorscreen);
	TT (currentcolortransfer);
	TT (currentcontext);
	TT (currentflat);

	TT (currenthalftone);
	TT (currenthalftonephase);
	TT (currentmiterlimit);
	TT (currentobjectformat);
	TT (currentpacking);
	TT (currentscreen);
	TT (currentstrokeadjust);
	TT (currenttransfer);
	TT (currentundercolorremoval);
	TT (defaultmatrix);

	TT (definefont);
	TT (deletefile);
	TT (detach);
	TT (deviceinfo);
	TT (dictstack);
	TT (echo);
	TT (erasepage);
	TT (errordict);
	TT (execstack);
	TT (executeonly);

	TT (exp);
	TT (false);
	TT (filenameforall);
	TT (fileposition);
	TT (fork);
	TT (framedevice);
	TT (grestoreall);
	TT (handleerror);
	TT (initclip);
	TT (initgraphics);

	TT (initmatrix);
	TT (instroke);
	TT (inustroke);
	TT (join);
	TT (kshow);
	TT (ln);
	TT (lock);
	TT (log);
	TT (mark);
	TT (monitor);

	TT (noaccess);
	TT (notify);
	TT (nulldevice);
	TT (packedarray);
	TT (quit);
	TT (rand);
	TT (rcheck);
	TT (readonly);
	TT (realtime);
	TT (renamefile);

	TT (renderbands);
	TT (resetfile);
	TT (reversepath);
	TT (rootfont);
	TT (rrand);
	TT (run);
	TT (scheck);
	TT (setblackgeneration);
	TT (setcachelimit);
	TT (setcacheparams);

	TT (setcolorscreen);
	TT (setcolortransfer);
	TT (setfileposition);
	TT (setflat);
	TT (sethalftone);
	TT (sethalftonephase);
	TT (setmiterlimit);
	TT (setobjectformat);
	TT (setpacking);
	TT (setscreen);

	TT (setstrokeadjust);
	TT (settransfer);
	TT (setucacheparams);
	TT (setundercolorremoval);
	TT (sin);
	TT (sqrt);
	TT (srand);
	TT (stack);
	TT (status);
	TT (statusdict);

	TT (true);
	TT (ucachestatus);
	TT (undefinefont);
	TT (usertime);
	TT (ustrokepath);
	TT (version);
	TT (vmreclaim);
	TT (vmstatus);
	TT (wait);
	TT (wcheck);

	TT (xcheck);
	TT (yield);
	TT (defineuserobject);
	TT (undefineuserobject);
	TT (UserObjects);
	TT (cleardictstack);
	TT (A);
	TT (B);
	TT (C);
	TT (D);

	TT (E);
	TT (F);
	TT (G);
	TT (H);
	TT (I);
	TT (J);
	TT (K);
	TT (L);
	TT (M);
	TT (N);

	TT (O);
	TT (P);
	TT (Q);
	TT (R);
	TT (S);
	TT (T);
	TT (U);
	TT (V);
	TT (W);
	TT (X);

	TT (Y);
	TT (Z);
	TT (a);
	TT (b);
	TT (c);
	TT (d);
	TT (e);
	TT (f);
	TT (g);
	TT (h);

	TT (i);
	TT (j);
	TT (k);
	TT (l);
	TT (m);
	TT (n);
	TT (o);
	TT (p);
	TT (q);
	TT (r);

	TT (s);
	TT (t);
	TT (u);
	TT (v);
	TT (w);
	TT (x);
	TT (y);
	TT (z);
	TT (setvmthreshold);
	TT (sym_begin_dict_mark);

	TT (sym_end_dict_mark);
	TT (currentcolorrendering);
	TT (currentdevparams);
	TT (currentoverprint);
	TT (currentpagedevice);
	TT (currentsystemparams);
	TT (currentuserparams);
	TT (defineresource);
	TT (findencoding);
	TT (gcheck);

	TT (glyphshow);
	TT (languagelevel);
	TT (product);
	TT (pstack);
	TT (resourceforall);
	TT (resourcestatus);
	TT (revision);
	TT (serialnumber);
	TT (setcolorrendering);
	TT (setdevparams);

	TT (setoverprint);
	TT (setsystemparams);
	TT (setuserparams);
	TT (startjob);
	TT (undefineresource);
	TT (GlobalFontDirectory);
	TT (ASCII85Decode);
	TT (ASCII85Encode);
	TT (ASCIIHexDecode);
	TT (ASCIIHexEncode);

	TT (CCITTFaxDecode);
	TT (CCITTFaxEncode);
	TT (DCTDecode);
	TT (DCTEncode);
	TT (LZWDecode);
	TT (LZWEncode);
	TT (NullEncode);
	TT (RunLengthDecode);
	TT (RunLengthEncode);
	TT (SubFileDecode);

	TT (CIEBasedA);
	TT (CIEBasedABC);
	TT (DeviceCMYK);
	TT (DeviceGray);
	TT (DeviceRGB);
	TT (Indexed);
	TT (Pattern);
	TT (Separation);
	TT (CIEBasedDEF);
	TT (CIEBasedDEFG);

	TT (DeviceN);

#undef TT

	p = hg_encoding_get_system_encoding_name(255);
	fail_unless(p != NULL, "Unable to obtain the system encoding name (unknown but in the range)");
	fail_unless(*p == '\0', "Obtaining non-null strings for unknown");
	p = hg_encoding_get_system_encoding_name(HG_enc_END);
	fail_unless(p == NULL, "Obtaining something wrong for encoding number out of range");
	g_free(hieroglyph_test_pop_error());
} TEND

TDEF (hg_encoding_lookup_system_encoding)
{
	hg_encoding_init();

#define TT(_n_)								\
	fail_unless(hg_encoding_lookup_system_encoding(#_n_) == HG_enc_ ## _n_, "Unexpected encoding type for " #_n_)

	TT (abs);
	TT (add);
	TT (aload);
	TT (anchorsearch);
	TT (and);
	TT (arc);
	TT (arcn);
	TT (arct);
	TT (arcto);
	TT (array);

	TT (ashow);
	TT (astore);
	TT (awidthshow);
	TT (begin);
	TT (bind);
	TT (bitshift);
	TT (ceiling);
	TT (charpath);
	TT (clear);
	TT (cleartomark);

	TT (clip);
	TT (clippath);
	TT (closepath);
	TT (concat);
	TT (concatmatrix);
	TT (copy);
	TT (count);
	TT (counttomark);
	TT (currentcmykcolor);
	TT (currentdash);

	TT (currentdict);
	TT (currentfile);
	TT (currentfont);
	TT (currentgray);
	TT (currentgstate);
	TT (currenthsbcolor);
	TT (currentlinecap);
	TT (currentlinejoin);
	TT (currentlinewidth);
	TT (currentmatrix);

	TT (currentpoint);
	TT (currentrgbcolor);
	TT (currentshared);
	TT (curveto);
	TT (cvi);
	TT (cvlit);
	TT (cvn);
	TT (cvr);
	TT (cvrs);
	TT (cvs);

	TT (cvx);
	TT (def);
	TT (defineusername);
	TT (dict);
	TT (div);
	TT (dtransform);
	TT (dup);
	TT (end);
	TT (eoclip);
	TT (eofill);

	TT (eoviewclip);
	TT (eq);
	TT (exch);
	TT (exec);
	TT (exit);
	TT (file);
	TT (fill);
	TT (findfont);
	TT (flattenpath);
	TT (floor);

	TT (flush);
	TT (flushfile);
	TT (for);
	TT (forall);
	TT (ge);
	TT (get);
	TT (getinterval);
	TT (grestore);
	TT (gsave);
	TT (gstate);

	TT (gt);
	TT (identmatrix);
	TT (idiv);
	TT (idtransform);
	TT (if);
	TT (ifelse);
	TT (image);
	TT (imagemask);
	TT (index);
	TT (ineofill);

	TT (infill);
	TT (initviewclip);
	TT (inueofill);
	TT (inufill);
	TT (invertmatrix);
	TT (itransform);
	TT (known);
	TT (le);
	TT (length);
	TT (lineto);

	TT (load);
	TT (loop);
	TT (lt);
	TT (makefont);
	TT (matrix);
	TT (maxlength);
	TT (mod);
	TT (moveto);
	TT (mul);
	TT (ne);

	TT (neg);
	TT (newpath);
	TT (not);
	TT (null);
	TT (or);
	TT (pathbbox);
	TT (pathforall);
	TT (pop);
	TT (print);
	TT (printobject);

	TT (put);
	TT (putinterval);
	TT (rcurveto);
	TT (read);
	TT (readhexstring);
	TT (readline);
	TT (readstring);
	TT (rectclip);
	TT (rectfill);
	TT (rectstroke);

	TT (rectviewclip);
	TT (repeat);
	TT (restore);
	TT (rlineto);
	TT (rmoveto);
	TT (roll);
	TT (rotate);
	TT (round);
	TT (save);
	TT (scale);

	TT (scalefont);
	TT (search);
	TT (selectfont);
	TT (setbbox);
	TT (setcachedevice);
	TT (setcachedevice2);
	TT (setcharwidth);
	TT (setcmykcolor);
	TT (setdash);
	TT (setfont);

	TT (setgray);
	TT (setgstate);
	TT (sethsbcolor);
	TT (setlinecap);
	TT (setlinejoin);
	TT (setlinewidth);
	TT (setmatrix);
	TT (setrgbcolor);
	TT (setshared);
	TT (shareddict);

	TT (show);
	TT (showpage);
	TT (stop);
	TT (stopped);
	TT (store);
	TT (string);
	TT (stringwidth);
	TT (stroke);
	TT (strokepath);
	TT (sub);

	TT (systemdict);
	TT (token);
	TT (transform);
	TT (translate);
	TT (truncate);
	TT (type);
	TT (uappend);
	TT (ucache);
	TT (ueofill);
	TT (ufill);

	TT (undef);
	TT (upath);
	TT (userdict);
	TT (ustroke);
	TT (viewclip);
	TT (viewclippath);
	TT (where);
	TT (widthshow);
	TT (write);
	TT (writehexstring);

	TT (writeobject);
	TT (writestring);
	TT (wtranslation);
	TT (xor);
	TT (xshow);
	TT (xyshow);
	TT (yshow);
	TT (FontDirectory);
	TT (SharedFontDirectory);
	TT (Courier);

	TT (Courier_Bold);
	TT (Courier_BoldOblique);
	TT (Courier_Oblique);
	TT (Helvetica);
	TT (Helvetica_Bold);
	TT (Helvetica_BoldOblique);
	TT (Helvetica_Oblique);
	TT (Symbol);
	TT (Times_Bold);
	TT (Times_BoldItalic);

	TT (Times_Italic);
	TT (Times_Roman);
	TT (execuserobject);
	TT (currentcolor);
	TT (currentcolorspace);
	TT (currentglobal);
	TT (execform);
	TT (filter);
	TT (findresource);
	TT (globaldict);

	TT (makepattern);
	TT (setcolor);
	TT (setcolorspace);
	TT (setglobal);
	TT (setpagedevice);
	TT (setpattern);

	TT (sym_eq);
	TT (sym_eqeq);
	TT (ISOLatin1Encoding);
	TT (StandardEncoding);

	TT (sym_left_square_bracket);
	TT (sym_right_square_bracket);
	TT (atan);
	TT (banddevice);
	TT (bytesavailable);
	TT (cachestatus);
	TT (closefile);
	TT (colorimage);
	TT (condition);
	TT (copypage);

	TT (cos);
	TT (countdictstack);
	TT (countexecstack);
	TT (cshow);
	TT (currentblackgeneration);
	TT (currentcacheparams);
	TT (currentcolorscreen);
	TT (currentcolortransfer);
	TT (currentcontext);
	TT (currentflat);

	TT (currenthalftone);
	TT (currenthalftonephase);
	TT (currentmiterlimit);
	TT (currentobjectformat);
	TT (currentpacking);
	TT (currentscreen);
	TT (currentstrokeadjust);
	TT (currenttransfer);
	TT (currentundercolorremoval);
	TT (defaultmatrix);

	TT (definefont);
	TT (deletefile);
	TT (detach);
	TT (deviceinfo);
	TT (dictstack);
	TT (echo);
	TT (erasepage);
	TT (errordict);
	TT (execstack);
	TT (executeonly);

	TT (exp);
	TT (false);
	TT (filenameforall);
	TT (fileposition);
	TT (fork);
	TT (framedevice);
	TT (grestoreall);
	TT (handleerror);
	TT (initclip);
	TT (initgraphics);

	TT (initmatrix);
	TT (instroke);
	TT (inustroke);
	TT (join);
	TT (kshow);
	TT (ln);
	TT (lock);
	TT (log);
	TT (mark);
	TT (monitor);

	TT (noaccess);
	TT (notify);
	TT (nulldevice);
	TT (packedarray);
	TT (quit);
	TT (rand);
	TT (rcheck);
	TT (readonly);
	TT (realtime);
	TT (renamefile);

	TT (renderbands);
	TT (resetfile);
	TT (reversepath);
	TT (rootfont);
	TT (rrand);
	TT (run);
	TT (scheck);
	TT (setblackgeneration);
	TT (setcachelimit);
	TT (setcacheparams);

	TT (setcolorscreen);
	TT (setcolortransfer);
	TT (setfileposition);
	TT (setflat);
	TT (sethalftone);
	TT (sethalftonephase);
	TT (setmiterlimit);
	TT (setobjectformat);
	TT (setpacking);
	TT (setscreen);

	TT (setstrokeadjust);
	TT (settransfer);
	TT (setucacheparams);
	TT (setundercolorremoval);
	TT (sin);
	TT (sqrt);
	TT (srand);
	TT (stack);
	TT (status);
	TT (statusdict);

	TT (true);
	TT (ucachestatus);
	TT (undefinefont);
	TT (usertime);
	TT (ustrokepath);
	TT (version);
	TT (vmreclaim);
	TT (vmstatus);
	TT (wait);
	TT (wcheck);

	TT (xcheck);
	TT (yield);
	TT (defineuserobject);
	TT (undefineuserobject);
	TT (UserObjects);
	TT (cleardictstack);
	TT (A);
	TT (B);
	TT (C);
	TT (D);

	TT (E);
	TT (F);
	TT (G);
	TT (H);
	TT (I);
	TT (J);
	TT (K);
	TT (L);
	TT (M);
	TT (N);

	TT (O);
	TT (P);
	TT (Q);
	TT (R);
	TT (S);
	TT (T);
	TT (U);
	TT (V);
	TT (W);
	TT (X);

	TT (Y);
	TT (Z);
	TT (a);
	TT (b);
	TT (c);
	TT (d);
	TT (e);
	TT (f);
	TT (g);
	TT (h);

	TT (i);
	TT (j);
	TT (k);
	TT (l);
	TT (m);
	TT (n);
	TT (o);
	TT (p);
	TT (q);
	TT (r);

	TT (s);
	TT (t);
	TT (u);
	TT (v);
	TT (w);
	TT (x);
	TT (y);
	TT (z);
	TT (setvmthreshold);
	TT (sym_begin_dict_mark);

	TT (sym_end_dict_mark);
	TT (currentcolorrendering);
	TT (currentdevparams);
	TT (currentoverprint);
	TT (currentpagedevice);
	TT (currentsystemparams);
	TT (currentuserparams);
	TT (defineresource);
	TT (findencoding);
	TT (gcheck);

	TT (glyphshow);
	TT (languagelevel);
	TT (product);
	TT (pstack);
	TT (resourceforall);
	TT (resourcestatus);
	TT (revision);
	TT (serialnumber);
	TT (setcolorrendering);
	TT (setdevparams);

	TT (setoverprint);
	TT (setsystemparams);
	TT (setuserparams);
	TT (startjob);
	TT (undefineresource);
	TT (GlobalFontDirectory);
	TT (ASCII85Decode);
	TT (ASCII85Encode);
	TT (ASCIIHexDecode);
	TT (ASCIIHexEncode);

	TT (CCITTFaxDecode);
	TT (CCITTFaxEncode);
	TT (DCTDecode);
	TT (DCTEncode);
	TT (LZWDecode);
	TT (LZWEncode);
	TT (NullEncode);
	TT (RunLengthDecode);
	TT (RunLengthEncode);
	TT (SubFileDecode);

	TT (CIEBasedA);
	TT (CIEBasedABC);
	TT (DeviceCMYK);
	TT (DeviceGray);
	TT (DeviceRGB);
	TT (Indexed);
	TT (Pattern);
	TT (Separation);
	TT (CIEBasedDEF);
	TT (CIEBasedDEFG);

	TT (DeviceN);

#undef TT

	hg_encoding_tini();
} TEND

/****/
Suite *
hieroglyph_suite(void)
{
	Suite *s = suite_create("hg_system_encoding_t");
	TCase *tc = tcase_create("Generic Functionalities");

	tcase_add_checked_fixture(tc, setup, teardown);

	T (hg_encoding_init);
	T (hg_encoding_tini);
	T (hg_encoding_get_system_encoding_name);
	T (hg_encoding_lookup_system_encoding);

	suite_add_tcase(s, tc);

	return s;
}
