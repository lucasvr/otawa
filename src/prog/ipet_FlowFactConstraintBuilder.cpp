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
#include <otawa/dfa/BitSet.h>
#include <otawa/util/FlowFactLoader.h>

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
:	ContextualProcessor("otawa::ipet::FlowFactConstraintBuilder", Version(1, 1, 0)),
	_explicit(false)
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

	if (LOOP_HEADER(bb)) {
		int max = ContextualLoopBound::undefined,
			total = ContextualLoopBound::undefined,
			min = ContextualLoopBound::undefined;
		if(isVerbose())
			log << "\t\tlooking bound for " << bb << io::endl;

		// Test contextual
		if(max == ContextualLoopBound::undefined) {
			ContextualLoopBound *cbound = CONTEXTUAL_LOOP_BOUND(bb);
			if(cbound) {
				if(isVerbose())
					log << "\t\tfound CONTEXTUAL_LOOP_BOUND(" << bb << ") = " << cbound << io::endl;
				max = cbound->findMax(path);
				total = cbound->findTotal(path);
				if(isVerbose())
					log << "\t\tmax = " << max << ", total = " << total << io::endl;
			}
		}

		// Look simple bound
		if(max == ContextualLoopBound::undefined)
			max = MAX_ITERATION(bb);
		if(min == ContextualLoopBound::undefined)
			min = MIN_ITERATION(bb);
		if(total == ContextualLoopBound::undefined)
			total = TOTAL_ITERATION(bb);

		// Generate the constraint
		if(max == ContextualLoopBound::undefined
		&& total == ContextualLoopBound::undefined) {
			warn(_ << "no flow fact constraint for loop at " << bb->address());
			return;
		}

		// constraint for MAX_ITERATION
		if(max != ContextualLoopBound::undefined) {

			ASSERT(max >= 0);

			// Substract unrolling from loop bound
			for (BasicBlock *bb2 = UNROLLED_FROM(bb); bb2; bb2 = UNROLLED_FROM(bb2))
				max--;

			// Set execution count to 0 for each unrolled iteration that cannot be taken
			if (max < 0) {
				BasicBlock *bb2 = bb;
				string label;
				if(_explicit)
					label = _ << "unrolled loop constraint for BB" << INDEX(bb) << "/" << cfg->label();
				otawa::ilp::Constraint *cons0 = system->newConstraint(label, otawa::ilp::Constraint::EQ);
				for(int i = 0; i < (-max); i++) {
					for(BasicBlock::InIterator edge(bb); edge; edge++) {
						ASSERT(edge->source());
						otawa::ilp::Var *var  = VAR(edge);
						cons0->addLeft(1, var);
					}
					bb2 = UNROLLED_FROM(bb2);
				}
			}

			// sum{(i,h) / h dom i} eih <= max * sum{(i, h) / not h dom x} xeih
			string label;
			if(_explicit)
				label = _ << "loop constraint on BB" << INDEX(bb) << "/" << cfg->label();
			otawa::ilp::Constraint *cons = system->newConstraint(label, otawa::ilp::Constraint::LE);
			for(BasicBlock::InIterator edge(bb); edge; edge++) {
				ASSERT(edge->source());
				otawa::ilp::Var *var = VAR(edge);
				if(Dominance::dominates(bb, edge->source()))
					cons->addLeft(1, var);
				else
					cons->addRight((max < 0) ? 0 : max, var);
			}

		}

		// constraint for MIN_ITERATION
		if(min != ContextualLoopBound::undefined) {
			ASSERT(min >= 0);

			// Substract unrolling from loop bound
			for (BasicBlock *bb2 = UNROLLED_FROM(bb); bb2; bb2 = UNROLLED_FROM(bb2))
				min--;

			// Set execution count to 0 for each unrolled iteration that cannot be taken
			if (min < 0) {
				BasicBlock *bb2 = bb;
				string label;
				if(_explicit)
					label = _ << "unrolled loop constraint for BB" << INDEX(bb) << "/" << cfg->label();
				otawa::ilp::Constraint *cons0 = system->newConstraint(label, otawa::ilp::Constraint::EQ);
				for(int i = 0; i < (-min); i++) {
					for(BasicBlock::InIterator edge(bb); edge; edge++) {
						ASSERT(edge->source());
						otawa::ilp::Var *var  = VAR(edge);
						cons0->addLeft(1, var);
					}
					bb2 = UNROLLED_FROM(bb2);
				}
			}

			// sum{(i,h) / h dom i} eih >= min * sum{(i, h) / not h dom x} xeih
			string label;
			if(_explicit)
				label = _ << "loop constraint on BB" << INDEX(bb) << "/" << cfg->label();
			otawa::ilp::Constraint *cons = system->newConstraint(label, otawa::ilp::Constraint::GE);
			for(BasicBlock::InIterator edge(bb); edge; edge++) {
				ASSERT(edge->source());
				otawa::ilp::Var *var = VAR(edge);
				if(Dominance::dominates(bb, edge->source()))
					cons->addLeft(1, var);
				else
					cons->addRight((min < 0) ? 0 : min, var);
			}

		}

		// constraint for TOTAL_ITERATION
		// sum {h dom i} eih + sum {u in unrolled(h))} eiu <= total
		// ensure that the back-edges does not increase alone
		// sum {h dom i} eih <= total * sum {i dom h} eih + (total - 1) sum {u in unrolled(h))} eiu)
		if(total != ContextualLoopBound::undefined) {

			// build constraints
			string label;
			if(_explicit)
				label = _ << "total loop constraint for BB" << INDEX(bb) << "/" << cfg->label();
			otawa::ilp::Constraint *cons = system->newConstraint(label, otawa::ilp::Constraint::LE);
			label = "";
			if(_explicit)
				label = _ << "0-execution for total loop constraint for BB" << INDEX(bb) << "/" << cfg->label();
			otawa::ilp::Constraint *zero = system->newConstraint(label, otawa::ilp::Constraint::LE);

			// 0 <= total
			// 0 <= 0
			cons->addRight(total);

			// sum {h dom i} eih <= total
			// sum {h dom i} eih <= total * sum {(i, h) in V & h -dom i} eih
			for(BasicBlock::InIterator edge(bb); edge; edge++) {
				ASSERT(edge->source());
				if(Dominance::dominates(bb, edge->source())) {
					cons->addLeft(1, VAR(edge));
					zero->addLeft(1, VAR(edge));
				}
				else if(!Dominance::dominates(bb, edge->source()))
					zero->addRight(total, VAR(edge));
			}

			// sum{h dom i} eih + sum {u in unrolled(h))} eiu <= total
			// sum {h dom i} eih <= total * sum {(i, h) in V & h -dom i} eih
			for(BasicBlock *hd = UNROLLED_FROM(bb); hd; hd = UNROLLED_FROM(hd))
				for(BasicBlock::InIterator edge(hd); edge; edge++)
					if(Dominance::dominates(edge->source(), hd))
						cons->addLeft(1, VAR(edge));
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
 */
void FlowFactConstraintBuilder::configure(const PropList& props) {
	ContextualProcessor::configure(props);
	_explicit = EXPLICIT(props);
}


/**
 * This feature asserts that constraints tied to the flow fact information
 * has been added to the ILP system.
 */
Feature<FlowFactConstraintBuilder>
	FLOW_FACTS_CONSTRAINTS_FEATURE("otawa::ipet::FLOW_FACTS_CONSTRAINTS_FEATURE");

}

} // otawa::ipet
