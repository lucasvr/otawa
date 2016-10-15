/*
 *	$Id$
 *	File class interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2006-8, IRIT UPS.
 *
 *	OTAWA is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	OTAWA is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OTAWA; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef OTAWA_PROG_FILE_H
#define OTAWA_PROG_FILE_H

#include <elm/inhstruct/DLList.h>
#include <otawa/instruction.h>
#include <otawa/prog/Symbol.h>
#include <otawa/prog/Segment.h>

#include <elm/genstruct/Vector.h>
#include <elm/genstruct/HashTable.h>

namespace otawa {

// Defined classes
class ProgItem;

// File class
class File: public PropList {
	String _name;
	genstruct::Vector<Segment *> segs;
	typedef genstruct::HashTable<String, Symbol *> syms_t;
	syms_t syms;


public:
	inline File(String name): _name(name) { }
	inline CString name(void) { return _name.toCString(); }
	Inst *findInstAt(address_t address);
	ProgItem *findItemAt(address_t address);

	// Segment management
	inline void addSegment(Segment *seg) { segs.add(seg); }
	Segment *findSegmentAt(Address addr);
	class SegIter: public genstruct::Vector<Segment *>::Iterator {
	public:
		inline SegIter(const File *file): genstruct::Vector<Segment *>::Iterator(file->segs) { }
		inline SegIter(const SegIter& iter): genstruct::Vector<Segment *>::Iterator(iter) { }
	};

	// Symbol management
	inline void addSymbol(Symbol *sym) { syms.put(sym->name(), sym); }
	inline void removeSymbol(Symbol *sym) { syms.remove(sym->name()); }
	address_t findLabel(const String& label);
	Symbol *findSymbol(String name);
	class SymIter: public syms_t::Iterator {
	public:
		inline SymIter(const File *file): syms_t::Iterator(file->syms) { }
		inline SymIter(const SymIter& iter): syms_t::Iterator(iter) { }
	};

	// Deprecated
	inline Inst *findByAddress(address_t address) { return findInstAt(address); }

protected:
	friend class Process;
	~File(void);
};

}	// otawa

#endif // OTAWA_PROG_FILE_H
