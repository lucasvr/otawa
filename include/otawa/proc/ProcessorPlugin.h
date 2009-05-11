/*
 *	$Id$
 *	ProcessorPlugin class interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2009, IRIT UPS.
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
#ifndef OTAWA_PROC_PROCESSORPLUGIN_H_
#define OTAWA_PROC_PROCESSORPLUGIN_H_

#include <elm/genstruct/Table.h>
#include <elm/system/Plugin.h>
#include <elm/system/Path.h>
#include <otawa/proc/Registration.h>

namespace otawa {

// Definitions
#define OTAWA_PROC_HOOK		proc_plugin
#define OTAWA_PROC_NAME		"proc_plugin"
#define OTAWA_PROC_VERSION	Version(1, 0, 0)

// ProcessorPlugin class
class ProcessorPlugin: public elm::system::Plugin {
public:
	ProcessorPlugin(cstring name, const elm::Version& version, const elm::Version& plugger_version);
	virtual elm::genstruct::Table<AbstractRegistration *>& processors(void) const = 0;

	static void addPath(const elm::system::Path& path);
	static void removePath(const elm::system::Path& path);
	static ProcessorPlugin *get(string name);
	static Processor *getProcessor(cstring name);
	static AbstractFeature *getFeature(cstring name);
	static AbstractIdentifier *getIdentifier(cstring name);

private:
	static void init(void);
};

}	// otawa

#endif /* OTAWA_PROC_PROCESSORPLUGIN_H_ */

