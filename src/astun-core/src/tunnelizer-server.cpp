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
#include <memory>
#include <mutex>
#include <csignal>
#include <exception>

#include "boost/asio/awaitable.hpp"
#include "boost/asio/buffer.hpp"
#include "boost/asio/co_spawn.hpp"
#include "boost/asio/detached.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/ip/udp.hpp"
#include "boost/asio/signal_set.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "boost/asio/write.hpp"
#include "boost/asio/this_coro.hpp"

#include "logger.hpp"

#include "tunnelizer-core.hpp"
#include "tunnelizer-server.hpp"
#include "tunnelizer-session.hpp"


namespace as::tunnelizer {


	boost::asio::awaitable<void> TunnelizerServer::worker( std::shared_ptr<TunnelizerSessionTcp> session )
	{
		try {
			{
				ASTUN_LOCK( std::recursive_mutex, m_sessionsTcpMutex );

				for ( size_t i = 0; i < m_sessionsTcp.size(); ++i ) {
					if ( m_sessionsTcp[i]->Id() != session->Id() && !m_sessionsTcp[i]->Cache().empty() ) {
						// try {
						AS_LOG_DEBUG_LINE( "session: " << m_sessionsTcp[i]->Id() << " has cached data, sending it first." );

						auto cachedBuffer = boost::asio::buffer( m_sessionsTcp[i]->Cache() );
						co_await boost::asio::async_write( session->Socket(), cachedBuffer, boost::asio::use_awaitable );

						m_sessionsTcp[i]->Cache().clear();
						//}
						// catch ( std::exception & e ) {
						//}
					}
				}
			}

			std::vector<char> data( ASTUN_TCP_BUFFER_SIZE );

			for ( ;; ) {
				std::size_t n = co_await session->Socket().async_read_some( boost::asio::buffer( data ), boost::asio::use_awaitable );
				AS_LOG_DEBUG_LINE( "received " << n << " bytes from session: " << session->Id() );

				{
					ASTUN_LOCK( std::recursive_mutex, m_sessionsTcpMutex );

					auto buffer = boost::asio::buffer( data, n );

					for ( size_t i = 0; i < m_sessionsTcp.size(); ++i ) {
						if ( m_sessionsTcp[i]->Id() != session->Id() && !m_sessionsTcp[i]->IsDead() ) {
							// try {
							AS_LOG_DEBUG_LINE( "echoing to session: " << m_sessionsTcp[i]->Id() );

							co_await async_write( m_sessionsTcp[i]->Socket(), buffer, boost::asio::use_awaitable );
							//}
							// catch ( std::exception & e ) {
							//	m_sessionsTcp[i]->IsDead( true );
							//}
						}
					}

					if ( ActiveTcpSessionsCount() < 2 ) {
						session->Cache().insert( session->Cache().end(), data.begin(), data.begin() + n );
					}
				}
			}
		}
		catch ( std::exception & e ) {
			AS_LOG_ERROR_LINE( session->Id() << " - " << e.what() );
		}

		{
			ASTUN_LOCK( std::recursive_mutex, m_sessionsTcpMutex );

			for ( size_t i = 0; i < m_sessionsTcp.size(); ++i ) {
				try {
					m_sessionsTcp[i]->Socket().close();
				}
				catch ( std::exception & e ) {
					AS_LOG_ERROR_LINE( m_sessionsTcp[i]->Id() << " - " << e.what() );
				}
			}

			m_sessionsTcp.clear();
		}
		// session->IsDead( true );
	}


	boost::asio::awaitable<void> TunnelizerServer::listenerTcp()
	{
		size_t sessionId = 0;

		while ( true ) {
			try {
				auto executor = co_await boost::asio::this_coro::executor;
				boost::asio::ip::tcp::acceptor acceptor( executor, boost::asio::ip::tcp::endpoint{ boost::asio::ip::tcp::v4(), m_portNo } );

				AS_LOG_INFO_LINE( "listening on port (TCP): " << m_portNo );

				for ( ;; ) {
					auto socket = co_await acceptor.async_accept( boost::asio::use_awaitable );
					AS_LOG_INFO_LINE( "new connection from: " << socket.remote_endpoint() );

					std::shared_ptr<TunnelizerSessionTcp> session;

					{
						ASTUN_LOCK( std::recursive_mutex, m_sessionsTcpMutex );

						if ( ActiveTcpSessionsCount() >= 2 ) {
							socket.close();
							continue;
						}

						session = m_sessionsTcp.emplace_back( new TunnelizerSessionTcp( socket, sessionId++ ) );
					}

					boost::asio::co_spawn( executor, worker( session ), boost::asio::detached );
				}
			}
			catch ( std::exception & e ) {
				AS_LOG_ERROR_LINE( e.what() );
			}
		}
	}


	boost::asio::awaitable<void> TunnelizerServer::listenerUdp()
	{
		size_t sessionId = 0;

		while ( true ) {
			try {
				auto executor = co_await boost::asio::this_coro::executor;
				boost::asio::ip::udp::socket socket( executor, boost::asio::ip::udp::endpoint{ boost::asio::ip::udp::v4(), m_portNo } );

				AS_LOG_INFO_LINE( "listening on port (UDP): " << m_portNo );

				boost::asio::ip::udp::endpoint remoteEndpoint;
				std::array<char, 1024> data;

				for ( ;; ) {
					auto n = co_await socket.async_receive_from( boost::asio::buffer( data ), remoteEndpoint, boost::asio::use_awaitable );

					AS_LOG_DEBUG_LINE( "new datagram from: " << remoteEndpoint );

					std::shared_ptr<TunnelizerSessionUdp> session;

					auto sessionKey = TunnelizerSession::SessionKey( remoteEndpoint );
					auto sessionIt = m_sessionsUdp.find( sessionKey );

					if ( sessionIt != m_sessionsUdp.end() ) {
						session = sessionIt->second;
					}
					else {
						session = std::make_shared<TunnelizerSessionUdp>( remoteEndpoint, sessionId++ );
						m_sessionsUdp[sessionKey] = session;
					}

					for ( auto & [key, s] : m_sessionsUdp ) {
						if ( s->Id() != session->Id() && !s->IsDead() ) {
							AS_LOG_DEBUG_LINE( "echoing to session: " << s->Id() );
							auto buffer = boost::asio::buffer( data, n );

							try {
								co_await socket.async_send_to( buffer, s->Endpoint(), boost::asio::use_awaitable );
							}
							catch ( std::exception & e ) {
								s->IsDead( true );
							}
						}
					}

					// boost::asio::co_spawn( executor, worker( session ), boost::asio::detached );
				}
			}
			catch ( std::exception & e ) {
				AS_LOG_ERROR_LINE( e.what() );
			}
		}
	}


	void TunnelizerServer::run()
	{
		try {
			boost::asio::signal_set signals( m_ioContext, SIGINT, SIGTERM );
			signals.async_wait( [this]( auto, auto ) { m_ioContext.stop(); } );

			boost::asio::co_spawn( m_ioContext, listenerTcp(), boost::asio::detached );
			boost::asio::co_spawn( m_ioContext, listenerUdp(), boost::asio::detached );

			m_ioContext.run();
		}
		catch ( std::exception & e ) {
			AS_LOG_ERROR_LINE( e.what() );
		}
	}


} // namespace as::tunnelizer
