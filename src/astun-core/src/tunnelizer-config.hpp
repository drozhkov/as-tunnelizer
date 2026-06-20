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

#ifndef __ASTUN__TUNNELIZER_CONFIG__H
#define __ASTUN__TUNNELIZER_CONFIG__H


#include "tunnelizer-core.hpp"


namespace as::tunnelizer {


	class TunnelizerConfig {
		TunnelizerMode m_mode;
		uint16_t m_portNo;
		std::string m_hostname;
		std::string m_serverHostname;
		uint16_t m_serverPortNo;


	public:
		TunnelizerMode Mode() const
		{
			return m_mode;
		}


		uint16_t PortNo() const
		{
			return m_portNo;
		}


		const std::string & Hostname() const
		{
			return m_hostname;
		}


		const std::string & ServerHostname() const
		{
			return m_serverHostname;
		}


		uint16_t ServerPortNo() const
		{
			return m_serverPortNo;
		}


		void load( const std::string_view path );
	};


} // namespace as::tunnelizer


#endif
