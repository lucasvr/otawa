/*
 *	$Id$
 *	Copyright (c) 2003-06, IRIT UPS.
 *
 *	otawa/prog/FrameWork.h -- interface for FrameWork class.
 */
#ifndef OTAWA_PROG_FRAMEWORK_H
#define OTAWA_PROG_FRAMEWORK_H

#include <elm/Collection.h>
#include <elm/system/Path.h>
#include <otawa/properties.h>
#include <otawa/prog/Process.h>

namespace elm { namespace xom {
	class Element;
} } // elm::xom

namespace otawa {

using namespace elm;
using namespace elm::genstruct;

// Classes
class AbstractFeature;
class AST;
class ASTInfo;
class CFG;
class CFGInfo;
class File;
class Inst;
class Loader;
class Manager;
namespace hard {
	class Platform;
	class Processor;
}
namespace ilp {
	class System;
}
namespace sim {
	class Simulator;
}

// FrameWork class
class FrameWork: public PropList {
	Process *proc;
	Vector<const AbstractFeature *> features;
	
protected:
	virtual Property *getDeep(const AbstractIdentifier *id)
		{ return proc->getProp(id); };
public:
	FrameWork(Process *_proc);
	~FrameWork(void);
	inline Process *process(void) const { return proc; };
	
	// Process overload
	virtual hard::Platform *platform(void) { return proc->platform(); };
	virtual Manager *manager(void) { return proc->manager(); };
	virtual Inst *start(void) { return proc->start(); };
	virtual Inst *findInstAt(address_t addr) { return proc->findInstAt(addr); };
	virtual const hard::CacheConfiguration& cache(void);
	
	// CFG Management
	CFGInfo *getCFGInfo(void);
	CFG *getStartCFG(void);
	
	// AST Management
	ASTInfo *getASTInfo(void);
	
	// ILP support
	ilp::System *newILPSystem(bool max = true);

	// Configuration services
	elm::xom::Element *config(void);
	void loadConfig(const elm::system::Path& path);
	
	// Feature management
	void provide(const AbstractFeature& feature);
	bool isProvided(const AbstractFeature& feature);
	void remove(const AbstractFeature& feature);
};

};	// otawa

#endif	// OTAWA_PROG_FRAMEWORK_H
