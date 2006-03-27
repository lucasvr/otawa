/*
 * $Id$
 * Copyright (c) 2005 IRIT-UPS
 *
 * src/prog/ContextTree.h -- ContextTree class implementation.
 */

#include <otawa/util/ContextTree.h>
#include <otawa/util/Dominance.h>
#include <elm/genstruct/Vector.h>
#include <elm/util/BitVector.h>
#include <otawa/cfg.h>
#include <otawa/util/DFAEngine.h>
#include <otawa/util/DFABitSet.h>

using namespace elm;
using namespace otawa::util;

namespace otawa {

/**
 * This class defines a DFA problem for detecting which loop or function call
 * contains a BB.
 * @par
 * For building the context tree, we use a DFA that works on a reversed CFG
 * using the following sets :
 * <dl>
 * 	<dt>LOOP</dt><dd>set of loop headers</dd>
 * 	<dt>ENTRY</dt><dd>set of inlined function entries</dd>
 * 	<dt>EXIT</dt><dd>set of inlined function exits</dd>
 * </dl>
 * @par
 * The gen set is built is a follow :
 * gen(n) =
 * 		if n in LOOP, { }
 * 		else if n in EXIT(m) { m }
 * 		else { m / (m, n) in m in LOOP }
 * @par
 * And the kill set is as follow :
 *		kill(n) = { n / n in LOOP U ENTRY }
 * @note This implemenation does not yet support virtual CFG.
 */
class ContextTreeProblem {
	CFG& _cfg;
	genstruct::Vector<BasicBlock *> hdrs;
public:
	
