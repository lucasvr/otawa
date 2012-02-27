/*
 *	arm2 -- OTAWA loader to support ARMv5 ISA with GLISS2
 *	PowerPC OTAWA plugin
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2011, IRIT UPS.
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

#include <otawa/prog/Loader.h>
#include <otawa/hard.h>
#include <gel/gel.h>
#include <gel/gel_elf.h>
#include <gel/debug_line.h>
#include <otawa/prog/sem.h>
#include <otawa/loader/arm.h>

extern "C" {
#	include <arm/api.h>
#	include <arm/config.h>
#	include <arm/used_regs.h>
}

namespace otawa {

namespace arm {

/**
 * @class Info
 * Information provided by an ARM loader supporting GLISS2 engine.
 */

/**
 * @fn void *Info::decode(Inst *inst);
 * Get the decode instruction by GLISS2.
 * The obtained data must be fried with free() method.
 * @param inst	Instruction to decode.
 * @return		Decoded data castable to arm_inst_t *.
 */

/**
 * @fn void Info::free(void *decoded);
 * Free an instruction previously obtained by decode() method.
 * @paramd decoded	Result of decode.
 */

/**
 * Identifier used to retrieve ARM specific information.
 * Must be accessed dynamically because ARM loader is a plugin !
 *
 * @par Hooks
 * @li Process from ARM loader.
 */
Identifier<arm::Info *> INFO("otawa::arm::INFO", 0);

}

namespace arm2 {

#include "otawa_kind.h"
#include "otawa_target.h"


/****** Platform definition ******/

// registers
static hard::PlainBank gpr("GPR", hard::Register::INT, 32, "r%d", 16);
static hard::Register sr("sr", hard::Register::BITS, 32);
static hard::MeltedBank misc("misc", &sr, 0);
static const hard::RegBank *banks_tab[] = { &gpr, &misc };
static genstruct::Table<const hard::RegBank *> banks_table(banks_tab, 2);

// register decoding
class RegisterDecoder {
public:
	RegisterDecoder(void) {

		// clear the map
		for(int i = 0; i < ARM_REG_COUNT; i++)
			map[i] = 0;

		// initialize it
		map[ARM_REG_UCPSR] = &sr;
		for(int i = 0; i < 16; i++)
			map[ARM_REG_GPR(i)] = gpr[i];
	}

	inline hard::Register *operator[](int i) const { ASSERT(i < ARM_REG_COUNT); return map[i]; }

private:
	hard::Register *map[ARM_REG_COUNT];
};
static RegisterDecoder register_decoder;


// semantics functions
#define _GPR(i)			gpr[i]->platformNumber()
#define _CPSR()			sr.platformNumber()
#define _BRANCH(t)		sem::branch(t)
#define _TRAP()			sem::trap(0)
#define _CONT()			sem::cont()
#define _IF(c, r, o)	sem::_if(c, r, o)
#define _LOAD(d, a, b)	sem::load(d, a, b)
#define _STORE(d, a, b)	sem::store(d, a, b)
#define _SCRATCH(d)		sem::scratch(d)
/*macro _set(d, a)			= "_SET"(d, a)*/
//#define _SETI(d, a)		sem::seti(d, a)
//#define _CMP(d, a, b)	sem::cmp(d, a, b)
/*macro cmpu(d, a, b)		= "_CMPU"(d, a, b)*/
//#define _ADD(d, a, b)	sem::add(d, a, b)
/*macro sub(d, a, b)		= "_SUB"(d, a, b)*/
//#define _SHL(d, a, b)	sem::shl(d, a, b)
//#define _SHR(d, a, b)	sem::shr(d, a, b)
//#define _ASR(d, a, b)	sem::asr(d, a, b)
/*macro neg(d, a)			= "_NEG"(d, a)
macro not(d, a)			= "_NOT"(d, a)
macro and(d, a, b)		= "_AND"(d, a, b)
macro or(d, a, b)		= "_AOR"(d, a, b)
macro xor(d, a, b)		= "_XOR"(d, a, b)
macro mul(d, a, b)		= "_MUL"(d, a, b)
macro mulu(d, a, b)		= "_MULU"(d, a, b)
macro div(d, a, b)		= "_DIV"(d, a, b)
macro divu(d, a, b)		= "_DIVU"(d, a, b)
macro mod(d, a, b)		= "_MOD"(d, a, b)
macro modu(d, a, b)		= "_MODU"(d, a, b)*/

#include "otawa_sem.h"


// platform
class Platform: public hard::Platform {
public:
	static const Identification ID;

