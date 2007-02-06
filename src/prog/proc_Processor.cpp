/*
 *	$Id$
 *	Copyright (c) 2005, IRIT UPS.
 *
 *	src/prog/proc_Processor.cpp -- Processor class implementation.
 */

#include <otawa/proc/Processor.h>
#include <otawa/proc/Feature.h>

using namespace elm;
using namespace elm::io;

namespace otawa {

/**
 * @class Processor
 * The processor class is implemented by all code processor. At this level,
 * the argument can only be the full framework. Look at sub-classes for more
 * accurate processors.
 * 
 * The processor configuration accepts the following properties:
 * @ref PROC_OUTPUT, @ref PROC_VERBOSE, @ref PROC_STATS, @ref PROC_TIMED.
 * 
 * This processor may generate the following statistics: @ref PROC_RUNTIME.
 */


/**
 * Build a new processor with name and version.
 * @param name		Processor name.
 * @param version	Processor version.
 * @param props		Configuration properties.
 * @deprecated		Configuration must be passed at the process() call.
 */
Processor::Processor(elm::String name, elm::Version version,
const PropList& props): _name(name), _version(version), flags(0), stats(0) {
	init(props);
}

/**
 * Build a new processor with name and version.
 * @param name		Processor name.
 * @param version	Processor version.
 */
Processor::Processor(String name, Version version)
: _name(name), _version(version), flags(0), stats(0) {
}


/**
 * Build a new processor.
 * @param	Configuration properties.
 */
Processor::Processor(const PropList& props): flags(0), stats(0) {
	init(props);
}


/**
 */
void Processor::init(const PropList& props) {
	
	// Process output
	OutStream *out_stream = OUTPUT(props);
	if(out_stream)
		out.setStream(*out_stream);
		
	// Process statistics
	stats = STATS(props);
	if(stats) { 
		if(TIMED(props))
			flags |= IS_TIMED;
		else
			flags &= ~IS_TIMED;
	}
	
	// Process verbosity
	if(VERBOSE(props))
		flags |= IS_VERBOSE;
	else
		flags &= ~IS_VERBOSE;
}


/**
 * @fn void Processor::processFrameWork(FrameWork *fw);
 * Process the given framework.
 * @param fw	Framework to process.
 */


/**
 * This method may be called for configuring a processor thanks to information
 * passed in the property list.
 * @param props	Configuration information.
 */
void Processor::configure(const PropList& props) {
	init(props);
}


/**
 * Execute the code processor on the given framework.
 * @param fw	Framework to work on.
 * @param props	Configuration properties.
 */
void Processor::process(FrameWork *fw, const PropList& props) {
	
	// Check required feature
	for(int i = 0; i < required.length(); i++)
		if(!fw->isProvided(*required[i]))
			required[i]->process(fw, props);

	// Perform configuration
	configure(props);

	// Pre-processing actions
	if(isVerbose())
		out << "Starting " << name() << " (" << version() << ')' << io::endl;
	system::StopWatch swatch;
	if(isTimed())
		swatch.start();
	
	// Launch the work
	processFrameWork(fw);
	
	// Post-processing actions
	if(isVerbose())
		out << "Ending " << name();
	if(isTimed()) {
		swatch.stop();
		RUNTIME(*stats) = swatch.delay();
		if(isVerbose())
			out << " (" << (swatch.delay() / 1000) << "ms)" << io::endl;
	}
	if(isVerbose())
		out << io::endl;
	
	// Add provided features
	for(int i = 0; i < provided.length(); i++)
		fw->provide(*provided[i]);
}


/**
 * @fn bool Processor::isVerbose(void) const;
 * Test if the verbose mode is activated.
 * @return	True if the verbose is activated, false else.
 */


/**
 * @fn bool Processor::isTimed(void) const;
 * Test if the timed mode is activated (recording of timings in the statistics).
 * @return	True if timed mode is activated, false else.
 * @note	If statistics are not activated, this method returns ever false.
 */


/**
 * @fn bool Processor::recordsStats(void) const;
 * Test if the statictics mode is activated.
 * @return	True if the statistics mode is activated, false else.
 */


/**
 * This method is called before an anlysis to let the processor do some
 * initialization.
 * @param fw	Processed framework.
 */
void Processor::setup(FrameWork *fw) {
}


/**
 * This method is called after the end of the processor analysis to let it
 * do some clean up.
 */
void Processor::cleanup(FrameWork *fw) {
}


/**
 * Display a warning.
 * @param format	Format string a-la printf.
 * @param args		Format string arguments.
 */
void Processor::warn(CString format, VarArg args) {
	out << "WARNING:" << name() << ' ' << version() << ':';
	out.format(format, args);
	out << io::endl;
}


/**
 * Display a warning.
 * @param format	Format string a-la printf.
 * @param ...		Format string arguments.
 */
void Processor::warn(CString format, ...) {
	VARARG_BEGIN(args, format)
		warn(format, args);
	VARARG_END
}


/**
 * This property identifier is used for setting the output stream used by
 * the processor for writing messages (information, warning, error) to the user.
 */
Identifier<elm::io::OutStream *> Processor::OUTPUT("Processor::output", 0, otawa::NS);


/**
 * This property identifiers is used to pass a property list to the processor
 * that will be used to store statistics about the performed work. Implicitly,
 * passing such a property activates the statistics recording facilities.
 */
Identifier<PropList *> Processor::STATS("Processor::stats", 0, otawa::NS);


/**
 * If the value of the associated property is true (default to false), time
 * statistics will also be collected with other processor statistics. Passing
 * such a property without @ref PROC_STATS has no effects.
 */
Identifier<bool> Processor::TIMED("Processor::timed", false, otawa::NS);


/**
 * This property identifier is used to store in the statistics of a processor
 * the overall run time of the processor work.
 */
Identifier<elm::system::time_t> Processor::RUNTIME("Processor::runtime", 0, otawa::NS);


/**
 * This property activates the verbose mode of the processor: information about
 * the processor work will be displayed.
 */
Identifier<bool> Processor::VERBOSE("Processor::verbose", false, otawa::NS);


/**
 * This property selects the task entry point currently processed.
 */
//Identifier<elm::CString> Processor::ENTRY("Processor::entry", "main", otawa::NS);


/**
 * Usually called from a processor constructor, this method records a 
 * required feature for the work of the current processor.
 * @param feature	Required feature.
 */
void Processor::require(const AbstractFeature& feature) {
	required.add(&feature);
}


/**
 * Usually called from a processor constructor, this method records a feature
 * provided by the work of the current processor.
 * @param feature	Provided feature.
 */
void Processor::provide(const AbstractFeature& feature) {
	provided.add(&feature);
}


/**
 * @class NullProcessor
 * A simple processor that does nothing.
 */


/**
 */
NullProcessor::NullProcessor(void):
	Processor("otawa::NullProcessor", Version(1, 0, 0))
{
}


/**
 * @class NoProcessor class
 * A processor whise execution cause an exception throw. Useful for features
 * without default definition.
 */


/**
 */
void NoProcessor::processFrameWork(FrameWork *fw) {
	throw ProcessorException(*this, "this processor should not have been called");
}


/**
 */
NoProcessor::NoProcessor(void):
	Processor("otawa::NoProcessor", Version(1, 0, 0))
{
}

} // otawa
