# .gdbinit
# Copyright (C) 2006 Akira TAGOH

# Authors:
#   Akira TAGOH  <at@gclab.org>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

define _hggetmemobj
  set $_obj = (HgMemObject *)($arg0)
  if ($_obj->id != 0x48474d4f)
    set $_obj = (HgMemObject *)((gsize)($arg0) - sizeof (HgMemObject))
    if ($_obj->id != 0x48474d4f)
      set $_obj = 0
    end
  end
end

define _hggethgobj
  _hggetmemobj $arg0
  set $_hobj = 0
  if ($_obj != 0)
    set $_hobj = (HgObject *)$_obj->data
    if ($_hobj->id != 0x48474f4f)
      set $_hobj = 0
    end
  end
end

define _hgmeminfo
  dont-repeat
  _hggetmemobj $arg0
  if ($_obj == 0)
    printf "Invalid object %p\n", $arg0
  else
    printf "(HgMemObject *)%p - [subid: %p] ", $_obj, $_obj->subid
    printf "[heap id: %d] ", $_obj->heap_id
    printf "[pool: %p] ", $_obj->pool
    printf "[block_size: %u] ", $_obj->block_size
    set $f = $_obj->flags
    set $o = 0
    printf "[flags: "
    if (($f & 1) == 1)
      printf "MARK"
      set $o = 1
    end
    if (($f & 2) == 2)
      if $o != 0
	printf "|"
      end
      printf "RESTORABLE"
      set $o = 1
    end
    if (($f & 4) == 4)
      if $o != 0
	printf "|"
      end
      printf "COMPLEX"
      set $o = 1
    end
    if (($f & 8) == 8)
      if $o != 0
	printf "|"
      end
      printf "LOCK"
      set $o = 1
    end
    if (($f & 16) == 16)
      if $o != 0
	printf "|"
      end
      printf "COPYING"
      set $o = 1
    end
    if (($f & 32) == 32)
      if $o != 0
	printf "|"
      end
      printf "COPIED"
      set $o = 1
    end
    printf "]\n"
  end
end

define _hgobjinfo
  dont-repeat
  _hggethgobj $arg0
  if ($_hobj == 0)
    printf "Invalid object %p\n", $arg0
  else
    printf "(HgObject *)%p - ", $_hobj
    set $f = $_hobj->state
    set $o = 0
    printf "[state: "
    if (($f & 1) == 1)
      printf "READABLE"
      set $o = 1
    end
    if (($f & 2) == 2)
      if $o != 0
	printf "|"
      end
      printf "WRITABLE"
      set $o = 1
    end
    if (($f & 4) == 4)
      if $o != 0
	printf "|"
      end
      printf "EXECUTABLE"
      set $o = 1
    end
    printf "]\n"
    printf "  vtable[%p]:\n", $_hobj->vtable
    if ($_hobj->vtable)
      printf "    free: "
      output $_hobj->vtable->free
      printf "\n"
      printf "    set_flags: "
      output $_hobj->vtable->set_flags
      printf "\n"
      printf "    relocate: "
      output $_hobj->vtable->relocate
      printf "\n"
      printf "    dup: "
      output $_hobj->vtable->dup
      printf "\n"
      printf "    copy: "
      output $_hobj->vtable->copy
      printf "\n"
      printf "    to_string: "
      output $_hobj->vtable->to_string
      printf "\n"
    end
  end
end

define hgnodeprint
  dont-repeat
  _hggethgobj $arg0
  if ($_hobj == 0)
    printf "%p isn't an valid object managed by hieroglyph.\n", $arg0
  else
    _hgmeminfo $arg0
    _hgobjinfo $arg0
    set $_node = (HgValueNode *)$_hobj
    printf "(HgValueNode *)%p - [type: ", $_node
    if ($_node->type == HG_TYPE_VALUE_BOOLEAN)
      printf "BOOLEAN] [%d]\n", $_node->v.boolean
    end
    if ($_node->type == HG_TYPE_VALUE_INTEGER)
      printf "INTEGER] [%d]\n", $_node->v.integer
    end
    if ($_node->type == HG_TYPE_VALUE_REAL)
      printf "REAL] [%f]\n", $_node->v.real
    end
    if ($_node->type == HG_TYPE_VALUE_NAME)
      printf "NAME] [%s]\n", (char *)$_node->v.pointer
    end
    if ($_node->type == HG_TYPE_VALUE_ARRAY)
      printf "ARRAY]\n"
      print (HgArray *)$_node->v.pointer
    end
    if ($_node->type == HG_TYPE_VALUE_STRING)
      printf "STRING]\n"
      print (HgString *)$_node->v.pointer
    end
    if ($_node->type == HG_TYPE_VALUE_DICT)
      printf "DICT]\n"
      print (HgDict *)$_node->v.pointer
    end
    if ($_node->type == HG_TYPE_VALUE_NULL)
      printf "NULL]\n"
    end
    if ($_node->type == HG_TYPE_VALUE_POINTER)
      printf "OPERATOR]\n"
      print (LibrettoOperator *)$_node->v.pointer
    end
    if ($_node->type == HG_TYPE_VALUE_MARK)
      printf "MARK]\n"
    end
    if ($_node->type == HG_TYPE_VALUE_FILE)
      printf "FILE]\n"
      print (HgFileObject *)$_node->v.pointer
    end
    if ($_node->type == HG_TYPE_VALUE_SNAPSHOT)
      printf "SNAPSHOT]\n"
      print (HgMemSnapshot *)$_node->v.pointer
    end
  end
end
document hgnodeprint
Print the value of the node given as argument.
end

define hgarrayprint
  dont-repeat
  _hggethgobj $arg0
  if ($_hobj == 0)
    printf "%p isn't a valid object managed by hieroglyph.\n", $arg0
  else
    _hgmeminfo $arg0
    _hgobjinfo $arg0
    set $_array = (HgArray *)$_hobj
    printf "(HgArray *)%p - [%d/%d]\n", $_array, $_array->n_arrays, $_array->allocated_arrays
    set $_i = 0
    while ($_i < $_array->n_arrays)
      set $_p = $_array->current[$_i]
      printf "[%d]: ", $_i
      hgnodeprint $_array->current[$_i]
      set $_i++
    end
  end
end

define hgdictprint
  dont-repeat
end

define hgstringprint
  dont-repeat
end

define hgoperprint
  dont-repeat
end

define hgfileprint
  dont-repeat
end

define hgsnapprint
  dont-repeat
end
