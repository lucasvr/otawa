/*
 *	$Id$
 *	Copyright (c) 2005, IRIT UPS.
 *
 *	ExeGraphBBTime.h -- ExeGraphBBTime class interface.
 */
#ifndef PAR_EXEGRAPH_BBTIME_H
#define PAR_EXEGRAPH_BBTIME_H

#include <otawa/ipet.h>
#include <otawa/prop/Identifier.h>
#include <otawa/cfg.h>
#include <otawa/util/LBlockBuilder.h>
#include <otawa/cache/categorisation/CATBuilder.h>
#include <otawa/hard/Memory.h>
#include <otawa/parexegraph/ParExeGraph.h>

namespace otawa {
  extern Identifier<String> GRAPHS_DIRECTORY;
  extern Identifier<bool> COMPRESSED_CODE;
  
  using namespace elm::genstruct; 
    
  class ParExeGraphBBTime: public BBProcessor {
    private:
    WorkSpace *_ws;
    PropList* _props;
    ParExeProc *_microprocessor;
    
    int _prologue_depth;
    OutStream *_output_stream;
    elm::io::Output *_output;
    String _graphs_dir_name;
    bool _compressed_code;
    public:
    ParExeGraphBBTime(const PropList& props = PropList::EMPTY);
    void processWorkSpace(WorkSpace *ws);
    void processBB(WorkSpace *ws, CFG *cfg, BasicBlock *bb);
    int getLatency(Address address);
    void buildPrologueList(BasicBlock * bb,
			   ParExeSequence * prologue, 
			   int capacity, 
			   elm::genstruct::DLList<ParExeSequence *> * prologue_list,
			   int depth);
    
  };
  
 
 
} //otawa

#endif // PAR_EXEGRAPH_BBTIME_H
