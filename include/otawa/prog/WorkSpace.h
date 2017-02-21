/*
 *	WorkSpace class interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2007-8, IRIT UPS.
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
 *	This file is part of OTAWA
 *	Copyright (c) 2007-08, IRIT UPS.
 */
#ifndef OTAWA_PROG_WORK_SPACE_H
#define OTAWA_PROG_WORK_SPACE_H

#include <elm/data/List.h>
#include <elm/genstruct/HashTable.h>
#include <elm/genstruct/Vector.h>
#include <elm/sys/Path.h>
#include <elm/sys/Thread.h>
#include <elm/util/LockPtr.h>

#include <otawa/properties.h>

namespace elm { namespace xom {
	class Element;
} } // elm::xom

namespace otawa {

using namespace elm;

// Classes
class AbstractFeature;
class FeatureDependency;
namespace ast {
	class AST;
	class ASTInfo;
}
class CFG;
class CFGInfo;
class File;
class Inst;
class Loader;
class Manager;
class Process;
class Processor;
namespace hard {
	class CacheConfiguration;
	class Platform;
	class Processor;
}
namespace ilp {
	class System;
}
namespace sim {
	class Simulator;
}

// WorkSpace class
class WorkSpace: public PropList {
public:
	WorkSpace(Process *_proc);
	WorkSpace(const WorkSpace *ws);
	virtual ~WorkSpace(void);
	inline Process *process(void) const { return &proc; };

	// Process overload
	virtual hard::Platform *platform(void);
	virtual Manager *manager(void);
	virtual Inst *start(void);
	virtual Inst *findInstAt(address_t addr);
	string format(Address addr, bool with_address = true);

	// Configuration services
	elm::xom::Element *config(void);
	void loadConfig(const elm::sys::Path& path);

	// Feature management
	void run(Processor *proc, const PropList& props, bool del_proc = false);
	inline void run(Processor& proc, const PropList& props, bool del_proc = false) { run(&proc, props); }
	void require(const AbstractFeature& feature, const PropList& props = PropList::EMPTY);
	bool isProvided(const AbstractFeature& feature);
	void invalidate(const AbstractFeature& feature);

	// cancellation management
	inline void clearCancellation(void) { cancelled = false; }
	inline void cancel(void) { cancelled = true; }
	inline bool isCancelled(void) const { return cancelled; }

	// serialization
	virtual void serialize(elm::serial2::Serializer& serializer);
	virtual void unserialize(elm::serial2::Unserializer& unserializer);

	// concurrency support
	static sys::Thread *run(sys::Runnable& run);
	static void runAll(sys::Runnable& run);
	static void remove(Property *prop);

	// deprecated
	ast::ASTInfo *getASTInfo(void);
	ilp::System *newILPSystem(bool max = true);
	CFGInfo *getCFGInfo(void);
	CFG *getStartCFG(void);

private:

	// new dependency system
	typedef struct Dependency {
		Dependency(void);
		Dependency(Processor *proc, bool del_proc = false);
		Processor *_proc;
		List<struct Dependency *> _users, _used;
		bool _del_proc;
	} Dependency ;
	typedef genstruct::HashTable<const AbstractFeature *, Dependency *> dep_map_t;
	dep_map_t dep_map;

	void invalidate(Dependency *dep);
	void add(Processor *proc, bool del_proc);
	void remove(Dependency *dep);

	LockPtr<Process> proc;
	bool cancelled;
};

};	// otawa

#endif	// OTAWA_PROG_WORK_SPACE_H
