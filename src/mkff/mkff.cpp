/*
 *	$Id$
 *	mkff utility
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

#include <elm/io.h>
#include <elm/options.h>
#include <elm/genstruct/Vector.h>
#include <otawa/cfg.h>
#include <otawa/otawa.h>
#include <otawa/util/ContextTree.h>
#include <elm/checksum/Fletcher.h>
#include <elm/io/InFileStream.h>
#include <elm/system/Path.h>
#include <otawa/cfg/CFGCollector.h>
#include <otawa/util/FlowFactLoader.h>

using namespace elm;
using namespace otawa;

const char *noreturn_labels[] = {
	"_exit",
	"exit",
	0
};

const char *nocall_labels[] = {
	"__eabi",	// ppc-eabi-*
	"_main",	// tricore-*-*
	0
};


/**
 * @page mkff mkff Command
 * 
 * This command is used to generate F4 file template file template (usually
 * suffixed by a @e .ff) to pass flow facts
 * to OTAWA. Currently, only constant loop bounds are supported as flow facts.
 * Look the @ref f4 documentation for more details.
 * 
 * @par SYNTAX
 * @code
 * $ mkff binary_file function1 function2 ...
 * @endcode
 * 
 * mkff builds the .ff loop statements for each function calling sub-tree for
 * the given binary file. If no function name is given, only the main()
 * function is processed.
 * 
 * The loop statement are indented according their depth in the context tree
 * and displayed with the current syntax:
 * @code
 * // Function function_name
 * loop loop1_address ?;
 *  loop loop1_1_address ?;
 *    loop loop_1_1_address ?;
 * loop loop2_address ?;
 * @endcode
 * The "?" question marks must be replaced by the maximum loop bound in order
 * to get valid .ff files. A good way to achieve this task is to use the
 * @ref dumpcfg command to get  a graphical display of the CFG.
 * 
 * @par Example
 * @code
 * $ mkff fft1
 * // Function main
 * loop 0x100006c0 ?;
 * 
 * // Function fft1
 * loop 0x10000860 ?;
 * loop 0x10000920 ?;
 *  loop 0x10000994 ?;
 *    loop 0x10000a20 ?;
 * loop 0x10000bfc ?;
 *  loop 0x10000d18 ?;
 * loop 0x10000dc0 ?;
 *
 * // Function sin
 * loop 0x10000428 ?;
 * loop 0x1000045c ?;
 * loop 0x10000540 ?;
 * @endcode
 *
 * @par Other information
 * 
 * mkff has the ability to produce automatically other commands to handle
 * problematic or exotic flow fact structures:
 * @li false control instruction (branching to the next instruction to get
 * the PC values on some architecture),
 * @li undirect branches (switch-like construction, function pointer calls),
 * @li non-returning functions (like exit(), _exit()),
 * @li problematic initialization (like __eabi on EABI based platforms,
 * _main for tricore).
 * 
 * @par Usage
 * In very complex programs, it may be required to launch mkff several times.
 * 
 * As mkff may detect unsolved indirect branches (function pointer call or
 * swicth-like statements, the first phase consist to fill this kind
 * information and to relaunch mkff to scan unreachable parts of the program.
 * Possibly, some parts may also be cut to tune the WCET computation.
 * 
 * As an example, we want to build the flow facts of the program xxx.
 * -# generate a first version: @c{$ mkff xxx > xxx.ff},
 * -# if required, fix the non-loop directives and removes the loop directives,
 * -# generate a new version: @c{$ mkff xxx >> xxx.ff},
 * -# remove the duplicated directives and restart at (2).
 * 
 * In the second phase, you must fix the loop directives, that is, to replace
 * the question marks '?' by actual loop iteration bounds.
 */

// Marker for processed sub-programs
//static Identifier ID_Processed("mkff.processed");


/**
 * Find a name for the current CFG.
 * @param cfg	Current CFG.
 * @return		Matching name.
 */
inline string nameOf(CFG *cfg) {
	string label = cfg->label();
	if(!label)
		label = _ << "0x" << cfg->address();
	else
		label = "\"" + label + "\"";
	return label;
}


/**
 * Compute an address for an item, relative to the container CFG if
 * possible.
 * @param CFG		Container CFG.
 * @param address	Address of the item.
 * @return			String representing the address of the instruction in F4.
 */
inline string addressOf(CFG *cfg, Address address) {
	string label = cfg->label();
	if(!label)
		return _ << "0x" << address;
	long offset = address - cfg->address();
	StringBuffer buf;
	buf << '"' << label << '"';
	if(offset > 0)
		buf << " + 0x" << io::hex(offset);
	else
		buf << " - 0x" << io::hex(-offset);
	return buf.toString();
}


