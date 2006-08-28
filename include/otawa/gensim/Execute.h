#ifndef _EXECUTE_H_
#define _EXECUTE_H_

#include <systemc.h>
#include <otawa/gensim/SimulatedInstruction.h>
#include <otawa/gensim/PipelineStage.h>

namespace otawa {
	class GenericState;
}


class ExecuteInOrderStageIQ : public PipelineStage {
	public:
		// signals
		sc_in<bool> in_clock;
		sc_in<SimulatedInstruction *> * in_instruction;
		sc_in<int> in_number_of_ins;
		sc_out<int> out_number_of_accepted_ins;
		
	private:
		// variables
		int stage_width;
		otawa::GenericState * sim_state;
		elm::genstruct::AllocatedTable<rename_table_t> * rename_tables;
		
	public:
		ExecuteInOrderStageIQ(sc_module_name name, int width, otawa::GenericState * gen_state,
								elm::genstruct::AllocatedTable<rename_table_t> * _rename_tables);
		
		SC_HAS_PROCESS(ExecuteInOrderStageIQ);
		void action();
};

class ExecuteOOOStage : public PipelineStage {
	public:
		// signals
		sc_in<bool> in_clock;
		
	private:
		// variables
		int stage_width;
		elm::genstruct::AllocatedTable<rename_table_t> * rename_tables;
		InstructionQueue * rob;
		
	public:
		ExecuteOOOStage(sc_module_name name, int width, InstructionQueue * _rob,
								elm::genstruct::AllocatedTable<rename_table_t> * _rename_tables);
		
		SC_HAS_PROCESS(ExecuteOOOStage);
		void action();
};

class CommitStage : public PipelineStage {
	public:
		// signals
		sc_in<bool> in_clock;
		sc_in<SimulatedInstruction *> * in_instruction;
		sc_in<int> in_number_of_ins;
		sc_out<int> out_number_of_accepted_ins;
		
	private:
		// variables
		int width;
		otawa::GenericState * sim_state;
		
	
	public:
		CommitStage(sc_module_name name, int _width, otawa::GenericState * gen_state);
		
		SC_HAS_PROCESS(CommitStage);
		void action();
};



#endif //_EXECUTE_H_
