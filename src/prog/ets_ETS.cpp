/*
 *	$Id$
 *	Copyright (c) 2005, IRIT UPS.
 *
 *	src/ets_ETS.cpp -- ETS class implementation.
 */

 #include <otawa/ets/ETS.h>
 #include <otawa/prop/NameSpace.h>
 
namespace otawa { namespace ets {

/**
 * @class ETS
 * This static class is used for storing ressources (property identifiers)
 * used by ETS processors.
 */


/**
 * Namespace for ETS property identifiers.
 */
NameSpace NS("ets", otawa::NS);

/**
 * This identifier (int) is used for storing the number of iteration for each loop
 * of the program.
 */
Identifier<int> LOOP_COUNT("otawa::ets::loop_count", -1, NS);

/**
 * Identifier of annotation (int) used for storing the WCET value
 * in the ETS of the computed function.
 */
Identifier<int> WCET("otawa::ets::wcet", -1, NS);

/**
 * This identifier (AbstractCacheState *) is used for storing the abstract cache
 * line for each node.
 */
Identifier<AbstractCacheState *> ACS("otawa::ets::acs", 0, NS);

/**
 * This identifier (int) is used for storing the number of hit accesses for each
 * node.
 */
Identifier<int> HITS("otawa::ets::hits", -1, NS);

/**
 * This identifier (int) is used for storing the number of miss accesses for each
 * node.
 */
Identifier<int> MISSES("otawa::ets::misses", 0, NS);

/**
 * This identifier (int) is used for storing the number of first miss accesses
 * for each node.
 */
Identifier<int> FIRST_MISSES("otawa::ets::first_misses", 0, NS);

/**
 * This identifier (int) is used for storing the number of conflict accesses for
 * each node.
 */
Identifier<int> CONFLICTS("otawa::ets::conflicts", 0, NS);

} }// otawa::ets
