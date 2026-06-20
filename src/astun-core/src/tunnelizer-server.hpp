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

#ifndef __ASTUN__TUNNELIZER_SERVER__H
#define __ASTUN__TUNNELIZER_SERVER__H


#include <memory>
#include <mutex>
#include <vector>
#include <cstdint>
#include <unordered_map>

#include "boost/asio/awaitable.hpp"

#include "tunnelizer-core.hpp"
#include "tunnelizer-session.hpp"


namespace as::tunnelizer {


	class TunnelizerServer : public TunnelizerBase {
	protected:
		uint16_t m_portNo;

		std::vector<std::shared_ptr<TunnelizerSessionTcp>> m_sessionsTcp;
		std::unordered_map<uint64_t, std::shared_ptr<TunnelizerSessionUdp>> m_sessionsUdp;
		std::recursive_mutex m_sessionsTcpMutex;


	protected:
		boost::asio::awaitable<void> worker( std::shared_ptr<TunnelizerSessionTcp> session );
		boost::asio::awaitable<void> listenerTcp();
		boost::asio::awaitable<void> listenerUdp();


	public:
		TunnelizerServer( uint16_t portNo )
			: m_portNo( portNo )
		{
		}


		~TunnelizerServer() = default;


		size_t ActiveTcpSessionsCount() const
		{
			size_t result = 0;

			for ( size_t i = 0; i < m_sessionsTcp.size(); ++i ) {
				if ( !m_sessionsTcp[i]->IsDead() ) {
					++result;
				}
			}

			return result;
		}


		void run();
	};


} // namespace as::tunnelizer


#endif
