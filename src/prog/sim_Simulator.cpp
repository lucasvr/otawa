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
 * If supported by the simulator, enable or disable the functional part.
 * Default to false.
 */
Identifier<bool> IS_FUNCTIONAL("sim.is_functional");


/**
 * If supported by the simulator, enable or disable the structural part.
 * Default to true.
 */
Identifier<bool> IS_STRUCTURAL("sim.is_structural");


/**
 * Activate or unactivate the memory management.
 * Default to false.
 */
Identifier<bool> USE_MEMORY("sim.use_memory");


/**
 * Activate or unactivate the control management.
 * Default to false.
 */
Identifier<bool> USE_CONTROL("sim.use_control");


/**
 * @class Simulator
 * This class represents a plugin to use facilities provided by a simulator.
 * Before simulation, this class allows to instantiate a simulator for the
 * current framework and the result, a @ref otawa::sim::State_t object, provides
 * services to drive the simulation.
 */


/**
 * Build a new simulator plugin.
 * @param name				Name of the plugin.
 * @param version			Version of the plug-in.
 * @param plugger_version	Plugger required version.
 */
Simulator::Simulator(elm::CString name,
		const elm::Version& version,
		const elm::Version& plugger_version)
: Plugin(name, plugger_version, OTAWA_SIMULATOR_NAME) {
	_plugin_version = version;
}


/**
 * @fn State *Simulator::instantiate(FrameWork *fw, const PropList& props);
 * This method instantiate a new simulator using the given framework. It must
 * be implemented by actual simulator plugins.
 * @param fw		Framework to use.
 * @param props		Properties for tuning the built simulator.
 * @return			The instantiated simulator.
 * @throw otawa::sim::Exception	If there is an error.
 */


/**
 * @fn bool Simulator::accept(FrameWork *fw);
 * Test if the current simulator may execute programs in the given framework.
 * @param fw	Framework to execute program in.
 * @return		True if the simulator accept this framework, false else.
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
 Exception::Exception(const Simulator& sim, String& message) {
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

