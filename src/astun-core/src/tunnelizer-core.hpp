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

#ifndef __ASTUN__TUNNELIZER_CORE__H
#define __ASTUN__TUNNELIZER_CORE__H


#include <string_view>

#include "boost/asio.hpp"


#if 0
#define ASTUN_LOCK( a_mutexType, a_mutex ) std::lock_guard<a_mutexType> lock( a_mutex )
#else
#define ASTUN_LOCK( a_mutexType, a_mutex )
#endif

#define ASTUN_TCP_BUFFER_SIZE ( 1024 * 1024 )

#if defined( _WIN32 ) or defined( _WIN64 )
#define ASTUN_WINDOWS
#endif


namespace as::tunnelizer {


	enum class TunnelizerMode { Client, Server };


	class TunnelizerBase {
	protected:
		boost::asio::io_context m_ioContext;


	public:
		virtual void stop()
		{
			m_ioContext.stop();
		}
	};


	class TunnelizerHelper {
	public:
		static std::string resolveCwdPath( const std::string_view path );
	};


} // namespace as::tunnelizer


#endif
