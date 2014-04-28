/*
 *	etime plugin hook
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2014, IRIT UPS.
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

#include <otawa/proc/ProcessorPlugin.h>
#include <otawa/etime/StandardEventBuilder.h>

namespace otawa { namespace etime {


/**
 * @defgroup etime	Event Time Module
 * This module aims to make more generic how events (cache hits/misses, branch prediction, etc)
 * are processed to generate block time.
 *
 * The basic idea is that each analysis contributes to the WCET by defining the events
 * (changes of the execution time) applying to instruction or a block. As a result, these events
 * are hooked to the basic block they apply to and design (a) which instruction is concerned,
 * (b) which part of the instruction execution they apply to (memory access, stage, functional unit, etc)
 * and (c) how they contribute to the time.
 *
 * In addition, a event may a contribution to the constraints of the ILP system and may be asked
 * to provide a overestimation or an underestimation of their occurrence according to their
 * contribution (worst case time or best case time) to the WCET.
 *
 * From the events, several policies may be applied to compute the times, to select
 * the granularity and the precision of the computed time or to choose a way to split
 * the CFG into blocks.
 */

/**
 * This feature ensures that events of the following analyses has been hooked to the task basic blocks:
 * @li L1 instruction cache by category,
 * @li L1 data cache by category,
 * @li branch prediction by categrory.
 *
 * @par Properties
 *  @li @ref EVENT
 *
 * @ingroup etime
 */
p::feature STANDARD_EVENTS_FEATURE("otawa::etime::STANDARD_EVENTS_FEATURE", new Maker<StandardEventBuilder>());


/**
 * Allows to hook an event to a basic block.
 *
 * @par Feature
 * @li @ref STANDARD_EVENTS_FEATURE
 *
 * @par Hooks
 * @li @ref BasicBlock
 *
 * @ingroup etime
 */
Identifier<Event *> EVENT("otawa::etime::EVENT", 0);


/**
 * @class Event
 * An event represents a time variation in the execution of an instruction.
 * Examples of events include instruction/data cache hit/miss, resolution
 * of a branch prediction, hit/miss in the prefetch device of flash memory, etc.
 *
 * Events are used to compute the execution time of code blocks
 * and are usually linked to their matching block.
 *
 * @ingroup etime
 */


/**
 * Build an event.
 * @param inst	Instruction it applies to.
 */
Event::Event(Inst *inst): _inst(inst) {
}


/**
 */
Event::~Event(void) {
}


/**
 * @fn Inst *Event::inst(void) const;
 * Get the instruction this event applies to.
 * @return	Event instruction.
 */


/**
 * @fn kind_t Event::kind(void) const;
 * Get the kind of the event.
 * @return	Event kind.
 */


/**
 * Get the occurrences class of this event.
 * @return	Event occurrence class.
 */
occurrence_t Event::occurrence(void) const {
	return SOMETIMES;
}


/**
 * Get the name of the event (for human user).
 * @return	Event name.
 */
cstring Event::name(void) const {
	return "";
}


/**
 * For events applying to a particular processor stage,
 * get this stage.
 * @return	Processor stage the event applies to.
 */
const hard::Stage *Event::stage(void) const {
	return 0;
}


/**
 * For events applying to a particular processor functional unit,
 * get this functional unit.
 * @return	Processor functional unit the event applies to.
 */
const hard::FunctionalUnit *Event::fu(void) const {
	return 0;
}


/**
 * This method may be called to let the event add its own constraint
 * to the given ILP system.
 * @param sys	ILP system to contribute to.
 */
void Event::contribute(ilp::System *sys) {
}


/**
 * This method may be called to let the event add an overestimation
 * of its occurrences to the right of the constraint.
 * @param cons	Constraint to add occurrences to.
 */
void Event::overestimate(ilp::Constraint *cons) {
}


/**
 * This method may be called to let the event add an underestimation
 * of its occurrences to the right of the constraint.
 * @param cons	Constraint to add occurrences to.
 */
void Event::underestimate(ilp::Constraint *cons) {
}


/**
 * @fn ot::time Event::cost(void) const;
 * Get the cost in cycles of the occurrence of the event.
 * @return	Cost of the event (in cycles).
 */


class Plugin: public ProcessorPlugin {
public:
	typedef genstruct::Table<AbstractRegistration * > procs_t;

	Plugin(void): ProcessorPlugin("otawa::etime", Version(1, 0, 0), OTAWA_PROC_VERSION) { }
	virtual procs_t& processors(void) const { return procs_t::EMPTY; };
};

} }	// otawa::etime

otawa::etime::Plugin OTAWA_PROC_HOOK;
otawa::etime::Plugin& otawa_etime = OTAWA_PROC_HOOK;
