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

#ifndef __ASTUN__TUNNELIZER_SESSION__H
#define __ASTUN__TUNNELIZER_SESSION__H


#include <vector>

#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/ip/udp.hpp"


namespace as::tunnelizer {

	enum class TunnelizerSessionType { Tcp, Udp };


	class TunnelizerSession {
	protected:
		size_t m_id;
		TunnelizerSessionType m_type;
		volatile bool m_isDead = false;

		std::vector<char> m_cache;


	public:
		TunnelizerSession( size_t id, TunnelizerSessionType type )
			: m_id( id )
			, m_type( type )
		{
		}


		template <typename T_protocol> static uint64_t SessionKey( const boost::asio::ip::basic_endpoint<T_protocol> & endpoint )
		{
			return ( ( static_cast<uint64_t>( endpoint.address().to_v4().to_uint() ) << 16 ) | static_cast<uint16_t>( endpoint.protocol().protocol() ) );
		}


		size_t Id() const
		{
			return m_id;
		}


		TunnelizerSessionType Type() const
		{
			return m_type;
		}


		bool IsDead() const
		{
			return m_isDead;
		}


		void IsDead( bool v )
		{
			m_isDead = v;
		}


		std::vector<char> & Cache()
		{
			return m_cache;
		}
	};


	class TunnelizerSessionTcp : public TunnelizerSession {
		std::shared_ptr<boost::asio::ip::tcp::socket> m_socket;


	public:
		TunnelizerSessionTcp( boost::asio::ip::tcp::socket & socket, size_t id )
			: TunnelizerSession( id, TunnelizerSessionType::Tcp )
			, m_socket( std::make_shared<boost::asio::ip::tcp::socket>( std::move( socket ) ) )
		{
		}


		boost::asio::ip::tcp::socket & Socket()
		{
			return *m_socket;
		}


		void Socket( boost::asio::ip::tcp::socket & v )
		{
			m_socket = std::make_shared<boost::asio::ip::tcp::socket>( std::move( v ) );
		}
	};


	class TunnelizerSessionUdp : public TunnelizerSession {
		boost::asio::ip::udp::endpoint m_endpoint;

	public:
		TunnelizerSessionUdp( boost::asio::ip::udp::endpoint & endpoint, size_t id )
			: TunnelizerSession( id, TunnelizerSessionType::Udp )
			, m_endpoint( std::move( endpoint ) )
		{
		}


		const boost::asio::ip::udp::endpoint & Endpoint() const
		{
			return m_endpoint;
		}
	};

} // namespace as::tunnelizer


#endif
