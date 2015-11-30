/*
 *	cfgio plugins features
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2014, IRIT UPS.
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
#ifndef OTAWA_CFGIO_FEATURES_H_
#define OTAWA_CFGIO_FEATURES_H_

#include <otawa/prop/Identifier.h>

namespace otawa { namespace cfgio {

// Output configuration
extern Identifier<cstring> INCLUDE;
extern Identifier<bool> NO_INSTS;
extern Identifier<Path> OUTPUT;

// Input configuration
extern Identifier<Path> FROM;

} }		// otawa::cfgio

#endif /* OTAWA_CFGIO_FEATURES_H_ */
