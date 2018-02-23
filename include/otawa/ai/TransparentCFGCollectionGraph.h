/*
 *	TransparentCFGCollectionGraph class interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2017, IRIT UPS.
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
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *	02110-1301  USA
 */
#ifndef OTAWA_AI_TRANSPARENTCFGCOLLECTIONGRAPH_H_
#define OTAWA_AI_TRANSPARENTCFGCOLLECTIONGRAPH_H_

#include <elm/data/List.h>
#include <otawa/cfg/interproc.h>
#include <otawa/properties.h>

namespace otawa { namespace ai {

class TransparentCFGCollectionGraph {
public:
	TransparentCFGCollectionGraph(const CFGCollection& coll): _coll(coll) { }

	typedef Block *vertex_t;
	typedef Edge *edge_t;
	typedef CFG::CallerIter Callers;

	inline vertex_t entry(void) const { return _coll.entry()->entry(); }
	inline vertex_t exit(void) const { return _coll.entry()->exit(); }
	inline vertex_t sinkOf(edge_t e) const { if(!e->sink()->isSynth()) return e->sink(); else return e->sink()->toSynth()->callee()->entry(); }
	inline vertex_t sourceOf(edge_t e) const { if(!e->source()->isSynth()) return e->source(); else return e->source()->toSynth()->callee()->exit(); }

	inline bool isEntry(vertex_t v) const { return v->isEntry(); }
	inline bool isExit(vertex_t v) const { return v->isExit(); }
	inline bool isCall(vertex_t v) const { return v->isSynth() && v->toSynth()->callee(); }
	inline vertex_t entryOf(vertex_t v) const { return v->toSynth()->callee()->entry(); }
	inline vertex_t exitOf(vertex_t v) const { return v->toSynth()->callee()->exit(); }
	inline Callers callers(vertex_t v) const { return v->cfg()->callers(); }

	class Iterator: public CFGCollection::BlockIter {
	public:
		inline Iterator(const TransparentCFGCollectionGraph& g): CFGCollection::BlockIter(&g._coll) { }
	};

	class Successor: public ip::Successor {
	public:
		Successor(Block *b);
	};
	inline Successor succs(vertex_t v) const { return Successor(v); }

	class Predecessor: public ip::Predecessor {
	public:
		Predecessor(Block *b);
	};
	inline Predecessor preds(vertex_t v) const { return Predecessor(v); }

	// Indexed concept
	inline int index(Block *v) const { return v->id(); }
	inline int count(void) const { return _coll.countBlocks(); }

private:
	const CFGCollection& _coll;
};

} }		// otawa::ai

#endif /* OTAWA_AI_TRANSPARENTCFGCOLLECTIONGRAPH_H_ */

