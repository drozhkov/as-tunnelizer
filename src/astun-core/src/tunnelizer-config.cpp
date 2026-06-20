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

#include <iostream>
#include <fstream>

#include "boost/json.hpp"

#include "logger.hpp"

#include "tunnelizer-core.hpp"
#include "tunnelizer-config.hpp"

#ifdef ASTUN_WINDOWS
#include <tchar.h>

#include "windows.h"
#endif


namespace as::tunnelizer {


	static bool getObject( const boost::json::object & o, std::string_view n, std::reference_wrapper<const boost::json::object> & result )
	{
		if ( o.contains( n ) && o.at( n ).is_object() ) {
			result = o.at( n ).get_object();
			return true;
		}

		return false;
	}


	static bool getString( const boost::json::object & o, std::string_view n, std::reference_wrapper<const boost::json::string> & result )
	{
		if ( o.contains( n ) && o.at( n ).is_string() ) {
			result = o.at( n ).get_string();
			return true;
		}

		return false;
	}


	static bool getU64( const boost::json::object & o, std::string_view n, uint64_t & result )
	{
		if ( o.contains( n ) && o.at( n ).is_number() ) {
			result = o.at( n ).get_uint64();
			return true;
		}

		return false;
	}


	void TunnelizerConfig::load( const std::string_view path )
	{
		std::ifstream fs;
		fs.open( path.data() );

		if ( !fs.is_open() ) {
			return;
		}

		std::string json( std::istreambuf_iterator<char>{ fs }, {} );
		auto vRoot = boost::json::parse( json );
		const auto & oRoot = vRoot.get_object();

		std::reference_wrapper<const boost::json::object> o( oRoot );

		boost::json::string _;
		std::reference_wrapper<const boost::json::string> valueString( _ );

		m_mode = TunnelizerMode::Server;

		if ( getString( o, "mode", valueString ) ) {
			m_mode = valueString.get() == "server" ? TunnelizerMode::Server
				: valueString.get() == "client"	   ? TunnelizerMode::Client
												   : throw std::runtime_error( "Invalid mode." );
		}

		uint64_t u64;

		if ( !getU64( o, "server-port", u64 ) ) {
			throw std::runtime_error( "Invalid server port." );
		}

		m_serverPortNo = static_cast<uint16_t>( u64 );

		if ( m_mode == TunnelizerMode::Server ) {
			return;
		}

		if ( !getString( o, "host", valueString ) ) {
			throw std::runtime_error( "Invalid hostname." );
		}

		m_hostname = valueString.get().c_str();

		if ( !getU64( o, "port", u64 ) ) {
			throw std::runtime_error( "Invalid port." );
		}

		m_portNo = static_cast<uint16_t>( u64 );

		if ( !getString( o, "server-host", valueString ) ) {
			throw std::runtime_error( "Invalid server hostname." );
		}

		m_serverHostname = valueString.get().c_str();
	}


} // namespace as::tunnelizer
