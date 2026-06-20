/*
 *	Copyright (c) 2026 Denis Rozhkov <denis@rozhkoff.com>
 *	This file is part of as-tunnelizer.
 *
 *	as-tunnelizer is free software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	as-tunnelizer is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *	Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along with
 *	as-tunnelizer. If not, see <https://www.gnu.org/licenses/>.
 */

// SPDX-FileCopyrightText: 2026 Denis Rozhkov <denis@rozhkoff.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tunnelizer-core.hpp"

#ifdef ASTUN_WINDOWS
#include <tchar.h>

#include "windows.h"
#endif


namespace as::tunnelizer {


	std::string TunnelizerHelper::resolveCwdPath( const std::string_view p )
	{
		std::string result( p );

#ifdef ASTUN_WINDOWS
		TCHAR moduleFileName[MAX_PATH + 1];

		if ( GetModuleFileName( NULL, moduleFileName, MAX_PATH ) != 0 ) {
			TCHAR drive[_MAX_DRIVE];
			TCHAR dir[_MAX_DIR];

			if ( _tsplitpath_s( moduleFileName, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0 ) == 0 ) {
				TCHAR pathT[_MAX_PATH];

				if ( _tmakepath_s( pathT, _MAX_PATH, drive, dir, p.data(), NULL ) == 0 ) {
					result.assign( pathT );
				}
			}
		}
#endif

		return result;
	}


} // namespace as::tunnelizer