/**
 * Compute an address for an instruction, relative to the container CFG if
 * possible.
 * @param CFG	Container CFG.
 * @param inst	Instruction to get address of.
 * @return		String representing the address of the instruction in F4.
 */
inline string addressOf(CFG *cfg, Inst *inst) {
	return addressOf(cfg, inst->address());
}


/**
 * Make an address for F4 file.
 * @param cfg	Current CFG.
 * @param addr	Address of the item relative to the CFG.
 * @return		String representing the address.
 */
inline string makeAddress(CFG *cfg, Address addr) {
	string label = cfg->label();
	if(!label)
		return _ << "0x" << addr;
	else {
		int offset = cfg->address() - addr;
		StringBuffer buf;
		buf << label;
		if(offset) {
			if(offset >= 0)
				buf << '+';
			else
				buf << '-';
		}
		buf << io::hex(offset);
		return buf.toString();
	}
}


// Command class
class Command: public option::Manager {
public:
	Command(void);
	~Command(void);
	void run(int argc, char **argv);
	
	// Manager overload
	virtual void process (String arg);

protected:
	bool one;
	otawa::Manager manager;
	WorkSpace *fw;
	CFGInfo *info;
	genstruct::Vector<String> added;
	void perform(String name);
};


// ControlOutput processor
class ControlOutput: public CFGProcessor {
public:
	ControlOutput(void);
protected:
	virtual void setup(WorkSpace *ws);
	virtual void cleanup(WorkSpace *ws);
	virtual void processCFG(WorkSpace *ws, CFG *cfg);
private:
	void prepare(WorkSpace *ws, CFG *cfg);
	bool one;
};


// FFOutput processor
class FFOutput: public CFGProcessor {
public:
	FFOutput(void);
protected:
	virtual void processCFG(WorkSpace *ws, CFG *cfg);
private:
	void scanFun(ContextTree *ctree);
	void scanLoop(CFG *cfg, ContextTree *ctree, int indent);
};


// Options
static Command command;


/**
 * Perform the work, get the loop and outputting the flow fact file.
 * @param name	Name of the function to process.
 */
void Command::perform(String name) {
	assert(name);

	// Configuration	
	PropList props;
	//Processor::VERBOSE(props) = true;
	TASK_ENTRY(props) = &name;
	for(int i = 0; i < added.length(); i++)
		CFGCollector::ADDED_FUNCTION(props).add(added[i].toCString());

	// Build the checksums of the binary files
	for(Process::FileIter file(fw->process()); file; file++) {
		checksum::Fletcher sum;
		io::InFileStream stream(file->name());
		sum.put(stream);
		elm::system::Path path = file->name();
		cout << "checksum \"" << path.namePart()
			 << "\" 0x" << io::hex(sum.sum()) << ";\n";
	}
	cout << io::endl;

	// Display low-level flow facts
	ControlOutput ctrl;
	ctrl.process(fw, props);
	
	// Display the context tree
	FFOutput out;
	out.process(fw, props);
}


/**
 * Process the free arguments.
 * @param arg	Free param value.
 */
void Command::process(String arg) {

	// First free argument is binary path
	if(!fw) {
		PropList props;
		NO_SYSTEM(props) = true;
		fw = manager.load(arg.toCString(), props);
		info = fw->getCFGInfo();
	}
	
	// Process function names
	else if(!one) {
		one = true;
		perform(arg);	
	}
	else
		added.add(arg);
}


/**
 * Build the command.
 */
Command::Command(void): one(false), fw(0) {
	program = "mkff";
	version = "1.0";
	author = "Hugues Cassé";
	copyright = "Copyright (c) 2005-07, IRIT-UPS France";
	description = "Generate a flow fact file for an application.";
	free_argument_description = "program [function names...]";
}


/**
 * Run the command.
 * @param argc	Argument count.
 * @param argv	Argument vector.
 */
void Command::run(int argc, char **argv) {
	parse(argc, argv);
	if(!fw) {
		displayHelp();
		throw option::OptionException("no binary file to process");
	}
	else if(!one)
		process("main");
}


/**
 * Release all command ressources.
 */
Command::~Command(void) {
	delete fw;
}


/**
 * Display the flow facts.
 */
FFOutput::FFOutput(void): CFGProcessor("FFOutput", Version(1, 0, 0)) {
	require(CONTEXT_TREE_BY_CFG_FEATURE);
}


