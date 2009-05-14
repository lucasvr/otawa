/*
 *	$Id $
 *	Inst class interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2003-08, IRIT UPS.
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
#ifndef OTAWA_INST_H
#define OTAWA_INST_H

#include <elm/string.h>
#include <elm/io.h>
#include <elm/genstruct/Table.h>
#include <otawa/prog/ProgItem.h>
#include <otawa/properties.h>

namespace otawa {

// Declaration
class Inst;
class PseudoInst;
namespace hard {
	class Register;
} // hard

// Inst class
class Inst: public ProgItem {
	friend class CodeItem;
protected:
	static const elm::genstruct::Table<hard::Register *> no_regs;
	virtual ~Inst(void) { };
public:

	// Kind management
	typedef unsigned long kind_t;
	static const kind_t IS_COND		= 0x00001;
	static const kind_t IS_CONTROL	= 0x00002;
	static const kind_t IS_CALL		= 0x00004;
	static const kind_t IS_RETURN	= 0x00008;
	static const kind_t IS_MEM		= 0x00010;
	static const kind_t IS_LOAD		= 0x00020;
	static const kind_t IS_STORE	= 0x00040;
	static const kind_t IS_INT		= 0x00080;
	static const kind_t IS_FLOAT	= 0x00100;
	static const kind_t IS_ALU		= 0x00200;
	static const kind_t IS_MUL		= 0x00400;
	static const kind_t IS_DIV		= 0x00800;
	static const kind_t IS_SHIFT	= 0x01000;
	static const kind_t IS_TRAP		= 0x02000;
	static const kind_t IS_INTERN	= 0x04000;
	static const kind_t IS_MULTI 	= 0x08000;
	static const kind_t IS_SPECIAL 	= 0x10000;

	// null instruction
	static Inst& null;

	// Accessors
	inline Inst *nextInst(void) const
		{ ProgItem *item = next(); if(!item) return 0; else return item->toInst(); }
	inline Inst *prevInst(void) const
		{ ProgItem *item = previous(); if(!item) return 0; else return item->toInst(); }
	virtual void dump(io::Output& out);

	// Kind access
	virtual kind_t kind(void) = 0;
	inline bool meets(kind_t mask) { return (kind() & mask) == mask; }
	inline bool oneOf(kind_t mask) { return kind() & mask; }
	inline bool noneOf(kind_t mask) { return !oneOf(mask); }

	inline bool isIntern(void) { return oneOf(IS_INTERN); }
	inline bool isMem(void) { return oneOf(IS_MEM); }
	inline bool isControl(void) { return oneOf(IS_CONTROL); }
	inline bool isLoad(void) { return oneOf(IS_LOAD); }
	inline bool isStore(void) { return oneOf(IS_STORE); }
	inline bool isBranch(void)
		{ return oneOf(IS_CONTROL) && noneOf(IS_RETURN | IS_CALL | IS_TRAP); }
	inline bool isCall(void) { return oneOf(IS_CALL); }
	inline bool isReturn(void) { return oneOf(IS_RETURN); }
	inline bool isConditional(void) { return oneOf(IS_COND); }
	inline bool isMulti(void) { return meets(IS_MEM | IS_MULTI); }
	inline bool isSpecial(void) { return oneOf(IS_SPECIAL); }
	inline bool isMul(void) { return oneOf(IS_MUL); }
	inline bool isDiv(void) { return oneOf(IS_DIV); }

	// Low-level register access
	virtual const elm::genstruct::Table<hard::Register *>& readRegs(void);
	virtual const elm::genstruct::Table<hard::Register *>& writtenRegs(void);

	// Specialized information
	virtual bool isPseudo(void);
	virtual PseudoInst *toPseudo(void);
	virtual Inst *target(void);
	virtual Type *type(void);
	virtual int multiCount(void);

	// Stack information
	static const unsigned long READ = 0x01;
	static const unsigned long WRITE = 0x02;
	static const long UNKNOW_CHANGE = 0x80000000;
	virtual long stackChange(void);
	virtual unsigned long stackAccess(void);

	// ProgItem overload
	virtual Inst *toInst(void);
};


// PseudoInst class
class PseudoInst: public virtual Inst {
	const AbstractIdentifier *_id;
public:
	inline PseudoInst(const AbstractIdentifier *id): _id(id) { };
	inline const AbstractIdentifier *id(void) const { return _id; };
	virtual address_t address(void) const;
	virtual size_t size(void) const { return 0; };
	virtual void dump(io::Output& out);
	virtual bool isPseudo(void) { return true; };
	virtual PseudoInst *toPseudo(void) { return this; };
	virtual kind_t kind(void) { return 0; };
};


// output
inline elm::io::Output& operator<<(elm::io::Output& out, Inst *inst) {
	inst->dump(out);
	return out;
}

} // namespace otawa

#endif // OTAWA_INST_H
