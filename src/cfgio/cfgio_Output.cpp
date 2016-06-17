/*
 *	cfgio::Output class implementation
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2013, IRIT UPS.
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
#include <elm/io/OutFileStream.h>
#include <elm/xom/Serializer.h>
#include <otawa/cfgio/Output.h>
#include <otawa/proc/ProcessorPlugin.h>
#include <otawa/prop/DynIdentifier.h>
#include <otawa/ipet/features.h>


namespace otawa { namespace cfgio {

using namespace elm;

p::declare Output::reg = p::init("otawa::cfgio::Output", Version(1, 0, 0))
	.maker<Output>()
	.base(BBProcessor::reg);


/**
 * Used in the configuration of otawa::cfgio::Output to select
 * which properties to include in the output. The argument is
 * the fully-qualified name of the property identifier.
 * @ingroup cfgio
 */
Identifier<cstring> INCLUDE("otawa::cfgio::INCLUDE", 0);


/**
 * USed as a configuration for otawa::cfgio::Output, disable
 * output of instructions in basic blocks.
 * @ingroup cfgio
 */
Identifier<bool> NO_INSTS("otawa::cfgio::NO_INSTS", false);


/**
 * Used to select the file path to output to.
 * If not defined, output is performed to standard output.
 */
Identifier<Path> OUTPUT("otawa::cfgio::OUTPUT");


/**
 * @defgroup cfgio	CFG Input / Output
 *
 * This plugin is responsible to perform input / output of the CFG collection.
 * This allows external work on the CFG provided by OTAWA. As a default, XML is provided
 * with files matching the DTD from ${OTAWA_HOME}/share/Otawa/dtd/cfg.dtd .
 *
 * You can use this module in a script or embed it in an executable using the following
 * @c otawa-config result:
 * @code
 * 	otawa-config --libs --rpath cfgio
 * @endcode
 */

/**
 * @class Output
 * Output the current CFG collection in XML matching the DTD ${OTAWA_HOME}/share/Otawa/dtd/cfg.dtd .
 *
 * @par Configuration
 * @li @ref INCLUDE -- include the identifier whose name is given in the output.
 * @li @ref OUTPUT -- path to output file to (if not defined, output to standard output).
 * @ingroup cfgio
 */

/**
 */
Output::Output(void): BBProcessor(reg), root(0), cfg_node(0), last_bb(0), all(false), no_insts(false) {
}


/**
 * Generate ID for a CFG.
 * @param cfg	CFG to generate ID for.
 * @return		ID of the CFG.
 */
string Output::id(CFG *cfg) {
	return _ << '_' << cfg->index();
}


/**
 * Generate ID for a BB.
 * @param bb	BB to generate ID for.
 * @return		ID for the BB.
 */
string Output::id(Block *bb) {
	string suff;
	return _ << '_' << bb->cfg()->index() << '-' << bb->index();
}


/**
 */
void Output::processCFG(WorkSpace *ws, CFG *cfg) {

	// build the CFG node
	cfg_node = new xom::Element("cfg");
	root->appendChild(cfg_node);
	string addr = _ << "0x" << cfg->address();
	string label = cfg->label();
	string num = _ << cfg->index();
	string _id = id(cfg);
	cfg_node->addAttribute(new xom::Attribute("id", &_id));
	cfg_node->addAttribute(new xom::Attribute("address", &addr));
	cfg_node->addAttribute(new xom::Attribute("label", &label));
	cfg_node->addAttribute(new xom::Attribute("number", &num));
	processProps(cfg_node, *cfg);

	// build the entry BB
	xom::Element *entry = new xom::Element("entry");
	cfg_node->appendChild(entry);
	string entry_id = id(cfg->entry());
	entry->addAttribute(new xom::Attribute("id", &entry_id));
	last_bb = cfg_node->getChildCount();

	// usual processing
	BBProcessor::processCFG(ws, cfg);

	// add the exit node
	xom::Element *exit = new xom::Element("exit");
	cfg_node->insertChild(exit, last_bb);
	string exit_id = id(cfg->exit());
	exit->addAttribute(new xom::Attribute("id", &exit_id));
}


/**
 */
