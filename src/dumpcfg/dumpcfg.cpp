/*
 *	dumpcfg command implementation
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2004-08, IRIT UPS.
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
#include <elm/genstruct/DLList.h>
#include <elm/genstruct/SortedBinMap.h>
#include <elm/genstruct/DLList.h>
#include <elm/options.h>

#include <otawa/app/Application.h>
#include <otawa/manager.h>
#include <otawa/cfg/CFGCollector.h>
#include <otawa/util/FlowFactLoader.h>
#include <otawa/proc/DynProcessor.h>
#include <otawa/prog/features.h>

#include "SimpleDisplayer.h"
#include "DisassemblerDisplayer.h"
#include "DotDisplayer.h"
#include "MultipleDotDisplayer.h"

using namespace elm;
using namespace otawa;


/**
 * @addtogroup commands
 * @section dumpcfg dumpcfg Command
 *
 * This command is used to output the CFG of a binary program using different
 * kind of output.
 * @par SYNTAX
 * @code
 * > dumpcfg options binary_file functions1 function2 ...
 * @endcode
 * dumpcfg first loads the binary file then compute and display the CFG of the
 * functions whose name is given. If there is no function, the @e main functions
 * is dumped.
 *
 * Currently, dumpcfg provides the following outputs:
 *
 * @li -S (simple output): the basic blocks are displayed, one by one, with
 * their number, their address and the -1-ended list of their successors.
 * @code
 * !icrc1
 * # Inlining icrc1
 * 0 10000368 100003a4 1 -1
 * 1 100003a8 100003b0 3 2 -1
 * 2 100003b4 100003b4 4 -1
 * 3 100003b8 100003c8 6 5 -1
 * 4 1000040c 10000418 7 -1
 * 5 100003cc 100003e8 8 -1
 * 6 100003ec 100003f8 8 -1
 * 7 1000041c 10000428 9 -1
 * 8 100003fc 10000408 1 -1
 * @endcode
 *
 * @li -L (listing output): each basic block is displayed starting by "BB",
 * followed by its number, a colon and the list of its successors. Its
 * successors may be T(number) for a taken edge, NT(number) for a not-taken edge
 * and C(function) for a function call.
 * @code
 * # Function main
 * # Inlining main
 * BB 1:  C(icrc) NT(2)
 *        main:
 *               10000754 stwu r1,-32(r1)
 *               10000758 mfspr r0,256
 *               1000075c stw r31,28(r1)
 *               10000760 stw r0,36(r1)
 *					...
 * BB 2:  C(icrc) NT(3)
 *               1000079c or r0,r3,r3
 *               100007a0 or r9,r0,r0
 *               100007a4 sth r9,8(r31)
 *               100007a8 addis r9,r0,4097
 *               ...
 *  BB 3:  T(4)
 *               10000808 or r0,r3,r3
 *               1000080c or r9,r0,r0
 *               10000810 sth r9,10(r31)
 *               10000814 addi r3,r0,0
 *               10000818 b 1
 * BB 4:
 *               1000081c lwz r11,0(r1)
 *               10000820 lwz r0,4(r11)
 *               10000824 mtspr 256,r0
 *               10000828 lwz r31,-4(r11)
 *               1000082c or r1,r11,r11
 *               10000830 bclr 20,0
 *
 * @endcode
 *
 * @li -D (dot output): the CFG is output as a DOT graph description.
 * @image html dot.png
 *
 * @li -X or --xml (XML output): output a CFG as an XML file satisfying the DTD
 * from $(OTAWA_HOME/share/Otawa/data/cfg.dtd .
 *
 * dumpcfg accepts other options like:
 * @li -a -- dump all functions.
 * @li -d -- disassemble the machine code contained in each basic block,
 * @li -i -- inline the functions calls (recursive calls are reduced to loops),
 * @li -v -- verbose information about the work.
 */


/**
 */
Displayer::Displayer(cstring name, const Version version):
	Processor(name, version),
	display_assembly(false),
	source_info(false),
	display_all(false)
{
	require(otawa::COLLECTED_CFG_FEATURE);
}


/**
 * Select disassembling option.
 */
Identifier<bool> Displayer::DISASSEMBLE("", false);

/**
 * Select source information display.
 */
Identifier<bool> Displayer::SOURCE("", false);

/**
 * Display all CFGs.
 */
Identifier<bool> Displayer::ALL("", false);

/**
 * Display all CFGs.
 */
Identifier<string> Displayer::OUT("");

/**
 */
void Displayer::configure(const PropList& props) {
	Processor::configure(props);
	display_assembly = DISASSEMBLE(props);
	source_info = SOURCE(props);
	display_all = ALL(props);
	out = OUT(props);
}


// Displayers
SimpleDisplayer simple_displayer;
DisassemblerDisplayer disassembler_displayer;
DotDisplayer dot_displayer;
MultipleDotDisplayer mult_displayer;


