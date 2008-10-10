/*
 *	$Id$
 *	CFG class interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2003-07, IRIT UPS.
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
#ifndef OTAWA_CFG_CFG_H
#define OTAWA_CFG_CFG_H

#include <assert.h>
#include <elm/genstruct/FragTable.h>
#include <otawa/cfg/BasicBlock.h>

namespace otawa {

// Classes
class BasicBlock;
class CodeItem;
class CFG;
class CFGInfo;

// Properties
extern Identifier<CFG *> ENTRY;
extern Identifier<int> INDEX;

// CFG class
class CFG: public PropList {
	typedef genstruct::FragTable<BasicBlock *> bbs_t;
public:
	//static Identifier ID_Dom;
	
	// Iterator
	class BBIterator: public bbs_t::Iterator {
	public:
		inline BBIterator(CFG *cfg): bbs_t::Iterator(cfg->getBBS()) { }
		inline BBIterator(const BBIterator& iter): bbs_t::Iterator(iter) { }
	};
	
	// Methods
	CFG(Segment *seg, BasicBlock *entry);
	virtual ~CFG(void);
	inline Segment *segment(void) const;
	String label(void);
	inline int number(void);
	address_t address(void);
	inline BasicBlock *entry(void);
	inline BasicBlock *exit(void);
	inline int countBB(void);
	bool dominates(BasicBlock *bb1, BasicBlock *bb2);
	inline bool isVirtual(void) const;
	inline bool isInlined(void) const;
	void numberBB(void);
	BasicBlock *firstBB(void);
	Inst *firstInst(void);

protected:
	friend class CFGInfo;

	unsigned long flags;
	EndBasicBlock _entry, _exit;
	static const unsigned long FLAG_Scanned = 0x01;
	static const unsigned long FLAG_Virtual = 0x02;
	static const unsigned long FLAG_Inlined = 0x04;
	bbs_t _bbs;

	CFG(void);
	virtual void scan(void);

private:
	inline const bbs_t& getBBS(void) {
		if(!(flags & FLAG_Scanned))
			scan();
		return _bbs;
	}
	Segment *_seg;
	BasicBlock *ent;
};


// CFG inlines
inline Segment *CFG::segment(void) const {
	return _seg;
};

inline BasicBlock *CFG::entry(void) {
	if(!(flags & FLAG_Scanned))
		scan();
	return &_entry;
}

inline BasicBlock *CFG::exit(void) {
	if(!(flags & FLAG_Scanned))
		scan();
	return &_exit;
}
inline int CFG::countBB(void) {
	if(!(flags & FLAG_Scanned))
		scan();
	return _bbs.length();
}

inline int CFG::number(void) {
	return(INDEX(this));
}

inline bool CFG::isVirtual(void) const {
	return flags & FLAG_Virtual;
}
inline bool CFG::isInlined(void) const {
	return flags & FLAG_Inlined;
}

// Property display
template <>
void Identifier<CFG *>::print(elm::io::Output& out, const Property& prop) const;

} // otawa

#endif // OTAWA_CFG_CFG_H