	ContextTreeProblem(CFG& cfg);
	inline DFABitSet *empty(void) const { return new DFABitSet(hdrs.length()); };
	DFABitSet *gen(BasicBlock *bb) const;
	DFABitSet *kill(BasicBlock *bb) const;
	inline int count(void) const { return hdrs.length(); };
	inline BasicBlock *get(int index) const { return hdrs[index]; };
	#ifndef NDEBUG
		void dump(elm::io::Output& out, DFABitSet *set);
	#endif
};


/* Dump the content of a bit set.
 */
#ifndef NDEBUG
void ContextTreeProblem::dump(elm::io::Output& out, DFABitSet *set) {
	bool first = true;
	cout << "{ ";
	for(int i = 0; i < hdrs.length(); i++)
		if(set->contains(i)) {
			if(first)
				first = false;
			else
				cout << ", ";
			cout << hdrs[i]->number();
		}
	cout << " }";
}
#endif

/**
 * Build a new context tree problem.
 * @param cfg	CFG which this problem is applied to.
 */
ContextTreeProblem::ContextTreeProblem(CFG& cfg): _cfg(cfg) {
	Dominance::ensure(&cfg);
	for(Iterator<BasicBlock *> bb(cfg.bbs()); bb; bb++)
		if(!bb->isEntry() && Dominance::isLoopHeader(bb))
			hdrs.add(bb);
}


/**
 * Compute the generation set for the given BB.
 * @param bb	Current basic block.
 * @return		Matching bit set.
 */
DFABitSet *ContextTreeProblem::gen(BasicBlock *bb) const {
	DFABitSet *result = empty();
	if(!Dominance::isLoopHeader(bb))
		for(BasicBlock::OutIterator edge(bb); edge; edge++) {
			//cout << edge->kind() << '\n';
			if(edge->kind() != Edge::CALL
			&& Dominance::dominates(edge->target(), bb))
				result->add(hdrs.indexOf(edge->target()));
		}
	return result;
}

/**
 * Compute the kill set for the given BB.
 * @param bb	Current basic block.
 * @return		Matching bit set.
 */
DFABitSet *ContextTreeProblem::kill(BasicBlock *bb) const {
	DFABitSet *result = empty();
	if(Dominance::isLoopHeader(bb))
			result->add(hdrs.indexOf(bb));
	return result;
}


/**
 * @class ContextTree
 * Representation of a context tree.
 */


/**
 * Annotations with this identifier are hooked to basic blocks and gives
 * the ower context tree (ContextTree * data).
 */
Identifier ContextTree::ID_ContextTree("otawa.context_tree");


/**
 * Build a new context tree for the given CFG.
 * @param cfg	CFG to build the context tree for.
 */
ContextTree::ContextTree(CFG *cfg): _kind(ROOT), _bb(cfg->entry()),
_parent(0), _cfg(cfg) {
	assert(cfg);
	//cout << "Computing " << cfg->label() << "\n";
	
	// Define the problem
	ContextTreeProblem prob(*cfg);
	if(prob.count() == 0) {
		for(Iterator<BasicBlock *> bb(cfg->bbs()); bb; bb++)
			addBB(bb);
		_bbs.merge(&cfg->bbs());		
		return;
	}
	//cout << "children = " << prob.count() << "\n";
	
	// Compute the solution
	DFAEngine<ContextTreeProblem, DFABitSet, DFASuccessor>
		dfa(prob, *cfg);
	dfa.compute();

	// Dump the DFA
	/*cout << "\nDFA\n";
	for(Iterator<BasicBlock *> bb(cfg->bbs()); bb; bb++) {
		cout << "BB " << bb->number() << " (" << bb->address() << ")";
		if(Dominance::isLoopHeader(bb))
			cout << " HEADER";
		cout << "\n\t gen = ";
		prob.dump(cout, dfa.genSet(bb));
		cout << "\n\t kill = ";
		prob.dump(cout, dfa.killSet(bb));
		cout << "\n\t in = ";
		prob.dump(cout, dfa.inSet(bb));
		cout << "\n\t out = ";
		prob.dump(cout, dfa.outSet(bb));
		cout << '\n';
	}*/
	
	// Prepare the tree analysis
	ContextTree *trees[prob.count()];
	BitVector *vecs[prob.count()];
	for(int i = 0; i < prob.count(); i++) {
		vecs[i] = &dfa.outSet(prob.get(i))->vector();
		//cout << "Child " << i << " " << *vecs[i];
		trees[i] = new ContextTree(prob.get(i), cfg);
		if(vecs[i]->isEmpty()) {
			addChild(trees[i]);
			//cout << " root child";
		}
		//cout << "\n";
		vecs[i]->set(i);
	}
	
	// Children find their parent
	for(int i = 0; i < prob.count(); i++) {
		vecs[i]->clear(i);
		//cout << "INITIAL " << i << " " << *vecs[i] << "\n";
		for(int j = 0; j < prob.count(); j++) {
			/*if(i != j)
				cout << "TEST " << *vecs[i] << " == " << *vecs[j] << "\n";*/
			if(i != j && *vecs[i] == *vecs[j]) {
				trees[j]->addChild(trees[i]);
				//cout << "FOUND !\n";
				break;
			}
		}
		assert(trees[i]->_parent);
		vecs[i]->set(i);
	}
	
	// BB find their parent
	for(Iterator<BasicBlock *> bb(cfg->bbs()); bb; bb++) {
		BitVector& bv = dfa.outSet(bb)->vector();
		if(bv.isEmpty())
			addBB(bb);
		else
			for(int i = 0; i < prob.count(); i++)
				if(vecs[i]->equals(bv)) {
					trees[i]->addBB(bb);
					break;
			}
	}
}


/**
 * Build the context tree of a loop.
 * @param bb	Header of the loop.
 */
ContextTree::ContextTree(BasicBlock *bb, CFG *cfg)
: _bb(bb), _kind(LOOP), _parent(0), _cfg(cfg) {
	assert(bb);
}


/**
 * Free the entire tree.
 */
ContextTree::~ContextTree(void) {
	for(int i = 0; i < _children.length(); i++)
		delete _children[i];
};


/**
 * Add the given basic block to the context tree.
 * @param bb	Added BB.
 */
void ContextTree::addBB(BasicBlock *bb) {
	_bbs.add(bb);
	if(bb->isCall())
		for(BasicBlock::OutIterator edge(bb); edge; edge++)
			if(edge->kind() == Edge::CALL && edge->calledCFG())
				addChild(new ContextTree(edge->calledCFG()));
}


/**
 * Add a children context tree.
 * @param child	Context tree to add.
 */
void ContextTree::addChild(ContextTree *child) {
	assert(child);
	_children.add(child);
	child->_parent = this;
	if(child->kind() == ROOT)
		child->_kind = FUNCTION;
}


/**
 * @fn BasicBlock *ContextTree::bb(void) const;
 * Get the BB that heads this tree (enter of functions for ROOT and FUNCTION,
 * header for LOOP).
 * @return	Header of the tree.
 */


/**
 * @fn kind_t ContextTree::kind(void) const;
 * Get the kind of the tree node.
 * @return	Tree node kind.
 */


/**
 * @fn ContextTree *ContextTree::parent(void) const;
 * Get the parent tree of the current one.
 * @return	Parent tree or null for root tree.
 */


/**
 * @fn elm::Collection<ContextTree *>& ContextTree::children(void);
 * Get the collection of children of the current tree.
 * @return	Collection of children.
 */


/**
 * @fn bool ContextTree::isChildOf(const ContextTree *ct);
 * Test transitively if the current context tree is a child of the given one.
 * @param ct	Parent context tree.
 * @return		True if the current context tree is a child of the given one.
 */


/**
 * @class ContextTree::ChildrenIterator
 * Iterator for the children of a context tree.
 */


/**
 * @fn ContextTree::ChildrenIterator::ChildrenIterator(ContextTree *tree);
 * Build a new iterator.
 * @param tree	Tree whose children are visited.
 */


/**
 * @fn bool ContextTree::ChildrenIterator::ended(void) const;
 * Test if the end of iteration is reached.
 * @return	True if the iteration is ended.
 */


/**
 * @fn ContextTree *ContextTree::ChildrenIterator::item(void) const;
 * Get the current item.
 * @return	Current context tree.
 */


/**
 * @fn void ContextTree::ChildrenIterator::next(void);
 * Go to the next children.
 */


/**
 * @fn CFG *ContextTree::ContextTree::cfg(void) const;
 * Get the CFG containing this context tree.
 * @return	Container CFG.
 */


/**
 * @fn elm::Collection<BasicBlock *>& ContextTree::bbs(void);
 * Get the basic blocks in the current context tree.
 * @return	Basic block collection.
 */

} // otawa