	Platform(const PropList& props = PropList::EMPTY): hard::Platform(ID, props)
		{ setBanks(banks_table); }
	Platform(const Platform& platform, const PropList& props = PropList::EMPTY)
		: hard::Platform(platform, props)
		{ setBanks(banks_table); }

	// otawa::Platform overload
	virtual bool accept(const Identification& id)
		{ return id.abi() == "eabi" && id.architecture() == "arm"; }
};
const Platform::Identification Platform::ID("arm-eabi-");


/****** Instruction declarations ******/

class Process;

// Inst class
class Inst: public otawa::Inst {
public:

	inline Inst(Process& process, kind_t kind, Address addr)
		: proc(process), _kind(kind), _addr(addr), isRegsDone(false) { }

	// Inst overload
	virtual void dump(io::Output& out);
	virtual kind_t kind() { return _kind; }
	virtual address_t address() const { return _addr; }
	virtual t::size size() const { return 4; }
	virtual Process &process() { return proc; }

	virtual const elm::genstruct::Table<hard::Register *>& readRegs() {
		if (!isRegsDone) {
			decodeRegs();
			isRegsDone = true;
		}
		return in_regs;
	}

	virtual const elm::genstruct::Table<hard::Register *>& writtenRegs() {
		if(!isRegsDone) {
			decodeRegs();
			isRegsDone = true;
		}
		return out_regs;
	}
	
	virtual void semInsts (sem::Block &block);
protected:
	Process &proc;

private:
	void decodeRegs(void);
	kind_t _kind;
	elm::genstruct::AllocatedTable<hard::Register *> in_regs;
	elm::genstruct::AllocatedTable<hard::Register *> out_regs;
	arm_address_t _addr;
	bool isRegsDone;
};


// BranchInst class
class BranchInst: public Inst {
public:

	inline BranchInst(Process& process, kind_t kind, Address addr)
		: Inst(process, kind, addr), _target(0), isTargetDone(false)
		{ }

	virtual otawa::Inst *target();

protected:
	arm_address_t decodeTargetAddress(void);

private:
	otawa::Inst *_target;
	bool isTargetDone;
};



/****** Segment class ******/
class Segment: public otawa::Segment {
public:
	Segment(Process& process,
		CString name,
		address_t address,
		t::size size)
	: otawa::Segment(name, address, size, EXECUTABLE), proc(process) { }

protected:
	virtual otawa::Inst *decode(address_t address);

private:
	Process& proc;
};


/****** Process class ******/

class Process: public otawa::Process, public arm::Info {
public:

	Process(Manager *manager, hard::Platform *pf, const PropList& props = PropList::EMPTY)
	:	otawa::Process(manager, props),
	 	_start(0),
	 	oplatform(pf),
		_memory(0),
		init(false),
		map(0),
		no_stack(true)
	{
		ASSERTP(manager, "manager required");
		ASSERTP(pf, "platform required");

		// gliss2 ppc structs
		_platform = arm_new_platform();
		ASSERTP(_platform, "cannot create an arm_platform");
		_decoder = arm_new_decoder(_platform);
		ASSERTP(_decoder, "cannot create an arm_decoder");
		_memory = arm_get_memory(_platform, ARM_MAIN_MEMORY);
		ASSERTP(_memory, "cannot get main arm_memory");
		arm_lock_platform(_platform);
#		ifdef ARM_THUMB
			arm_set_cond_state(_decoder, 0);
#		endif

		// build arguments
		static char no_name[1] = { 0 };
		static char *default_argv[] = { no_name, 0 };
		static char *default_envp[] = { 0 };
		argc = ARGC(props);
		if (argc < 0)
			argc = 1;
		argv = ARGV(props);
		if (!argv)
			argv = default_argv;
		else
			no_stack = false;
		envp = ENVP(props);
		if (!envp)
			envp = default_envp;
		else
			no_stack = false;

		// handle features
		provide(MEMORY_ACCESS_FEATURE);
		provide(SOURCE_LINE_FEATURE);
		provide(CONTROL_DECODING_FEATURE);
		provide(REGISTER_USAGE_FEATURE);
		provide(MEMORY_ACCESSES);
		arm::INFO(this) = this;
	}

