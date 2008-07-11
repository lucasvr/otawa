/*
 *	$Id$
 *	ipet::FlowFactLoader class implementation
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2005-08, IRIT UPS.
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

#include <otawa/cfg.h>
#include <elm/io.h>
#include <otawa/ipet/FlowFactConstraintBuilder.h>
#include <otawa/ipet/FlowFactLoader.h>
#include <otawa/ipet/IPET.h>
#include <otawa/util/Dominance.h>
#include <otawa/ilp.h>
#include <otawa/cfg/LoopUnroller.h>
#include <otawa/proc/ProcessorException.h>
#include <otawa/cfg/CFGCollector.h>
#include <otawa/util/Dominance.h>
#include <otawa/ipet/VarAssignment.h>
#include <otawa/flowfact/features.h>

namespace otawa { namespace ipet {

/**
 * @class FlowFactConstraintBuilder
 * This processor allows using extern flow facts in an IPET system.
 * Uses the LOOP_COUNT properties provided by FlowFactLoader to build constraints
 * 
 * @par Configuration
 * @li @ref FLOW_FACTS_PATH
 * 
 * @par Required Features
 * @li @ref ipet::ILP_SYSTEM_FEATURE
 * @li @ref ipet::COLLECTED_CFG_FEATURE
 * @li @ref ipet::FLOW_FACTS_FEATURE
 * @li @ref LOOP_HEADERS_FEATURE
 * 
 * @par Provided Features
 * @li @ref ipet::FLOW_FACTS_CONSTRAINTS_FEATURE
 */


/**
 * Build a new flow fact loader.
 */
FlowFactConstraintBuilder::FlowFactConstraintBuilder(void)
:	ContextualProcessor("otawa::ipet::FlowFactConstraintBuilder", Version(1, 1, 0))
{
	require(COLLECTED_CFG_FEATURE);
	require(LOOP_HEADERS_FEATURE);
	require(ASSIGNED_VARS_FEATURE);
	require(FLOW_FACTS_FEATURE);
	provide(FLOW_FACTS_CONSTRAINTS_FEATURE);
}


/**
 */
void FlowFactConstraintBuilder::setup(WorkSpace *ws) {
	path.clear();
	system = SYSTEM(ws);
	ASSERT(system);
}


/**
 */
void FlowFactConstraintBuilder::processBB(WorkSpace *ws, CFG *cfg, BasicBlock *bb) {
	if (Dominance::isLoopHeader(bb)) {
		int bound = ContextualLoopBound::undefined,
			total = ContextualLoopBound::undefined;
		if(isVerbose())
			log << "\t\tlooking bound for " << bb << io::endl;
		
		// Test contextual
		if(bound == ContextualLoopBound::undefined) {
			ContextualLoopBound *cbound = CONTEXTUAL_LOOP_BOUND(bb);
			if(cbound) {
				if(isVerbose())
					log << "\t\tfound CONTEXTUAL_LOOP_BOUND(" << bb << ") = " << cbound << io::endl;
				bound = cbound->findMax(path);
				total = cbound->findTotal(path);
				if(isVerbose())
					log << "\t\tmax = " << bound << ", total = " << total << io::endl;
			}
		}
		
		// Look simple bound
		if(bound == ContextualLoopBound::undefined)
			bound = LOOP_COUNT(bb);
		
		// Generate the constraint
		if(bound == ContextualLoopBound::undefined
		&& total == ContextualLoopBound::undefined)
			warn(_ << "no flow fact constraint for loop at " << bb->address());
		else  {
			
			
			
			// sum{(i,h) / h dom i} eih <= count * sum{(i, h) / not h dom x} xeih
			if(bound != ContextualLoopBound::undefined) {
				
				
				ASSERT(bound >= 0);
				
				/* Substract unrolling from loop bound */
				for (BasicBlock *bb2 = UNROLLED_FROM(bb); bb2; bb2 = UNROLLED_FROM(bb2))
					bound--;

				/* Set execution count to 0 for each unrolled iteration that cannot be taken */	
				if (bound < 0) {
					BasicBlock *bb2 = bb;
					otawa::ilp::Constraint *cons0 = system->newConstraint(otawa::ilp::Constraint::EQ);
					for (int i = 0; i < (-bound); i++) {
						for (BasicBlock::InIterator edge(bb); edge; edge++) {
							ASSERT(edge->source());
							otawa::ilp::Var *var  = VAR(edge);
							cons0->addLeft(1, var);
						}
						bb2 = UNROLLED_FROM(bb2);
					}
				}
				
				otawa::ilp::Constraint *cons = system->newConstraint(otawa::ilp::Constraint::LE);	
				for(BasicBlock::InIterator edge(bb); edge; edge++) {
					ASSERT(edge->source());
					otawa::ilp::Var *var = VAR(edge);
					if(Dominance::dominates(bb, edge->source()))
						cons->addLeft(1, var);
					else
						cons->addRight((bound < 0) ? 0 : bound, var);
				}

			}
			
			// eih <= total
			if(total != ContextualLoopBound::undefined) {
				otawa::ilp::Constraint *cons = system->newConstraint(otawa::ilp::Constraint::LE);
				
				BasicBlock *bb2 = bb;
				do {
					for(BasicBlock::InIterator edge(bb2); edge; edge++) {
						ASSERT(edge->source());
						otawa::ilp::Var *var = VAR(edge);
						if(Dominance::dominates(bb2, edge->source()) || UNROLLED_FROM(bb2))
							cons->addLeft(1, var);
					}
					bb2 = UNROLLED_FROM(bb2);				
				} while (bb2 != NULL);
				cons->addRight(total);
			}
		}
	}	
}


/**
 */
void FlowFactConstraintBuilder::enteringCall(
	WorkSpace *ws,
	CFG *cfg,
	BasicBlock *caller,
	BasicBlock *callee)
{
	path.push(callee->address());
}


/**
 */
void FlowFactConstraintBuilder::leavingCall(WorkSpace *ws, CFG *cfg) {
	path.pop();
}


/**
 * This feature asserts that constraints tied to the flow fact information
 * has been added to the ILP system.
 */
Feature<FlowFactConstraintBuilder>
	FLOW_FACTS_CONSTRAINTS_FEATURE("otawa::ipet::flow_facts_constraints");

} 

} // otawa::ipet
