/*
 *	$Id$
 *	Copyright (c) 2003, IRIT UPS.
 *
 *	cfg.cpp -- control flow graph classes implementation.
 */

#include <assert.h>
#include <elm/debug.h>
#include <elm/datastruct/Collection.h>
#include <otawa/cfg.h>

namespace otawa {

/**
 * @class CFG
 * Control Flow Graph representation. Its entry basic block is given and
 * the graph is built using following taken and not-taken properties of the block.
 */

/**
 * Identifier used for storing and retrieving the CFG on its entry BB.
 */
id_t CFG::ID = Property::getID("otawa.CFG");

/**
 * Constructor. Add a property to the basic block for quick retrieval of
 * the matching CFG.
 */
CFG::CFG(Code *code, BasicBlock *entry): ent(entry), _code(code) {
	assert(code && entry);
	ent->set<CFG *>(ID, this);
	Option<String> label = entry->get<String>(File::ID_Label);
	if(label)
		set<String>(File::ID_Label, *label);
}


/**
 * @fn BasicBlock *CFG::entry(void) const;
 * Get the entry basic block of the CFG.
 * @return Entry basic block.
 */


/**
 * @fn Code *CFG::code(void) const;
 * Get the code containing the CFG.
 * @return Container code.
 */


/**
 * Get the CFG name, that is, the label associated with the entry of the CFG.
 * @return	CFG label or an empty string.
 */
String CFG::label(void) {
	return ent->get<String>(File::ID_Label, "");
}


/**
 * Get the address of the first instruction of the CFG.
 * @return	Return address of the first instruction.
 */
address_t CFG::address(void) {
	return ent->address();
}


// Function for handling removal of CFGInfo
/*GenericProperty<CFGInfo *>::~GenericProperty(void) {
	delete value;
}*/

} // namespace otawa