void Output::processBB(WorkSpace *ws, CFG *cfg, Block *b) {

	// add the basic
	if(!b->isEnd()) {

		// make the BB element
		xom::Element *bb_node = new xom::Element("bb");
		string _id = id(b);
		string num = _ << b->index();
		cfg_node->insertChild(bb_node, last_bb++);
		bb_node->addAttribute(new xom::Attribute("id", &_id));
		bb_node->addAttribute(new xom::Attribute("number", &num));

		// basic block specialization
		if(b->isBasic()) {
			BasicBlock *bb = b->toBasic();
			string addr = _ << "0x" << bb->address();
			string size = _ << bb->size();
			bb_node->addAttribute(new xom::Attribute("address", &addr));
			bb_node->addAttribute(new xom::Attribute("size", &size));
		}
		else if(b->isSynth()) {
			CFG *cfg = b->toSynth()->callee();
			bb_node->addAttribute(new xom::Attribute("call", !cfg ? "" : &id(cfg)));
		}
		processProps(bb_node, *b);

		// make the list of instruction
		if(b->isBasic() && !no_insts)
			for(BasicBlock::InstIter inst= b->toBasic()->insts(); inst; inst++) {
				xom::Element *inst_node = new xom::Element("inst");
				bb_node->appendChild(inst_node);
				string addr = _ << "0x" << inst->address();
				inst_node->addAttribute(new xom::Attribute("address", &addr));
				Option<Pair<cstring, int> > line_info = ws->process()->getSourceLine(inst->address());
				if(line_info) {
					string line = _ << (*line_info).snd;
					inst_node->addAttribute(new xom::Attribute("file", (*line_info).fst));
					inst_node->addAttribute(new xom::Attribute("line", &line));
				}
			}

		// if provided, gives the time

	}

	// add the output edges
	for(Block::EdgeIter edge = b->outs(); edge; edge++) {
		xom::Element *edge_node = new xom::Element("edge");
		cfg_node->appendChild(edge_node);
		string source = id(edge->source());
		edge_node->addAttribute(new xom::Attribute("source", &source));
		string target = id(edge->target());
		edge_node->addAttribute(new xom::Attribute("target", &target));
		processProps(edge_node, **edge);
	}
}


/**
 */
void Output::configure(const PropList& props) {
	BBProcessor::configure(props);

	// scan INCLUDE
	all = true;
	for(Identifier<cstring>::Getter name(props, INCLUDE); name; name++) {
		AbstractIdentifier *id = ProcessorPlugin::getIdentifier(&*name);
		if(!id)
			log << "WARNING: cannot find identifier " << *name << ". Output will ignore it!";
		else {
			ids.add(id);
			all = false;
		}
	}

	// other options
	path = cfgio::OUTPUT(props);
	no_insts = NO_INSTS(props);
}


/**
 */
void Output::processWorkSpace(WorkSpace *ws) {

	// initial log
	if(logFor(LOG_DEPS))
		for(avl::Set<const AbstractIdentifier *>::Iterator id(ids); id; id++)
			log << "\tproperty " << id->name() << " include in the output\n";

	// build the root node
	root = new xom::Element("cfg-collection");

	// normal processing
	BBProcessor::processWorkSpace(ws);

	// open output
	io::OutFileStream *file = 0;
	io::OutStream *out = &io::out;
	if(path) {
		file = new OutFileStream(path);
		if(!file->isReady()) {
			cstring msg = file->lastErrorMessage();
			delete file;
			throw ProcessorException(*this, _ << "cannot open \"" << path << "\": " << msg);
		}
		out = file;
	}

	// output the XML
	xom::Document doc(root);
	xom::Serializer serial(*out);
	serial.write(&doc);
	serial.flush();

	// close file if needed
	if(file)
		delete file;
}


/**
 * Output the properties.
 * @param parent	Parent element.
 * @param props		Properties to output.
 */
void Output::processProps(xom::Element *parent, const PropList& props) {
	for(PropList::Iter prop(props); prop; prop++)

		if((all/* && prop->id()->name()*/) || ids.contains(prop->id())) {

			// make node
			xom::Element *prop_node = new xom::Element("property");
			parent->appendChild(prop_node);

			// add identifier attribute
			prop_node->addAttribute(new xom::Attribute("identifier", &prop->id()->name()));

			// add value
			StringBuffer buf;
			prop->id()->print(buf, *prop);
			string s = buf.toString();
			prop_node->appendChild(&s);
		}
}


class Plugin: public ProcessorPlugin {
public:
	typedef genstruct::Table<AbstractRegistration * > procs_t;

	Plugin(void): ProcessorPlugin("otawa::cfgio", Version(1, 0, 0), OTAWA_PROC_VERSION) { }
	virtual procs_t& processors(void) const { return procs_t::EMPTY; };
};

} }	// otawa::cfgio

otawa::cfgio::Plugin otawa_cfgio;
ELM_PLUGIN(otawa_cfgio, OTAWA_PROC_HOOK);
