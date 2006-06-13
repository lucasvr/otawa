/*
 *	$Id$
 *	Copyright (c) 2006, IRIT-UPS <casse@irit.fr>.
 *
 *	prog/sim_Simulator.h -- Simulator class implementation.
 */

#include <otawa/sim/Simulator.h>

using namespace elm;

namespace otawa { namespace sim {

/**
 * @class Simulator
 * This class represents a plugin to use facilities provided by a simulator.
 * Before simulation, this class allows to instantiate a simulator for the
 * current framework and the result, a @ref otawa::sim::State_t object, provides
 * services to drive the simulation.
 */


/**
 * Name of the hook used for simulator plugger.
 */
const CString Simulator::PLUGGER_NAME = "sim_plugin";


/**
 * Version of the current plugger.
 */
const Version Simulator::PLUGGER_VERSION(1, 0, 0);


/**
 * Build a new simulator plugin.
 * @param name			Name of the plugin.
 * @param version		Version of the plug-in.
 * @param description	Plugin description.
 * @param license		License applied to the use of this plugin.
 */
Simulator::Simulator(elm::String name, const elm::Version& version,
const elm::CString description, const elm::CString license)
: Plugin(name, PLUGGER_VERSION, PLUGGER_NAME) {
	_plugin_version = version;
	_description = description;
	_licence = license;
}


/**
 * @fn State *Simulator::instantiate(FrameWork *fw);
 * This method instantiate a new simulator using the given framework. It must
 * be implemented by actual simulator plugins.
 * @param fw	Framework to use.
 * @return		The instantiated simulator.
 * @throw otawa::sim::Exception	If there is an error.
 */


/**
 * @class Exception
 * Exception generated by a simulator.
 */


/**
 * Build the message with plug-in information.
 * @param sim		Current simulator.
 * @param message	Error message.
 */
elm::String Exception::header(const Simulator& sim, const CString message) {
	StringBuffer buf;
	buf << message
		<< " [FROM " << sim.name() << " V" << sim.pluginVersion() << "]";
	return buf.toString();
}


/**
 * Build an exception with a message.
 * @param sim		Current simulator.
 * @param message	Message to display.
 */
 Exception::Exception(const Simulator& sim, String message) {
 	setMessage(header(sim, message.toCString()));
 }
 
 
/**
 * Build an exception with a formatted message.
 * @param sim		Current simulator.
 * @param format	Message format (C-like escapes).
 * @param args		Message arguments.
 */
Exception::Exception(const Simulator& sim, elm::CString format,
elm::VarArg &args) {
	String formatp = header(sim, format);
	build(formatp.toCString(), args);
}


/**
 * Build an exception with a formatted message.
 * @param sim		Current simulator.
 * @param format	Message format (C-like escapes).
 * @param ...		Message arguments.
 */
Exception::Exception(const Simulator& sim, elm::CString format, ...) {
	VARARG_BEGIN(args, format)
	String formatp = header(sim, format);
	build(formatp.toCString(), args);	
	VARARG_END
}

} } // otawa::sim

