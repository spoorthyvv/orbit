//-----------------------------------
// Copyright Pierric Gimmig 2013-2017
//-----------------------------------

#include "OrbitAsio.h"
#include "Tcp.h"
#include "VariableTracing.h"

#include <asio/impl/src.hpp>
#include <asio/ssl/impl/src.hpp>

//-----------------------------------------------------------------------------
TcpService::TcpService()
{
    m_IoService = new asio::io_service();

    m_SSLContext = new asio::ssl::context(asio::ssl::context::sslv23);

    // :TODO_SSL: Setup default SSL Context states
    m_SSLContext->set_default_verify_paths();
}

//-----------------------------------------------------------------------------
TcpService::~TcpService()
{
    PRINT_FUNC;
}

//-----------------------------------------------------------------------------
TcpSocket::~TcpSocket()
{
    PRINT_FUNC;
}