/**
 */
void FFOutput::processCFG(WorkSpace *ws, CFG *cfg) {
	ASSERT(ws);
	ASSERT(cfg);
	ContextTree *ctree = CONTEXT_TREE(cfg);
	ASSERT(ctree);
	scanFun(ctree);
}


/**
 * Process a function context tree node.
 * @param ctree	Function context tree node.
 */
void FFOutput::scanFun(ContextTree *ctree) {
	assert(ctree);
	
	// Display header
	bool display = false;
	for(Iterator<ContextTree *> child(ctree->children()); child; child++)
		if(child->kind() == ContextTree::LOOP) {
			display = true;
			break;
		}
	if(display) {
		String label = ctree->cfg()->label();
		if(!label)
			label = _ << "0x" << ctree->cfg()->address(); 
		cout << "// Function " << label << "\n";
	}

	// Scan the content
	scanLoop(ctree->cfg(), ctree, 0);		
	
	// Display footer
	if(display)
		cout << "\n";
}


/**
 * Scan a context tree for displaying its loop flow facts.
 * @param cfg		Container CFG.
 * @param ctree		Current context tree.
 * @param indent	Current indentation.
 */
void FFOutput::scanLoop(CFG *cfg, ContextTree *ctree, int indent) {
	assert(ctree);
	
	for(Iterator<ContextTree *> child(ctree->children()); child; child++) {
		ASSERT(child->kind() != ContextTree::FUNCTION);
		
		// Process loop
		if(child->kind() == ContextTree::LOOP) {
			for(int i = 0; i < indent; i++)
				cout << "  ";
			cout << "loop " << addressOf(cfg, child->bb()->address()) << " ?;\n"; 
			scanLoop(cfg, child, indent + 1);
		}
	}
}


/**
 * @class ControlOutput
 * Output information about the control.
 */


/**
 * Constructor.
 */
ControlOutput::ControlOutput(void)
: CFGProcessor("ControlOutput", Version(1, 0, 0)) {
}


/**
 */
void ControlOutput::setup(WorkSpace *ws) {
	one = false;
}


/**
 */
void ControlOutput::processCFG(WorkSpace *ws, CFG *cfg) {
	
	// Look for labels
	Inst *inst = ws->findInstAt(cfg->address());
	if(!PRESERVED(inst)) {
		string label = cfg->label();
		if(label) {
			for(int i = 0; noreturn_labels[i]; i++)
				if(label == noreturn_labels[i])
					out << "noreturn \"" << label << "\";\n";
			for(int i = 0; nocall_labels[i]; i++)
				if(label == nocall_labels[i])
					out << "nocall \"" << label << "\";\n";
		}
	}
	
	// Look in BB
	for(CFG::BBIterator bb(cfg); bb; bb++)
		for(BasicBlock::InstIterator inst(bb); inst; inst++)
			if(inst->isControl() && !PRESERVED(inst) && !inst->isReturn()) {
				
				// Undefined branch target
				if(!inst->target()) {
					prepare(ws, cfg);
					out << "multibranch " << addressOf(cfg, inst->address())
						<< " to ?;";
					if(inst->isCall())
						out << "\t// indirect call in "; 
					else
						out << "\t// switch-like branch in ";
					out << nameOf(cfg) << io::endl;
				}
				
				// call to next instruction
				else if(inst->isCall()
				&& inst->target()->address() == inst->topAddress()) {
					prepare(ws, cfg);
					out << "ignorecontrol " << addressOf(cfg, inst)
						<< ";\t// " << nameOf(cfg) << " function\n"; 					
				}
			}
}


/**
 * Prepare the displaty of some information.
 * @param ws		Current workspace.
 * @param cfg		Current CFG.
 */
void ControlOutput::prepare(WorkSpace *ws, CFG *cfg) {
	if(!one) {
		out << "// Low-level flow facts\n";
		one = true;
	}
}


/**
 */
void ControlOutput::cleanup(WorkSpace *ws) {
	out << io::endl;
}


/**
 * "dumpcfg" entry point.
 * @param argc		Argument count.
 * @param argv		Argument list.
 * @return		0 for success, >0 for error.
 */
int main(int argc, char **argv) {
	try {
		command.run(argc, argv);
		return 0;
	}
	catch(option::OptionException& e) {
		cerr << "ERROR: " << e.message() << io::endl;
		command.displayHelp();
		return 1;
	}
	catch(elm::Exception& e) {
		cerr << "ERROR: " << e.message() << '\n';
		return 2;
	}
}

