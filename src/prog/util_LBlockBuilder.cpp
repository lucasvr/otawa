/*
 *	$Id$
 *	Copyright (c) 2005-07, IRIT UPS <casse@irit.fr>
 *
 *	LBlockBuilder class implementation
 *	This file is part of OTAWA
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
 *	along with Foobar; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <elm/assert.h>
#include <otawa/util/LBlockBuilder.h>
#include <otawa/proc/ProcessorException.h>
#include <otawa/hard/CacheConfiguration.h>
#include <otawa/hard/Platform.h>
#include <otawa/cfg.h>
#include <otawa/ilp.h>
#include <otawa/proc/ProcessorException.h>
#include <otawa/ipet/IPET.h>
#include <otawa/cfg/CFGCollector.h>
#include <elm/genstruct/Vector.h> 
#include <elm/genstruct/HashTable.h>
#include <elm/genstruct/Table.h>

namespace otawa {

/**
 * @class LBlockBuilder
 * This processor builds the list of l-blocks for each lines of instruction
 * cache and stores it in the CFG.
 * 
 * @par Required Features
 * @li @ref INVOLVED_CFGS_FEATURE
 * 
 * @par Provided Features
 * @li @ref COLLECTED_LBLOCKS_FEATURE
 */


/**
 * Build a new l-block builder.
 */
LBlockBuilder::LBlockBuilder(void)
: BBProcessor("otawa::util::LBlockBuilder", Version(1, 1, 0)) {
	require(COLLECTED_CFG_FEATURE);
	provide(COLLECTED_LBLOCKS_FEATURE);
}


/**
 */
void LBlockBuilder::setup(WorkSpace *fw) {
	ASSERT(fw);

	// Check the cache
	cache = fw->platform()->cache().instCache();
	if(!cache)
		throw ProcessorException(*this, "No cache in this platform.");

	// Build hash	
	cacheBlocks = new HashTable<int, int>();
	
	// Build the l-block sets
	lbsets = new LBlockSet *[cache->rowCount()];
	LBLOCKS(fw) = lbsets;
	for(int i = 0; i < cache->rowCount(); i++) {
		lbsets[i] = new LBlockSet(i);
		new LBlock(lbsets[i], 0, 0, 0, -1);
	}
}


/**
 */
void LBlockBuilder::cleanup(WorkSpace *fw) {
	ASSERT(fw);
	
	// Add end blocks
	for(int i = 0; i < cache->rowCount(); i++)
		new LBlock(lbsets[i], 0, 0, 0, -1);
	
	// Remove hash
	delete cacheBlocks;
}


/**
 * Add an lblock to the lblock lists.
 * @param bb		Basic block containing the l-block.
 * @param inst		First instruction of the l-block.
 * @param index		Index in the BB lblock table.
 * @paramlblocks	BB lblock table.
 */
void LBlockBuilder::addLBlock(
	BasicBlock *bb,
	Inst *inst,
	int& index,
	genstruct::AllocatedTable<LBlock*> *lblocks
) {
	
	// compute the cache block ID
	LBlockSet *lbset = lbsets[cache->line(inst->address())]; 
	int block = cache->block(inst->address());
	int cbid = cacheBlocks->get(block, -1);
	if(cbid == -1) {
    	cbid = lbset->newCacheBlockID();
    	cacheBlocks->put(block, cbid);
    }
	
	// Compute the size
	Address top = (inst->address() + cache->blockMask()) & ~cache->blockMask();
	if(top > bb->address() + bb->size())
		top = bb->address() + bb->size();
	
	// Build the lblock
	LBlock *lblock = new LBlock(
			lbset,
			inst->address(),
			bb,
			top - inst->address(),
			cbid
		);
	lblocks->set(index, lblock); 											
	index++;
}


/**
 */
void LBlockBuilder::processBB(WorkSpace *fw, CFG *cfg, BasicBlock *bb) {
	ASSERT(fw);
	ASSERT(cfg);
	ASSERT(bb);
	
	// Do not process entry and exit
	if (bb->isEnd())
		return;
		
	// Allocate the BB lblock table
	int num_lblocks =
		((bb->address() + bb->size() + cache->blockMask()) >> cache->blockBits())
		- (bb->address() >> cache->blockBits());
	genstruct::AllocatedTable<LBlock*> *lblocks =
		new genstruct::AllocatedTable<LBlock*>(num_lblocks);
	BB_LBLOCKS(bb) = lblocks;
		
	// Traverse instruction
	int index = 0;
	int block = -1;
	for(BasicBlock::InstIterator inst(bb); inst; inst++) {
		int new_block = cache->block(inst->address());
		if(!inst->isPseudo() && new_block != block) {
			addLBlock(bb, inst, index, lblocks);
			block = new_block;
		}
	}
	ASSERT(index == num_lblocks);
}


/**
 * This feature ensures that the L-blocks of the current task has been
 * collected.
 * 
 * @par Properties
 * @li @ref LBLOCKS
 * @li @ref BB_LBLOCKS
 */
Feature<LBlockBuilder> COLLECTED_LBLOCKS_FEATURE("otawa::COLLECTED_LBLOCKS_FEATURE");


} // otawa
