#ifndef _ARMSIMULATOR_H_
#define _ARMSIMULATOR_H_

/*
 *	$Id$
 *	Copyright (c) 2006, IRIT-UPS <casse@irit.fr>.
 *
 *	otawa/sim/TrivialSimulator.h -- TrivialSimulator class interface.
 */

#include <ARMProcessor.h>
#include <otawa/sim/Simulator.h>
#include <otawa/sim/State.h>
#include <otawa/otawa.h>


namespace otawa { 

// Configuration
extern GenericIdentifier<int> INSTRUCTION_TIME;

class Driver {
	public :
		virtual ~Driver(); 
		virtual Inst* nextInstruction()=0;
		virtual Inst* nextInstruction(bool branch_is_taken)=0;
		virtual void terminateInstruction(Inst* inst)=0;
		virtual sim::mode_t simMode()=0;
};
class DriverUntilAddr : public Driver {
	address_t end_addr;
	Inst *next;
	sim::mode_t sim_mode;
	public:
		inline DriverUntilAddr(Inst* start_inst, address_t addr);
		Inst *nextInstruction();
		Inst* nextInstruction(bool branch_is_taken);
		void terminateInstruction(Inst* inst);
		sim::mode_t simMode();
};
inline DriverUntilAddr::DriverUntilAddr(Inst* start_inst, address_t addr): 
	next(start_inst), end_addr(addr), sim_mode(sim::NORMAL) {
}
class DriverUntilEnd : public Driver {
	Inst *next;
	sim::mode_t sim_mode;
	public:
		inline DriverUntilEnd(Inst* start_inst);
		Inst *nextInstruction();
		Inst* nextInstruction(bool branch_is_taken);
		void terminateInstruction(Inst* inst);
		sim::mode_t simMode();
};
inline DriverUntilEnd::DriverUntilEnd(Inst* start_inst): 
	next(start_inst), sim_mode(sim::NORMAL) {
}


class ARMState: public sim::State {
	FrameWork *fw;
	int time;
	int _cycle;
	Inst* next;
	ARMProcessor* processor;	
	sim::mode_t sim_mode;
	
public:
	Driver *driver;

	ARMState(FrameWork *framework, int _time):
	fw(framework), time(_time), _cycle(0), driver(NULL) {
		next = fw->start();
	}
	
	ARMState(const ARMState& state):
	fw(state.fw), time(state.time), _cycle(state._cycle), next(state.next), driver(NULL) {
	}
	
	void init();
	
	// State overload
	State *clone(void) {
		return new ARMState(*this);	
	}
	
	
	sim::mode_t step(void);
	
	 sim::mode_t run(void) {
	 	driver = new DriverUntilEnd(next);
		while(/*driver->simMode() == sim::NORMAL*/ !processor->isEmpty())
			// mode = step(); ------------------------- pas besoin de retourner un mode ?
			step();
		delete driver;
		return sim::NORMAL; //------------------------- pas besoin de retourner un mode ?
	}
	
	 sim::mode_t runUntil(address_t addr) {
	 	driver = new DriverUntilAddr(next,addr);
		while(/*driver->simMode() == sim::NORMAL*/ !processor->isEmpty())
			// mode = step(); ------------------------- pas besoin de retourner un mode ?
			step();
		delete driver;
		return sim::NORMAL; //------------------------- pas besoin de retourner un mode ?
	}

	sim::mode_t runUntil(Inst *inst) {
		assert(inst);
		return runUntil(inst->address());
	}

	sim::mode_t runUntilBranch(void) {
	}
	
	sim::mode_t flush(void) {
		return sim::NORMAL;
	}
	
	int cycle(void) {
		return _cycle;
	}
	
	void reset(void) {
		_cycle = 0;
	}
	
	address_t getPC(void) {
		assert(next);
		return next->address();
	}

	Inst *pcInst(void) {
		return next;
	}

	void setPC(address_t pc);
	
	void setPC(Inst *inst) {
		assert(inst);
		next = inst;
	}
	
};

// ARMSimulator class
class ARMSimulator: public sim::Simulator {
public:
	ARMSimulator(void);
	
	// Simulator overload
	ARMState *instantiate(FrameWork *fw,
		const PropList& props = PropList::EMPTY);
};




} // otawa


#endif //_ARMSIMULATOR_H_
