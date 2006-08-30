#ifndef IPET_BBTIMESIMULATOR_H
#define IPET_BBTIMESIMULATOR_H

#include <otawa/cfg.h>
#include <otawa/proc/BBProcessor.h>

namespace otawa { namespace ipet {

class BBTimeSimulator : public BBProcessor{
public:
	BBTimeSimulator(const PropList& props = PropList::EMPTY);
	void processBB(FrameWork *fw, CFG *cfg, BasicBlock *bb);
};

} }

#endif /*IPET_BBTIMESIMULATOR_H*/