	virtual ~Process() {
		arm_delete_decoder(_decoder);
		arm_unlock_platform(_platform);
		if(_file)
			gel_close(_file);
	}

	// Process overloads
	virtual int instSize(void) const { return 4; }
	virtual hard::Platform *platform(void) { return oplatform; }
	virtual otawa::Inst *start(void) { return _start; }

	virtual File *loadFile(elm::CString path) {

		// check if there is not an already opened file !
		if(program())
			throw LoadException("loader cannot open multiple files !");

		// make the file
		File *file = new otawa::File(path);
		addFile(file);

		// build the environment
		gel_env_t genv = *gel_default_env();
		genv.argv = argv;
		genv.envp = envp;
		if(no_stack)
			genv.flags = GEL_ENV_NO_STACK;

		// build the GEL image
		_file = gel_open(&path, NULL, 0);
		if(!_file)
			throw LoadException(_ << "cannot load \"" << path << "\": " << gel_strerror());
		gel_image_t *gimage = gel_image_load(_file, &genv, 0);
		if(!gimage) {
			gel_close(_file);
			throw LoadException(_ << "cannot build image of \"" << path << "\": " << gel_strerror());
		}

		// build the GLISS image
		gel_image_info_t iinfo;
		gel_image_infos(gimage, &iinfo);
		for(int i = 0; i < iinfo.membersnum; i++) {
			gel_cursor_t cursor;
			gel_block2cursor(iinfo.members[i], &cursor);
			arm_mem_write(_memory,
				gel_cursor_vaddr(cursor),
				gel_cursor_addr(&cursor),
				gel_cursor_avail(cursor));
		}

		// cleanup image
		gel_image_close(gimage);

		// build segments
		gel_file_info_t infos;
		gel_file_infos(_file, &infos);
		for (int i = 0; i < infos.sectnum; i++) {
			gel_sect_info_t infos;
			gel_sect_t *sect = gel_getsectbyidx(_file, i);
			assert(sect);
			gel_sect_infos(sect, &infos);
			if(infos.vaddr != 0 && infos.size != 0) {
				Segment *seg = new Segment(*this, infos.name, infos.vaddr, infos.size);
				file->addSegment(seg);
			}
		}

		// Initialize symbols
		gel_enum_t *iter = gel_enum_file_symbol(_file);
		gel_enum_initpos(iter);
		for(char *name = (char *)gel_enum_next(iter); name; name = (char *)gel_enum_next(iter)) {
			ASSERT(name);
			Address addr = Address::null;
			Symbol::kind_t kind;
			gel_sym_t *sym = gel_find_file_symbol(_file, name);
			ASSERT(sym);
			gel_sym_info_t infos;
			gel_sym_infos(sym, &infos);
			switch(ELF32_ST_TYPE(infos.info)) {
			case STT_FUNC:
				kind = Symbol::FUNCTION;
				addr = Address(infos.vaddr);
				break;
			case STT_NOTYPE:
				kind = Symbol::LABEL;
				addr = Address(infos.vaddr);
				break;
			default:
				continue;
			}

			// Build the label if required
			if(addr != Address::null) {
				String label(infos.name);
				Symbol *sym = new Symbol(*file, label, kind, addr);
				file->addSymbol(sym);
			}
		}
		gel_enum_free(iter);

		// Last initializations
		_start = findInstAt((address_t)infos.entry);
		return file;
	}

	// internal work
	void decodeRegs(Inst *oinst,
		elm::genstruct::AllocatedTable<hard::Register *> *in,
		elm::genstruct::AllocatedTable<hard::Register *> *out)
	{
		// Decode instruction
		arm_inst_t *inst = decode_raw(oinst->address());
		if(inst->ident == ARM_UNKNOWN) {
			free_inst(inst);
			return;
		}

		// get the registers
		arm_used_regs_read_t rds;
		arm_used_regs_write_t wrs;
		arm_used_regs(inst, rds, wrs);

		// convert registers to OTAWA
		elm::genstruct::Vector<hard::Register *> reg_in;
		elm::genstruct::Vector<hard::Register *> reg_out;
		for (int i = 0; rds[i] != -1; i++ ) {
			hard::Register *r = register_decoder[rds[i]];
			if(r)
				reg_in.add(r);
		}
		for (int i = 0; wrs[i] != -1; i++ ) {
			hard::Register *r = register_decoder[wrs[i]];
			if(r)
				reg_out.add(r);
		}

		// make the in and the out
		in->allocate(reg_in.length());
		for(int i = 0 ; i < reg_in.length(); i++)
			in->set(i, reg_in.get(i));
		out->allocate(reg_out.length());
		for (int i = 0 ; i < reg_out.length(); i++)
			out->set(i, reg_out.get(i));

		// Free instruction
		free_inst(inst);
	}

