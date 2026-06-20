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

#include "boost/asio.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/asio/buffer.hpp"
#include "boost/asio/co_spawn.hpp"
#include "boost/asio/detached.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/signal_set.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "boost/asio/write.hpp"
#include "boost/asio/this_coro.hpp"

#include "logger.hpp"

#include "tunnelizer-core.hpp"
#include "tunnelizer-client.hpp"
#include "tunnelizer-session.hpp"


namespace as::tunnelizer {


	boost::asio::awaitable<void> TunnelizerClient::workerTcp( const std::string_view hostname, uint16_t portNo )
	{
		AS_LOG_DEBUG_LINE( hostname << ":" << portNo );

		boost::asio::ip::tcp::resolver::results_type endpoints;
		std::shared_ptr<TunnelizerSessionTcp> session;
		auto executor = co_await boost::asio::this_coro::executor;

		while ( true ) {
			try {
				boost::asio::ip::tcp::resolver resolver( executor );

				endpoints = co_await resolver.async_resolve( hostname, std::to_string( portNo ), boost::asio::use_awaitable );

				boost::asio::ip::tcp::socket socket( executor );

				{
					ASTUN_LOCK( std::recursive_mutex, m_sessionsTcpMutex );

					session = m_sessionsTcp.emplace_back( new TunnelizerSessionTcp( socket, m_sessionId.fetch_add( 1ULL ) ) );

					session->IsDead( true );
				}

				break;
			}
			catch ( std::exception & e ) {
				AS_LOG_ERROR_LINE( hostname << ": " << portNo << " - " << e.what() );
			}
		}

		std::vector<char> data( ASTUN_TCP_BUFFER_SIZE );

		while ( true ) {
			try {
				// boost::asio::ip::tcp::socket socket( executor );
				// session->Socket( socket );

				AS_LOG_DEBUG_LINE( "connecting to session: " << session->Id() );
				// co_await boost::asio::async_connect( session->Socket(), endpoints, boost::asio::use_awaitable );
				co_await session->Socket().async_connect( *endpoints.begin(), boost::asio::use_awaitable );
				session->IsDead( false );

				AS_LOG_INFO_LINE( "connected to session: " << session->Id() << " (" << session->Socket().remote_endpoint() << ")" );

				{
					ASTUN_LOCK( std::recursive_mutex, m_sessionsTcpMutex );

					for ( size_t i = 0; i < m_sessionsTcp.size(); ++i ) {
						if ( m_sessionsTcp[i]->Id() != session->Id() && !m_sessionsTcp[i]->Cache().empty() ) {
							// try {
							auto cachedBuffer = boost::asio::buffer( m_sessionsTcp[i]->Cache() );
							co_await async_write( session->Socket(), cachedBuffer, boost::asio::use_awaitable );

							m_sessionsTcp[i]->Cache().clear();
							//}
							// catch ( std::exception & e ) {
							//	// m_sessionsTcp[i]->IsDead( true );
							//}
						}
					}
				}

				for ( ;; ) {
					std::size_t n = co_await session->Socket().async_read_some( boost::asio::buffer( data ), boost::asio::use_awaitable );

					AS_LOG_DEBUG_LINE( "received " << n << " bytes from session: " << session->Id() << " (" << session->Socket().remote_endpoint() << ")" );

					{
						ASTUN_LOCK( std::recursive_mutex, m_sessionsTcpMutex );

						auto buffer = boost::asio::buffer( data, n );

						for ( size_t i = 0; i < m_sessionsTcp.size(); ++i ) {
							if ( m_sessionsTcp[i]->Id() != session->Id() && !m_sessionsTcp[i]->IsDead() ) {
								// try {
								AS_LOG_DEBUG_LINE(
									"echoing to session: " << m_sessionsTcp[i]->Id() << " (" << m_sessionsTcp[i]->Socket().remote_endpoint() << ")" );

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
						m_sessionsTcp[i]->Cache().clear();
						m_sessionsTcp[i]->IsDead( true );
						m_sessionsTcp[i]->Socket().shutdown( boost::asio::socket_base::shutdown_both );
						m_sessionsTcp[i]->Socket().close();
					}
					catch ( std::exception & e ) {
						AS_LOG_ERROR_LINE( m_sessionsTcp[i]->Id() << " - " << e.what() );
					}
				}

				// m_sessionsTcp.clear();
			}

			// session->IsDead( true );
		}
	}


	boost::asio::awaitable<void> TunnelizerClient::workerUdp( const std::string_view hostname, uint16_t portNo, std::span<const char> data )
	{
		AS_LOG_DEBUG_LINE( hostname << ":" << portNo );

		std::shared_ptr<TunnelizerSessionUdp> session;
		std::shared_ptr<boost::asio::ip::udp::socket> socket;

		while ( true ) {
			try {
				auto executor = co_await boost::asio::this_coro::executor;
				boost::asio::ip::udp::resolver resolver( executor );

				auto endpoints = co_await resolver.async_resolve( boost::asio::ip::udp::v4(), hostname, std::to_string( portNo ), boost::asio::use_awaitable );

				boost::asio::ip::udp::endpoint remoteEndpoint = *endpoints.begin();

				socket.reset( new boost::asio::ip::udp::socket( executor, boost::asio::ip::udp::v4() ) );

				if ( !data.empty() ) {
					co_await socket->async_send_to( boost::asio::buffer( data ), remoteEndpoint, boost::asio::use_awaitable );
				}

				{
					ASTUN_LOCK( std::recursive_mutex, m_sessionsUdpMutex );
					session = m_sessionsUdp.emplace_back( new TunnelizerSessionUdp( remoteEndpoint, m_sessionId.fetch_add( 1ULL ) ) );
				}

				break;
			}
			catch ( std::exception & e ) {
				AS_LOG_ERROR_LINE( hostname << ": " << portNo << " - " << e.what() );
			}
		}

		std::array<char, 1024> buffer;
		boost::asio::ip::udp::endpoint senderEndpoint;

		while ( true ) {
			try {
				while ( true ) {
					std::size_t n = co_await socket->async_receive_from( boost::asio::buffer( buffer ), senderEndpoint, boost::asio::use_awaitable );

					AS_LOG_DEBUG_LINE( "received " << n << " bytes from session: " << session->Id() << " (" << senderEndpoint << ")" );

					{
						ASTUN_LOCK( std::recursive_mutex, m_sessionsUdpMutex );

						for ( size_t i = 0; i < m_sessionsUdp.size(); ++i ) {
							if ( m_sessionsUdp[i]->Id() != session->Id() ) {
								try {
									AS_LOG_DEBUG_LINE( "echoing to session: " << m_sessionsUdp[i]->Id() << " (" << m_sessionsUdp[i]->Endpoint() << ")" );

									co_await socket->async_send_to(
										boost::asio::buffer( buffer, n ), m_sessionsUdp[i]->Endpoint(), boost::asio::use_awaitable );
								}
								catch ( std::exception & e ) {
								}
							}
						}
					}
				}
			}
			catch ( std::exception & e ) {
				AS_LOG_ERROR_LINE( e.what() );
			}
		}
	}


	void TunnelizerClient::run()
	{
		try {
			static char initData[] = { 'H' };

			AS_LOG_INFO_LINE( "starting tunnelizer client: " << m_hostname << ":" << m_portNo << " <-> " << m_serverHostname << ":" << m_serverPortNo );

			boost::asio::signal_set signals( m_ioContext, SIGINT, SIGTERM );
			signals.async_wait( [this]( auto, auto ) { m_ioContext.stop(); } );

			boost::asio::co_spawn( m_ioContext, workerTcp( m_hostname, m_portNo ), boost::asio::detached );
			// boost::asio::co_spawn( m_ioContext, workerUdp( m_hostname, m_portNo ), boost::asio::detached );
			boost::asio::co_spawn( m_ioContext, workerTcp( m_serverHostname, m_serverPortNo ), boost::asio::detached );
			// boost::asio::co_spawn( m_ioContext, workerUdp( m_serverHostname, m_serverPortNo, initData ), boost::asio::detached );

			m_ioContext.run();
		}
		catch ( std::exception & e ) {
			AS_LOG_ERROR_LINE( e.what() );
		}
	}


} // namespace as::tunnelizer
