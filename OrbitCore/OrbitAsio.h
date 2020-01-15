//-----------------------------------
// Copyright Pierric Gimmig 2013-2017
//-----------------------------------
#pragma once

#include "asio.hpp"
#include "asio/ssl.hpp"

using namespace asio::ip;

//-----------------------------------------------------------------------------
class TcpService
{
public:
    TcpService();
    ~TcpService();
    asio::io_context* m_IoService;
    asio::ssl::context* m_SSLContext;
};

//-----------------------------------------------------------------------------
using TcpSocketType = asio::ssl::stream<tcp::socket>;

//-----------------------------------------------------------------------------
class TcpSocket
{
public:
    TcpSocket() : m_Socket( nullptr ){}
    TcpSocket( TcpSocketType* a_Socket ) : m_Socket( a_Socket ){};
    ~TcpSocket();
    TcpSocketType* m_Socket;
};