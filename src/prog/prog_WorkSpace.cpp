/*
 *	$Id$
 *	Copyright (c) 2007, IRIT UPS.
 *
 *	WorkSpace class implementation
 */

#include <otawa/manager.h>
#include <otawa/prog/WorkSpace.h>
#include <otawa/ast/ASTInfo.h>
#include <otawa/ilp/System.h>
#include <otawa/cfg/CFGBuilder.h>
#include <config.h>
#include <elm/xom.h>


// Trace
//#define FRAMEWORK_TRACE
#if defined(NDEBUG) || !defined(FRAMEWORK_TRACE)
#	define TRACE(str)
#else
#	define TRACE(str) cerr << __FILE__ << ':' << __LINE__ << ": " << str << '\n';
#endif

namespace otawa {

/**
 * @class WorkSpace
 * A workspace represents a program, its run-time and all information about
 * WCET computation or any other analysis.
 */

	
/**
 * Build a new wokspace with the given process.
 * @param _proc	Process to use.
 */
WorkSpace::WorkSpace(Process *_proc): proc(_proc), featMap() {
	TRACE(this << ".WorkSpace::WorkSpace(" << _proc << ')');
	assert(_proc);
	addProps(*_proc);
	Manager *man = _proc->manager();
	man->frameworks.add(this);
	proc->link(this);
}


/**
 * Delete the workspace and the associated process.
 */
WorkSpace::~WorkSpace(void) {
	TRACE(this << ".WorkSpace::~WorkSpace()");
	clearProps();
	Manager *man = proc->manager();
	man->frameworks.remove(this);
	proc->unlink(this);
	delete proc;
}


/**
 * Get the CFG of the project. If it does not exists, built it.
 */
CFGInfo *WorkSpace::getCFGInfo(void) {
	
	// Already built ?
	CFGInfo *info = CFGInfo::ID(this);
	if(info)
		return info;
	
	// Build it
	CFGBuilder builder;
	builder.process(this);
	return CFGInfo::ID(this);
}


/**
 * Get the entry CFG of the program.
 * @return Entry CFG if any or null.
 */
CFG *WorkSpace::getStartCFG(void) {
	
	// Find the entry
	Inst *_start = start();
	if(!_start)
		return 0;
	
	// Get the CFG information
	CFGInfo *info = getCFGInfo();
	
	// Find CFG attached to the entry
	return info->findCFG(_start);
}


/**
 * Get the AST of the project. 
 */
ASTInfo *WorkSpace::getASTInfo(void) {
	return ASTInfo::getInfo(this);
}


/**
 */
const hard::CacheConfiguration& WorkSpace::cache(void) {
	return proc->cache();
}


/**
 * Build an ILP system with the default ILP engine.
 * @param max	True for a maximized system, false for a minimized.
 * @return		ILP system ready to use, NULL fi there is no support for ILP.
 */
ilp::System *WorkSpace::newILPSystem(bool max) {
	return manager()->newILPSystem();
}


/**
 * Load the given configuration in the process.
 * @param path				Path to the XML configuration file.
 * @throw LoadException		If the file cannot be found or if it does not match
 * the OTAWA XML type.
 */
void WorkSpace::loadConfig(const elm::system::Path& path) {
	xom::Builder builder;
	xom::Document *doc = builder.build(&path);
	if(!doc)
		throw LoadException(_ << "cannot load \"" << path << "\".");
	xom::Element *conf = doc->getRootElement();
	if(conf->getLocalName() != "otawa"
	|| conf->getNamespaceURI() != "")
		throw LoadException(_ << "bad file type in \"" << path << "\".");
	CONFIG_ELEMENT(this) = conf;
}


/**
 * Get the current configuration, if any, as an XML XOM element.
 * @return	Configuration XML element or null.
 */
xom::Element *WorkSpace::config(void) {
	xom::Element *conf = CONFIG_ELEMENT(this);
	if(!conf) {
		elm::system::Path path = CONFIG_PATH(this);
		if(path) {
			loadConfig(path);
			conf = CONFIG_ELEMENT(this);
		}
	}
	return conf;
}

/**
 * Get the dependency graph node associated with the feature and workspace
 * @param feature	Provided feature.
 */
FeatureDependency* WorkSpace::getGraph(const AbstractFeature* feature) {
	FeatureDependency *result = featMap.get(feature, NULL);
	ASSERT(result != NULL);
	return(result);
}

/**
 * Create a new dependency graph node associated with the feature and workspace
 * Replace a old deleted graph, if necessary.
 * @param feature	Provided feature.
 */
 void WorkSpace::newGraph(const AbstractFeature* feature) {
 	FeatureDependency *old = featMap.get(feature, NULL);
 	if (old) {
 		ASSERT(old->graph->isDeleted());
 		featMap.remove(feature);
 	}
 	featMap.put(feature, new FeatureDependency(feature));
}

/**
 * Delete the dependency graph node associated with the feature and workspace
 * It merely remove the item from the hashtable, it has to be freed by the user.
 * @param feature	Provided feature.
 */
 void WorkSpace::delGraph(const AbstractFeature* feature) {
 	FeatureDependency *old = featMap.get(feature, NULL);
 	ASSERT(old);
 	ASSERT(old->graph->isDeleted());
 	ASSERT(!old->isInUse());
 	delete old;
 	featMap.remove(feature);
}

/**
 * Tests if the feature has a dependency graph associated with it, in the context of the present workspace
 * @param feature	Provided feature.
 */
bool WorkSpace::hasGraph(const AbstractFeature* feature) {
	return (featMap.exists(feature));
}

/**
 * Record in the workspace that a feature is provided.
 * Also update the feature dependency graph
 * @param feature	Provided feature.
 */
void WorkSpace::provide(const AbstractFeature& feature, const Vector<const AbstractFeature*> *required) {
	if(!isProvided(feature)) {			
		if (!hasGraph(&feature) || getGraph(&feature)->graph->isDeleted()) 
			newGraph(&feature);
		
		if (required != NULL) {
			for (int j = 0; j < required->length(); j++) {
				if (isProvided(*required->get(j))) {
					getGraph(required->get(j))->graph->addChild(getGraph(&feature)->graph);
					getGraph(&feature)->incUseCount();
				}
			}
		}
		features.add(&feature);
	}
}

/**
 * Invalidate a feature (removing its dependancies) 
 * @param feature	Provided feature.
 */
void WorkSpace::invalidate(const AbstractFeature& feature) {
	if (isProvided(feature)) {
		for (genstruct::DAGNode<const AbstractFeature *>::Iterator dep(*getGraph(&feature)->graph); dep; dep++) {
			DAGNode<const AbstractFeature *> *node = *dep;
			invalidate(*node->useValue());
			getGraph(&feature)->graph->delChild(node);
			getGraph(node->useValue())->decUseCount();
			if (!getGraph(node->useValue())->isInUse())
				delGraph(node->useValue());
		}
		remove(feature);
	}
}



/**
 * Test if a feature is provided.
 * @param feature	Feature to test.
 * @return			True if it is provided, false else.
 */
bool WorkSpace::isProvided(const AbstractFeature& feature) {
	return features.contains(&feature);
}


/**
 * Remove a feature. It is usually called by processor whose actions remove
 * some properties from the workspace.
 * @param feature	Feature to remove.
 */
void WorkSpace::remove(const AbstractFeature& feature) {
	features.remove(&feature);
}


/**
 * Ensure that a feature is provided.
 * @param feature	Required feature.
 * @param props		Configuration properties (optional).
 */
void WorkSpace::require(const AbstractFeature& feature, const PropList& props) {
	if(!isProvided(feature)) {
		feature.process(this, props);
	}
}

} // otawa
