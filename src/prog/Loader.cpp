/*
 *	$Id$
 *	Copyright (c) 2003, IRIT UPS.
 *
 *	src/prog/Loader.cpp -- implementation for Loader class.
 */

#include <otawa/prog/Loader.h>

namespace otawa {

/**
 * @class Loader
 * This interface is implemented by all objects that may build and provide
 * a process. Many kind of loader may exists from the simple binary
 * loader to the complex program builder.
 */

/**
 * @fn Loader::~Loader(void)
 * Virtual destructor for destruction customization.
 */

/**
 * @fn CString Loader::getName(void) const
 * Get the name of the loader.
 * @return	Name of the loader.
 */

/**
 * @fn Process *Loader::load(Manager *man, CString path, PropList& props)
 * Load the file matching the given path with the given properties. The exact
 * type of the file and of the properties depends upon the underlying loader.
 * @param man		Caller manager.
 * @param path		Path to the file to load.
 * @param props	Property for loading.
 * @return Process containing the loaded file.
 * @exception LoadException							Error during the load.
 * @exception UnsupportedPlatformException	Loader does not handle
 * this platform.
 */

/**
 * @fn Process *Loader::create(Manager *man, PropList& props)
 * Build a new empty process matching the given properties.
 * @param man		Caller manager.
 * @param props	Properties describing the process.
 * @return Created process.
 * @exception UnsupportedPlatformException	Loader does not handle
 */


/**
 * Identifier of the property indicating the name (CString) of the platform to use.
 */	
GenericIdentifier<CString> Loader::ID_PlatformName("otawa.PlatformName");

/**
 * Identifier of the property indicating a name (CString) of the loader to use..
 */	
GenericIdentifier<CString> Loader::ID_LoaderName("otawa.LoaderName");

/**
 * Identifier of the property indicating a platform (Platform *) to use.
 */	
GenericIdentifier<hard::Platform *> Loader::ID_Platform("otawa.Platform");

/**
 * Identifier of the property indicating the loader to use.
 */	
GenericIdentifier<Loader *> Loader::ID_Loader("otawa.Loader");

/**
 * Identifier of the property indicating the identifier (PlatformId) of the loader to use.
 */	
GenericIdentifier<hard::Platform::Identification *> Loader::ID_PlatformId("otawa.PlatformId");

/**
 * Argument count as passed to the program (int).
 */	
GenericIdentifier<int> Loader::ID_Argc("otawa.Argc");

/**
 * Argument values as passed to the program (char **).
 */	
GenericIdentifier<char **> Loader::ID_Argv("otawa.Argv");

/**
 * Argument values as passed to the program (char **).
 */	
GenericIdentifier<char **> Loader::ID_Envp("otawa.Envp");


/**
 * Name of the "Heptane" loader.
 */
CString Loader::LOADER_NAME_Heptane = "heptane";

/**
 * Name of the "GLISS PowerPC" loader.
 */
CString Loader::LOADER_NAME_Gliss_PowerPC = "gliss-powerpc";

/**
 * Name of the default PowerPC platform.
 */
CString Loader::PLATFORM_NAME_PowerPC_Gliss = "powerpc/ppc601-elf/gliss-gliss";
	
} // otawa
