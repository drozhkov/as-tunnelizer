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

#ifndef __ASTUN__TUNNELIZER_SERVICE__H
#define __ASTUN__TUNNELIZER_SERVICE__H


#include <string>
#include <string_view>

#include "tunnelizer-core.hpp"
#include "tunnelizer-config.hpp"

#ifdef ASTUN_WINDOWS
#include "windows.h"
#endif // ASTUN_WINDOWS


namespace as::tunnelizer {


	class TunnelizerService {
		static TunnelizerService * s_instance;

		std::string m_name;
		std::string m_displayName;
		std::string m_description;
		bool m_isServiceMode;

		TunnelizerConfig m_config;
		std::shared_ptr<TunnelizerBase> m_tunnelizer;

		volatile bool m_isRunning = false;

#ifdef ASTUN_WINDOWS
		SERVICE_STATUS_HANDLE m_statusHandle = NULL;
		SERVICE_STATUS m_status = {};
#endif


	private:
		bool IsRunning() const
		{
			return m_isRunning;
		}


		void IsRunning( bool v )
		{
			m_isRunning = v;
		}


		void main();
		void stop();

#ifdef ASTUN_WINDOWS
		static VOID WINAPI main( DWORD argc, LPTSTR * argv );
		static DWORD WINAPI controlHandler( DWORD controlCode, DWORD eventType, LPVOID eventData, LPVOID context );

		void Status( DWORD status );
		DWORD Status();

		DWORD controlHandler( DWORD controlCode, DWORD eventType, LPVOID eventData );
#endif


	public:
		TunnelizerService( const std::string_view name,
			bool isServiceMode =
#ifdef ASTUN_WINDOWS
				true
#else
				false
#endif
			,
			const std::string_view displayName = "AS Tunnelizer",
			const std::string_view description = "Creates TCP tunnels" )
			: m_name( name )
			, m_displayName( displayName )
			, m_description( description )
			, m_isServiceMode( isServiceMode )
		{
		}


		void install();
		void uninstall();
		bool start( const as::tunnelizer::TunnelizerConfig & config );
	};


} // namespace as::tunnelizer


#endif
