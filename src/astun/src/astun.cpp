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

// astun.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <exception>

#include "logger.hpp"

#include "tunnelizer-core.hpp"
#include "tunnelizer-config.hpp"
#include "tunnelizer-server.hpp"
#include "tunnelizer-client.hpp"
#include "tunnelizer-service.hpp"


struct Options {
	bool isServer = false;
	bool isClient = false;
	bool isInstall = false;
	bool isUninstall = false;
	bool isRun = false;
	std::string serverHostname;
	uint16_t serverPortNo;
	std::string serviceHostname;
	uint16_t servicePortNo;
	std::string name{ "as-tunnelizer" };
};


static Options parseOptions( int argc, char * argv[] )
{
	Options options;

	if ( argc < 2 ) {
		options.isRun = true;
		return options;
	}

	auto argv1 = std::string_view( argv[1] );

	if ( argc > 1 && ( argv1 == "-c" || argv1 == "--client" ) ) {
		if ( argc < 6 ) {
			throw std::runtime_error( "Not enough arguments for client mode." );
		}

		options.isClient = true;
		options.serverHostname = argv[2];
		options.serverPortNo = static_cast<uint16_t>( std::stoi( argv[3] ) );
		options.serviceHostname = argv[4];
		options.servicePortNo = static_cast<uint16_t>( std::stoi( argv[5] ) );
	}
	else if ( argv1 == "-s" || argv1 == "--server" ) {
		if ( argc < 3 ) {
			throw std::runtime_error( "Not enough arguments for server mode." );
		}

		options.isServer = true;
		options.serverPortNo = static_cast<uint16_t>( std::stoi( argv[2] ) );
	}
	else if ( argv1 == "-i" || argv1 == "--install" ) {
		options.isInstall = true;

		if ( argc > 2 ) {
			options.name = argv[2];
		}
	}
	else if ( argv1 == "-u" || argv1 == "--uninstall" ) {
		options.isUninstall = true;

		if ( argc > 2 ) {
			options.name = argv[2];
		}
	}
	else if ( argv1 == "-r" || argv1 == "--run" ) {
		options.isRun = true;

		if ( argc > 2 ) {
			options.name = argv[2];
		}
	}
	else {
		throw std::runtime_error( "Unknown option: " + std::string( argv[1] ) );
	}

	return options;
}


static void usage( std::string_view & binName )
{
	std::cout << "Usage: " << binName
			  << " [-s <port>] | [-c <server-hostname> <server-port> <service-hostname> <service-port>] | [-r [name]]"
#ifdef ASTUN_WINDOWS
				 " | [-i [name]] | [-u [name]]"
#endif
			  << std::endl;
}


int main( int argc, char * argv[] )
{
	std::string_view binName = argv[0];

	if ( ( argc > 1 ) && ( std::string_view( argv[1] ) == "-h" || std::string_view( argv[1] ) == "--help" ) ) {
		usage( binName );
		return 0;
	}

	try {
		auto options = parseOptions( argc, argv );
		as::loggerInit( as::tunnelizer::TunnelizerHelper::resolveCwdPath( options.name + ".log" ) );

		if ( options.isServer ) {
			as::tunnelizer::TunnelizerServer server( options.serverPortNo );
			server.run();
		}
		else if ( options.isClient ) {
			as::tunnelizer::TunnelizerClient client( options.serverHostname, options.serverPortNo, options.serviceHostname, options.servicePortNo );
			client.run();
		}
		else {
			as::tunnelizer::TunnelizerService service( options.name );

			if ( options.isInstall ) {
				service.install();
				std::clog << "Service installed successfully." << std::endl;
			}
			else if ( options.isUninstall ) {
				service.uninstall();
				std::clog << "Service uninstalled successfully." << std::endl;
			}
			else if ( options.isRun ) {
				as::tunnelizer::TunnelizerConfig config;
				config.load( as::tunnelizer::TunnelizerHelper::resolveCwdPath( "config.json" ) );
				service.start( config );
			}
		}
	}
	catch ( const std::exception & e ) {
		AS_LOG_ERROR_LINE( e.what() );
		return 1;
	}

	return 0;
}