	otawa::Inst *decode(Address addr) {
		arm_inst_t *inst = decode_raw(addr);
		Inst::kind_t kind = 0;
		otawa::Inst *result = 0;
		kind = arm_kind(inst);
		if(kind & Inst::IS_CONTROL)
			result = new BranchInst(*this, kind, addr);
		else
			result = new Inst(*this, kind, addr);
		free_inst(inst);
		return result;
	}


	// GLISS2 ARM access
	inline int opcode(Inst *inst) const {
		arm_inst_t *i = decode_raw(inst->address());
		int code = i->ident;
		free_inst(i);
		return code;
	}

	inline ::arm_inst_t *decode_raw(Address addr) const
#		ifdef ARM_THUMB
			{ return arm_decode_ARM(decoder(), ::arm_address_t(addr.offset())); }
#		else
			{ return arm_decode(decoder(), ::arm_address_t(addr.offset())); }
#		endif

	inline void free_inst(arm_inst_t *inst) const { arm_free_inst(inst); }
	virtual gel_file_t *file(void) const { return _file; }
	virtual arm_memory_t *memory(void) const { return _memory; }
	inline arm_decoder_t *decoder() const { return _decoder; }
	inline void *platform(void) const { return _platform; }

	virtual Option<Pair<cstring, int> > getSourceLine(Address addr) throw (UnsupportedFeatureException) {
		setup_debug();
		if (!map)
			return none;
		const char *file;
		int line;
		if (!map || gel_line_from_address(map, addr.offset(), &file, &line) < 0)
			return none;
		return some(pair(cstring(file), line));
	}

	virtual void getAddresses(cstring file, int line, Vector<Pair<Address, Address> >& addresses) throw (UnsupportedFeatureException) {
		setup_debug();
		addresses.clear();
		if (!map)
			return;
		gel_line_iter_t iter;
		gel_location_t loc, ploc = { 0, 0, 0, 0 };
		for (loc = gel_first_line(&iter, map); loc.file; loc = gel_next_line(&iter))
		{
			cstring lfile = loc.file;
			//cerr << loc.file << ":" << loc.line << ", " << loc.low_addr << "-" << loc.high_addr << io::endl;
			if (file == loc.file || lfile.endsWith(file))
			{
				if (line == loc.line)
				{
					//cerr << "added (1) " << loc.file << ":" << loc.line << " -> " << loc.low_addr << io::endl;
					addresses.add(pair(Address(loc.low_addr), Address(loc.high_addr)));
				}
				else if(loc.file == ploc.file && line > ploc.line && line < loc.line)
				{
					//cerr << "added (2) " << ploc.file << ":" << ploc.line << " -> " << ploc.low_addr << io::endl;
					addresses.add(pair(Address(ploc.low_addr), Address(ploc.high_addr)));
				}
			}
			ploc = loc;
		}
	}

	virtual void get(Address at, signed char& val)
		{ val = arm_mem_read8(_memory, at.address()); }
	virtual void get(Address at, unsigned char& val)
		{ val = arm_mem_read8(_memory, at.address()); }
	virtual void get(Address at, signed short& val)
		{ val = arm_mem_read16(_memory, at.address()); }
	virtual void get(Address at, unsigned short& val)
		{ val = arm_mem_read16(_memory, at.address()); }
	virtual void get(Address at, signed long& val)
		{ val = arm_mem_read32(_memory, at.address()); }
	virtual void get(Address at, unsigned long& val)
		{ val = arm_mem_read32(_memory, at.address()); }
	virtual void get(Address at, signed long long& val)
		{ val = arm_mem_read64(_memory, at.address()); }
	virtual void get(Address at, unsigned long long& val)
		{ val = arm_mem_read64(_memory, at.address()); }
	virtual void get(Address at, Address& val)
		{ val = arm_mem_read32(_memory, at.address()); }
	virtual void get(Address at, string& str) {
		Address base = at;
		while(!arm_mem_read8(_memory, at.address()))
			at = at + 1;
		int len = at - base;
		char buf[len];
		get(base, buf, len);
		str = String(buf, len);
	}
	virtual void get(Address at, char *buf, int size)
		{ arm_mem_read(_memory, at.address(), buf, size); }

