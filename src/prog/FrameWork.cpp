/*
 *	$Id$
 *	Copyright (c) 2003, IRIT UPS.
 *
 *	src/prog/FrameWork.cpp -- implementation for FrameWork class.
 */

#include <otawa/manager.h>
#include <otawa/prog/FrameWork.h>

namespace otawa {

/**
 * @class FrameWork
 * A framework represents a program, its run-time and all information about
 * WCET computation or any other analysis.
 */

	
/**
 * Build a new framework with the given process.
 * @param _proc	Process to use.
 */
FrameWork::FrameWork(Process *_proc): proc(_proc) {
	assert(_proc);
	addProps(*_proc);
	Manager *man = _proc->manager();
	man->frameworks.add(this);
}


/**
 * Delete the framework and the associated process.
 */
FrameWork::~FrameWork(void) {
	clearProps();
	Manager *man = proc->manager();
	man->frameworks.remove(this);
	delete proc;
}


/**
 * Build the CFG of the project.
 */
void FrameWork::buildCFG(void) {
	
	// Get a CFG information descriptor
	AutoPtr<CFGInfo> info = get< AutoPtr<CFGInfo> >(CFGInfo::ID, 0);
	if(info)
		info->clear();
	else
		info = new CFGInfo(this);
	
	// Build the new one
	for(Iterator<File *> file(*files()); file; file++)
		for(Iterator<Segment *> seg(file->segments()); seg; seg++)
			for(Iterator<ProgItem *> item(seg->items()); item; item++)
				if(seg->flags() & Segment::EXECUTABLE)
					info->addCode((Code *)*item);
	
	// Add the entry point
	Inst *_start = start();
	if(_start)
		info->addSubProgram(_start);
}


/**
 * Get the CFG of the project. If it does not exists, built it.
 */
AutoPtr<CFGInfo> FrameWork::getCFGInfo(void) {
	AutoPtr<CFGInfo> info = get< AutoPtr<CFGInfo> >(CFGInfo::ID, 0);
	if(!info)
		buildCFG();
	return use< AutoPtr<CFGInfo> >(CFGInfo::ID);
}


/**
 * Get the entry CFG of the program.
 * @return Entry CFG if any or null.
 */
CFG *FrameWork::getStartCFG(void) {
	
	// Find the entry
	Inst *_start = start();
	if(!_start)
		return 0;
	
	// Get the CFG information
	AutoPtr<CFGInfo> info = getCFGInfo();
	
	// Find CFG attached to the entry
	return info->findCFG(_start);
}


} // otawa
