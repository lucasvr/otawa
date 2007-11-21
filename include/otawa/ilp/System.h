/*
 *	$Id$
 *	System class implementation
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
#ifndef OTAWA_ILP_SYSTEM_H
#define OTAWA_ILP_SYSTEM_H

#include <elm/io.h>
#include <elm/string.h>
#include <otawa/ilp/Constraint.h>

namespace otawa { namespace ilp {

using namespace elm;

// System class
class System {
public:
	virtual Constraint *newConstraint(Constraint::comparator_t comp,
		double constant = 0) = 0;
	virtual Constraint *newConstraint(const string& label,
		Constraint::comparator_t comp, double constant = 0);
	virtual void addObjectFunction(double coef, Var *var = 0) = 0;
	virtual Var *newVar(const string& name) = 0;
	inline Var *newVar(cstring name = "") { return newVar(name); }
	
	virtual bool solve(void) = 0;
	virtual int countVars(void) = 0;
	virtual int countConstraints(void) = 0;
	virtual double valueOf(Var *var) = 0;
	virtual double value(void) = 0;
	
	virtual void exportLP(io::Output& out = elm::cout) = 0;
	virtual void dumpSystem(io::Output& out = elm::cout);
	virtual void dumpSolution(io::Output& out = elm::cout) = 0;
	virtual void dump(elm::io::OutStream& out = elm::io::stdout);
};
	
} }	// otawa::ilp

#endif // OTAWA_ILP_SYSTEM_H