	// otawa::arm::Info overload
	virtual void *decode(otawa::Inst *inst) { return decode_raw(inst->address()); }
	virtual void free(void *decoded) { free_inst((arm_inst_t *)decoded); }

private:
	void setup_debug(void) {
		ASSERT(_file);
		if(init)
			return;
		init = true;
		map = gel_new_line_map(_file);
	}

	otawa::Inst *_start;
	hard::Platform *oplatform;
	arm_platform_t *_platform;
	arm_memory_t *_memory;
	arm_decoder_t *_decoder;
	gel_line_map_t *map;
	gel_file_t *_file;
	int argc;
	char **argv, **envp;
	bool no_stack;
	bool init;
};


/****** Instructions implementation ******/

void Inst::dump(io::Output& out) {
	char out_buffer[200];
	arm_inst_t *inst = proc.decode_raw(_addr);
	arm_disasm(out_buffer, inst);
	proc.free_inst(inst);
	out << out_buffer;
}

void Inst::decodeRegs(void) {

	// Decode instruction
	arm_inst_t *inst = proc.decode_raw(address());
	if(inst->ident == ARM_UNKNOWN)
		return;

	// get register infos
	arm_used_regs_read_t rds;
	arm_used_regs_write_t wrs;
	elm::genstruct::Vector<hard::Register *> reg_in;
	elm::genstruct::Vector<hard::Register *> reg_out;
	arm_used_regs(inst, rds, wrs);
	for (int i = 0; rds[i] != -1; i++ ) {
		hard::Register *r = register_decoder[rds[i]];
		if(r)
			reg_in.add(r);
	}
	for (int i = 0; wrs[i] != -1; i++ ) {
		hard::Register *r = register_decoder[wrs[i]];
		if(r)
			reg_out.add(r);
	}

	// store results
	in_regs.allocate(reg_in.length());
	for(int i = 0 ; i < reg_in.length(); i++)
		in_regs.set(i, reg_in.get(i));
	out_regs.allocate(reg_out.length());
	for (int i = 0 ; i < reg_out.length(); i++)
		out_regs.set(i, reg_out.get(i));

	// Free instruction
	arm_free_inst(inst);
}


arm_address_t BranchInst::decodeTargetAddress(void) {
	arm_inst_t *inst= proc.decode_raw(address());
	Address target_addr = arm_target(inst);
	proc.free_inst(inst);
	return target_addr;
}


otawa::Inst *BranchInst::target() {
	if (!isTargetDone) {
		arm_address_t a = decodeTargetAddress();
		if (a)
			_target = process().findInstAt(a);
		isTargetDone = true;
	}
	return _target;
}


otawa::Inst *Segment::decode(address_t address) {
	return proc.decode(address);
}


void Inst::semInsts (sem::Block &block) {
	arm_inst_t *inst = proc.decode_raw(address());
	if(inst->ident == ARM_UNKNOWN)
		return;
	arm_sem(inst, block);
	arm_free_inst(inst);

}

/****** loader definition ******/

// alias table
static string table[] = { "elf_40", "arm2" };
static elm::genstruct::Table<string> loader_aliases(table, 1);

// loader definition
class Loader: public otawa::Loader {
public:
	Loader(void): otawa::Loader("arm", Version(2, 0, 0), OTAWA_LOADER_VERSION, loader_aliases) {
	}

	virtual CString getName(void) const { return "arm"; }

	virtual otawa::Process *load(Manager *man, CString path, const PropList& props) {
		otawa::Process *proc = create(man, props);
		if (!proc->loadProgram(path)) {
			delete proc;
			return 0;
		}
		else
			return proc;
	}

	virtual otawa::Process *create(Manager *man, const PropList& props) {
		return new Process(man, new Platform(props), props);
	}
};


} }		// otawa::arm2

// hooks
otawa::arm2::Loader OTAWA_LOADER_HOOK;
otawa::arm2::Loader& arm2_plugin = OTAWA_LOADER_HOOK;

