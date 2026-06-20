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

#include "logger.hpp"

#include "tunnelizer-core.hpp"
#include "tunnelizer-service.hpp"
#include "tunnelizer-server.hpp"
#include "tunnelizer-client.hpp"

#ifdef ASTUN_WINDOWS
#include "windows.h"
#endif


namespace as::tunnelizer {


	TunnelizerService * TunnelizerService::s_instance = nullptr;


	void TunnelizerService::install()
	{
		bool result = false;

#ifdef ASTUN_WINDOWS
		SC_HANDLE scManagerHandle;
		TCHAR moduleFileName[MAX_PATH + 1];

		if ( GetModuleFileName( NULL, moduleFileName, MAX_PATH ) != 0 ) {
			scManagerHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

			if ( scManagerHandle != NULL ) {
				std::string path;
				path.append( "\"" ).append( moduleFileName ).append( "\" -r \"" ).append( m_name ).append( "\"" );

				SC_HANDLE serviceHandle = CreateService( scManagerHandle,
					m_name.c_str(),
					m_displayName.c_str(),
					SERVICE_ALL_ACCESS,
					SERVICE_WIN32_OWN_PROCESS,
					SERVICE_AUTO_START,
					SERVICE_ERROR_NORMAL,
					path.c_str(),
					NULL,
					NULL,
					NULL,
					NULL,
					NULL );

				if ( serviceHandle != NULL ) {
					SERVICE_DESCRIPTION serviceDesc;
					serviceDesc.lpDescription = const_cast<LPSTR>( m_description.c_str() );

					ChangeServiceConfig2( serviceHandle, SERVICE_CONFIG_DESCRIPTION, &serviceDesc );

					StartService( serviceHandle, 0, NULL );

					CloseServiceHandle( serviceHandle );

					result = true;
				}

				CloseServiceHandle( scManagerHandle );
			}
		}

		if ( !result ) {
			throw std::runtime_error( "Failed to install service: " + m_name );
		}
#endif
	}


	void TunnelizerService::uninstall()
	{
		bool result = false;

#ifdef ASTUN_WINDOWS
		SC_HANDLE scManagerHandle;

		scManagerHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

		if ( scManagerHandle != NULL ) {
			SC_HANDLE serviceHandle = OpenService( scManagerHandle, m_name.c_str(), DELETE );

			if ( serviceHandle != NULL ) {
				result = ( DeleteService( serviceHandle ) != 0 );

				CloseServiceHandle( serviceHandle );
			}

			CloseServiceHandle( scManagerHandle );
		}

		if ( !result ) {
			throw std::runtime_error( "Failed to uninstall service: " + m_name );
		}
#endif
	}


	bool TunnelizerService::start( const as::tunnelizer::TunnelizerConfig & config )
	{
		m_config = config;
		s_instance = this;

		bool result = true;

		if ( m_isServiceMode ) {
#ifdef ASTUN_WINDOWS
			SERVICE_TABLE_ENTRY serviceTable[] = { { .lpServiceName = const_cast<LPSTR>( m_name.c_str() ), .lpServiceProc = (LPSERVICE_MAIN_FUNCTION)main },
				{ NULL, NULL } };

			bool result = ( StartServiceCtrlDispatcher( serviceTable ) != 0 );
#endif
		}
		else {
			main();
		}

		return result;
	}


	void TunnelizerService::main()
	{
		if ( m_isServiceMode ) {
#ifdef ASTUN_WINDOWS
			m_statusHandle = RegisterServiceCtrlHandlerEx( m_name.c_str(), controlHandler, this );

			if ( m_statusHandle == 0 ) {
				AS_LOG_ERROR_LINE( "control handler registration failed" );
				return;
			}
#endif
		}

#ifdef ASTUN_WINDOWS
		m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		m_status.dwWin32ExitCode = NO_ERROR;
		m_status.dwCheckPoint = 0;
		m_status.dwWaitHint = 5 * 1000;

		Status( SERVICE_START_PENDING );
#endif

		IsRunning( true );

#ifdef ASTUN_WINDOWS
		Status( SERVICE_RUNNING );
#endif

		while ( IsRunning() ) {
			if ( m_config.Mode() == TunnelizerMode::Server ) {
				auto t = std::make_shared<TunnelizerServer>( m_config.ServerPortNo() );
				m_tunnelizer = t;
				t->run();
			}
			else if ( m_config.Mode() == TunnelizerMode::Client ) {
				auto t = std::make_shared<TunnelizerClient>( m_config.ServerHostname(), m_config.ServerPortNo(), m_config.Hostname(), m_config.PortNo() );
				m_tunnelizer = t;
				t->run();
			}

#ifndef ASTUN_WINDOWS
			IsRunning( false );
#endif
		}

#ifdef ASTUN_WINDOWS
		Status( SERVICE_STOPPED );
#endif
	}


	void TunnelizerService::stop()
	{
		if ( m_tunnelizer ) {
			m_tunnelizer->stop();
		}

		IsRunning( false );

#ifdef ASTUN_WINDOWS
		Status( SERVICE_STOP_PENDING );
#endif
	}


#ifdef ASTUN_WINDOWS
	void TunnelizerService::Status( DWORD status )
	{
		m_status.dwCurrentState = status;

		if ( m_isServiceMode ) {
			static DWORD dwCheckPoint = 1;

			if ( status == SERVICE_START_PENDING ) {
				m_status.dwControlsAccepted = 0;
			}
			else {
				m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

				if ( ( status == SERVICE_RUNNING ) || ( status == SERVICE_STOPPED ) ) {
					m_status.dwCheckPoint = 0;
				}
				else {
					m_status.dwCheckPoint = dwCheckPoint++;
				}
			}

			if ( SetServiceStatus( m_statusHandle, &m_status ) == FALSE ) {
				AS_LOG_ERROR_LINE( "SetServiceStatus failed" );
			}
		}
	}


	DWORD TunnelizerService::Status()
	{
		return m_status.dwCurrentState;
	}


	DWORD TunnelizerService::controlHandler( DWORD controlCode, DWORD eventType, LPVOID eventData )
	{
		DWORD result = NO_ERROR;

		switch ( controlCode ) {
			case SERVICE_CONTROL_STOP:
				stop();
				break;

			default:
				break;
		}

		return result;
	}


	VOID TunnelizerService::main( DWORD argc, LPTSTR * argv )
	{
		s_instance->main();
	}


	DWORD TunnelizerService::controlHandler( DWORD controlCode, DWORD eventType, LPVOID eventData, LPVOID context )
	{
		DWORD result = NO_ERROR;
		auto _this = static_cast<TunnelizerService *>( context );

		if ( _this != nullptr ) {
			return _this->controlHandler( controlCode, eventType, eventData );
		}
		else {
			return ERROR_INVALID_PARAMETER;
		}
	}
#endif


} // namespace as::tunnelizer