// DumpCFG class
class DumpCFG: public Application {
public:

	DumpCFG(void);

	// options
	option::BoolOption remove_eabi;
	option::BoolOption all_functions;
	option::BoolOption inline_calls;
	option::BoolOption display_assembly;
	option::BoolOption simple;
	option::BoolOption disassemble;
	option::BoolOption dot;
	option::SwitchOption source, xml, all, virt, mult;
	option::ValueOption<string> out;

	Displayer *displayer;

protected:
	virtual void work(const string& entry, PropList &props) throw(elm::Exception) { dump(entry, props); }
	virtual void prepare(PropList &props);

private:
	void dump(CFG *cfg, PropList& props);
	void dump(const string& name, PropList& props);

};


/**
 */
void DumpCFG::prepare(PropList &props) {
	if(simple)
		displayer = &simple_displayer;
	else if(disassemble)
		displayer = &disassembler_displayer;
	else if(dot)
		displayer = &dot_displayer;
	else if(mult)
		displayer = &mult_displayer;
}


/**
 * Build the command.
 */
DumpCFG::DumpCFG(void):
	Application(
		"DumpCFG",
		Version(2, 0, 0),
		"Dump to the standard output the CFG of functions."
			"If no function name is given, the main function is dumped.",
		"Hugues Casse <casse@irit.fr",
		"Copyright (c) 2016, IRIT-UPS France"
	),

	remove_eabi(*this, 'r', "remove", "Remove __eabi function call, if available.", false),
	all_functions(*this, 'a', "all", "Dump all functions.", false),
	inline_calls(*this, 'i', "inline", "Inline the function calls.", false),
	display_assembly(*this, 'd', "display", "Display assembly instructions.", false),
	simple(*this, 'S', "simple", "Select simple output (default).", false),
	disassemble(*this, 'L', "list", "Select listing output.", false),
	dot(*this, 'D', "dot", "Select DOT output.", false),
	source(*this, option::short_cmd, 's', option::cmd, "--source", option::description, "enable source debugging information output", option::def, false, option::end),
	xml(option::SwitchOption::Make(*this).cmd("-x").cmd("--xml").description("output the CFG as an XML file")),
	all(option::SwitchOption::Make(*this).cmd("-R").cmd("--recursive").description("display the current and called CFGs")),
	virt(option::SwitchOption::Make(*this).cmd("-V").cmd("--virtualize").description("virtualize the called CFG, i.e. duplicate them at each call site")),
	mult(option::SwitchOption::Make(*this).cmd("-M").cmd("--multiple-dot").description("output multiple .dot file (one for each CFG) linked with URLs")),
	out(option::ValueOption<string>::Make(*this).cmd("-o").cmd("--output").description("select the output file or directory (-M option)")),

	displayer(&simple_displayer)
{
}


/**
 * Process the given CFG, that is, build the sorted list of BB in the CFG and then display it.
 * @param name	Name of the function to process.
 */
void DumpCFG::dump(const string& name, PropList& props) {
	/*require(COLLECTED_CFG_FEATURE);
	const CFGCollection& coll = **INVOLVED_CFGS(workspace());
	CFG *current_inline = 0;
	WorkSpace *my_ws = new WorkSpace(workspace());
	ENTRY_CFG(my_ws) = cfg;*/

	// if required, build the delayed
	/* TODO
	if(workspace()->isProvided(DELAYED_FEATURE)
	|| workspace()->isProvided(DELAYED2_FEATURE))
		require(DELAYED_CFG_FEATURE);*/

	// if required, virtualize
	/*	TODO
	 if(inline_calls)
		require(VIRTUALIZED_CFG_FEATURE);*/

	// get the CFG
	require(COLLECTED_CFG_FEATURE);
	if(virt)
		require(VIRTUALIZED_CFG_FEATURE);
	//const CFGCollection *coll = INVOLVED_CFGS(workspace());
	//CFG *vcfg = (*coll)[0];

	// set options
	if(display_assembly)
		Displayer::DISASSEMBLE(props) = display_assembly;
	if(source)
		Displayer::SOURCE(props) = source;
	if(all)
		Displayer::ALL(props) = all;
	if(out)
		Displayer::OUT(props) = out;

	// XML case (will become the generic case)
	/*if(xml) {		TODO
		//XMLDisplayer dis;
		DynProcessor dis("otawa::cfgio::Output");
		dis.process(my_ws);
	}

	// Dump the CFG
	else {*/
	displayer->process(workspace(), props);
	//}
}


/**
 * "dumpcfg" entry point.
 * @param argc		Argument count.
 * @param argv		Argument list.
 * @return		0 for success, >0 for error.
 */
int main(int argc, char **argv) {
	DumpCFG dump;
	return dump.run(argc, argv);
}
