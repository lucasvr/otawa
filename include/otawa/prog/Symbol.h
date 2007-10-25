/*
 *	$Id$
 *	Copyright (c) 2003-07, IRIT UPS.
 *
 *	Symbol class interface
 */
#ifndef OTAWA_PROG_SYMBOL_H
#define OTAWA_PROG_SYMBOL_H

#include <elm/io.h>
#include <otawa/properties.h>

namespace otawa {

// External classes
class File;
class Inst;
	
// Symbol class
class Symbol {
public:

	typedef enum kind_t {
		NONE,
		FUNCTION,
		LABEL
	} kind_t;
	
	static Identifier<Symbol *> ID;

	Symbol(File& file, String name, kind_t kind, address_t address, size_t size = 0);
	inline File& file(void) const { return _file; }
	inline kind_t kind(void) const { return _kind; }
	inline const String& name(void) const { return _name; }
	inline address_t address(void) const { return _address; }
	inline size_t size(void) const { return _size; }
	Inst *findInst(void) const;
	inline bool doesNotReturn(void) const { return no_return; } 
	void setNoReturn(void);

private:
	File& _file;
	String _name;
	kind_t _kind;
	address_t _address;	
	size_t _size;
	bool no_return;
};

// GenericIdentifier<Symbol_t *>::print Specialization
template <>
void Identifier<Symbol *>::print(elm::io::Output& output, const Property& prop) const;

} // otawa

#endif	// OTAWA_SYMBOL_H
