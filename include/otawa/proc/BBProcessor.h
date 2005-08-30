/*
 *	$Id$
 *	Copyright (c) 2005, IRIT UPS.
 *
 *	otawa/proc/BBProcessor.h -- BBProcessor class interface.
 */
#ifndef OTAWA_PROC_BBPROCESSOR_H
#define OTAWA_PROC_BBPROCESSOR_H

#include <otawa/cfg/BasicBlock.h>
#include <otawa/proc/CFGProcessor.h>

namespace otawa {

class BBProcessor: public CFGProcessor {
public:
	BBProcessor(const PropList& props = PropList::EMPTY);
	BBProcessor(elm::String name, elm::Version version = elm::Version::ZERO,
		const PropList& props = PropList::EMPTY);

	virtual void processBB(FrameWork *fw, CFG *cfd, BasicBlock *bb) = 0;
	
	// CFGProcessor overload
	virtual void processCFG(FrameWork *fw, CFG *cfg);
};
	
} // otawa

#endif	// OTAWA_PROC_BBPROCESSOR_H
