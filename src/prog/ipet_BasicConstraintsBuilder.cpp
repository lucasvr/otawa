/*
 *	$Id$
 *	BasicConstraintsBuilder class implementation
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2005-07, IRIT UPS.
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

#include <otawa/ilp.h>
#include <otawa/ipet/IPET.h>
#include <otawa/ipet/BasicConstraintsBuilder.h>
#include <otawa/cfg.h>
#include <otawa/ipet/VarAssignment.h>
#include <otawa/ipet/ILPSystemGetter.h>

using namespace otawa::ilp;

namespace otawa { namespace ipet {

/**
 * Used to record the constraint of a called CFG.
 */
Identifier<Constraint *> CALLING_CONSTRAINT("otawa::ipet::calling_constraint", 0);


/**
 * @class BaseConstraintsBuilder
 * <p>This code processor create or re-use the ILP system in the framework and
 * add to it basic IPET system constraints as described in the article below:</p>
 * <p>Y-T. S. Li, S. Malik, <i>Performance analysis of embedded software using
 * implicit path enumeration</i>, Workshop on languages, compilers, and tools
 * for real-time systems, 1995.</p>
 * 
 * <p>For each basic bloc, the following
 * constraint is added:</p>
 * <p>
 * 		ni = i1 + i2 + ... + in<br>
 * 		ni = o1 + o2 + ... + om
 * </p>
 * <p> where </p>
 * <dl>
 * 	<dt>ni</dt><dd>Count of executions of basic block i.</dd>
 *  <dt>ik</dt><dd>Count of traversal of input edge k in basic block i</dd>
 *  <dt>ok</dt><dd>Count of traversal of output edge k in basic block i</dd>
 * </dl>
 * 
 * <p>And, finally, put the constraint on the entry basic block of the CFG:</p>
 * <p>
 * 		n1 = 1
 * </p>
 *
 * @par Provided Features
 * @li @ref ipet::CONTROL_CONSTRAINTS_FEATURE
 * 
 * @par Required Features
 * @li @ref ipet::ASSIGNED_VARS_FEATURE
 * @li @ref ipet::ILP_SYSTEM_FEATURE
 */


/**
 * Add the given variable to the entry constraint of the given CFG.
 * @param system	Current ILP system.
 * @param called	Looked CFG.
 * @param var		Variable to add.
 * @return			Entry CFG constraint.
 */
void BasicConstraintsBuilder::addEntryConstraint(System *system, CFG *called, Var *var) {
	Constraint *cons = CALLING_CONSTRAINT(called);
	if(!cons) {
		cons = system->newConstraint(Constraint::EQ);
		ASSERT(cons);
		cons->addLeft(1, VAR(called->entry()));
		CALLING_CONSTRAINT(called) = cons;
	}
	cons->addRight(1, var);
}


/**
 */
void BasicConstraintsBuilder::processBB (WorkSpace *fw, CFG *cfg, BasicBlock *bb)
{
	assert(fw);
	assert(cfg);
	assert(bb);

	// Prepare data
	Constraint *cons;
	bool used;
	CFG *called = 0;
	System *system = SYSTEM(fw);
	assert(system);
	Var *bbv = VAR( bb);
		
	// Input constraint
	cons = system->newConstraint(Constraint::EQ);
	cons->addLeft(1, bbv);
	used = false;
	for(BasicBlock::InIterator edge(bb); edge; edge++)
		if(edge->kind() != Edge::CALL) {
			cons->addRight(1, VAR(edge));
			used = true;
		}
	if(!used)
		delete cons;
	
	// Output constraint
	bool many_calls = false;
	cons = system->newConstraint(Constraint::EQ);
	cons->addLeft(1, bbv);
	used = false;
	for(BasicBlock::OutIterator edge(bb); edge; edge++) {
		if(edge->kind() != Edge::CALL) {
			cons->addRight(1, VAR(edge));
			used = true;
		}
		else {
			if(!edge->calledCFG())
				throw ProcessorException(*this, _ << "unresolved call at " << bb->address());
			if(called)
				many_calls = true;
			else
				called = edge->calledCFG();
		}
	}
	if(!used)
		delete cons;

	// Process the call
	if(called) {
		
		// Simple call
		if(!many_calls)
			addEntryConstraint(system, called, bbv);
		
		// Multiple calls
		else {
			Constraint *call_cons = system->newConstraint(Constraint::EQ);
			ASSERT(call_cons);
			call_cons->addLeft(1, bbv);
			for(BasicBlock::OutIterator edge(bb); edge; edge++)
				if(edge->kind() == Edge::CALL) {
					CFG *called_cfg = edge->calledCFG();
					
					// Create the variable
					String name;
					if(_explicit)
						name = _ << "call_" << bb->number() << '_' << cfg->label()
							<< "_to_" << called_cfg->label();
					Var *call_var = system->newVar(name);
					
					// Add the variable to the constraints
					addEntryConstraint(system, called_cfg, call_var);
					call_cons->addRight(1, call_var);
				}
		}
	}	
}


/**
 */	
void BasicConstraintsBuilder::processWorkSpace(WorkSpace *fw) {
	assert(fw);
	
	// Call the orignal processing
	BBProcessor::processWorkSpace(fw);
	
	// Just record the constraint "entry = 1"
	CFG *cfg = ENTRY_CFG(fw);
	assert(cfg);
	System *system = SYSTEM(fw);
	BasicBlock *entry = cfg->entry();
	assert(entry);
	Constraint *cons = system->newConstraint(Constraint::EQ, 1);
	cons->addLeft(1, VAR(entry));
};


/**
 * Build basic constraint builder processor.
 */
BasicConstraintsBuilder::BasicConstraintsBuilder(void)
: BBProcessor("otawa::ipet::BasicConstraintsBuilder", Version(1, 0, 0)) {
	provide(CONTROL_CONSTRAINTS_FEATURE);
	require(ASSIGNED_VARS_FEATURE);
	require(ILP_SYSTEM_FEATURE);
}


void BasicConstraintsBuilder::configure(const PropList &props) {
	BBProcessor::configure(props);
	_explicit = EXPLICIT(props);
}


/**
 * This feature ensures that control constraints has been added to the
 * current ILP system.
 */
Feature <BasicConstraintsBuilder>
	CONTROL_CONSTRAINTS_FEATURE("otawa::control_constraints");

} } //otawa::ipet
