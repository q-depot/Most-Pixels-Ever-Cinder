//
//  TCPAsyncClient.cpp
//  Unknown Project
//
//  Copyright (c) 2013 William Lindmeier. All rights reserved.
//

#include "TCPAsyncClient.h"

using namespace mpe;
using std::string;
using ci::app::console;
using namespace boost::asio::ip;

TCPAsyncClient::TCPAsyncClient(const std::string & messageDelimeter) :
TCPClient(messageDelimeter)
{
};

void TCPAsyncClient::open(const string & hostname,
                          const int port,
                          const OpenedCallback &callback)
{
    tcp::resolver resolver(mIOService);
    tcp::resolver::query query(hostname, std::to_string(port));
    tcp::resolver::iterator iterator = resolver.resolve(query);

    mOpenedCallback = callback;

    boost::asio::async_connect(mSocket, iterator,
                               boost::bind(&TCPAsyncClient::handleConnect, this,
                                            boost::asio::placeholders::error));

    mClientThread = std::thread(boost::bind(&boost::asio::io_service::run,
                                            &mIOService));
}

void TCPAsyncClient::close()
{
    mIOService.post(boost::bind(&TCPAsyncClient::_close, this));
}

void TCPAsyncClient::handleConnect(const boost::system::error_code & error)
{
    if (!error)
    {
        mIsConnected = true;
        boost::asio::async_read_until(mSocket, mBuffer, mMessageDelimiter,
                                      boost::bind(&TCPAsyncClient::Callbackead, this,
                                                  boost::asio::placeholders::error));
    }
    else
    {
        printf("ERROR connecting to host: %s\n", error.message().c_str());
    }

    if (mOpenedCallback)
    {
        mOpenedCallback(mIsConnected, error);
    }
}

void TCPAsyncClient::write(const string & msg)
{
    mIOService.post(boost::bind(&TCPAsyncClient::_write, this, msg));
}

void TCPAsyncClient::writeBuffer(const boost::asio::const_buffers_1 & buffer)
{
#if USE_STRING_QUEUE
    console() << "ALERT: Not using writeBuffer. Ignoring message.\n";
#else
    mIOService.post(boost::bind(&TCPAsyncClient::_writeBuffer, this, buffer));
#endif
}

void TCPAsyncClient::Callbackead(const boost::system::error_code & error)
{
    // console() << "handle read\n";
    if (!error)
    {
        if (mReadCallback)
        {
            std::ostringstream ss;
            ss << &mBuffer;
            string message = ss.str();
            mReadCallback(message);
        }
        boost::asio::async_read_until(mSocket, mBuffer, mMessageDelimiter,
                                      boost::bind(&TCPAsyncClient::Callbackead, this,
                                                  boost::asio::placeholders::error));
    }
    else
    {
        printf("ERROR reading from host: %s\n", error.message().c_str());
        _close();
    }
}

void TCPAsyncClient::_writeBuffer(const boost::asio::const_buffers_1 & buffer)
{
#if USE_STRING_QUEUE
    printf("ERROR not using _writeBuffer. Ignoring message.\n");
#else
    bool write_in_progress = !mWriteMessages.empty();
    mWriteMessages.push_back(buffer);
    if (!write_in_progress)
    {
        boost::asio::async_write(mSocket, mWriteMessages.front(),
                                 boost::bind(&TCPAsyncClient::handleWrite, this,
                                             boost::asio::placeholders::error));
    }
#endif
}

void TCPAsyncClient::_write(const string & msg)
{
#if USE_STRING_QUEUE
    bool write_in_progress = !mWriteMessages.empty();
    mWriteMessages.push_back(msg);
    if (!write_in_progress)
    {
        boost::asio::async_write(mSocket,
                                 boost::asio::buffer(mWriteMessages.front(),
                                                     mWriteMessages.front().length()),
                                 boost::bind(&TCPAsyncClient::handleWrite, this,
                                             boost::asio::placeholders::error));
    }
#else
    _writeBuffer(boost::asio::buffer(msg, msg.length()));
#endif
}

void TCPAsyncClient::handleWrite(const boost::system::error_code & error)
{
    if (!error)
    {
        mWriteMessages.pop_front();
        if (!mWriteMessages.empty())
        {
            //Send another
#if USE_STRING_QUEUE
            boost::asio::async_write(mSocket,
                                     boost::asio::buffer(mWriteMessages.front(),
                                                         mWriteMessages.front().length()),
                                     boost::bind(&TCPAsyncClient::handleWrite, this,
                                                 boost::asio::placeholders::error));
#else
            boost::asio::async_write(mSocket, mWriteMessages.front(),
                                     boost::bind(&TCPAsyncClient::handleWrite, this,
                                                 boost::asio::placeholders::error));
#endif
        }
    }
    else
    {
        printf("ERROR writing: %s\n", error.message().c_str());
        _close();
    }
}

void TCPAsyncClient::_close()
{
    if (mSocket.is_open())
    {
        mSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        mSocket.close();
    }
    mIsConnected = false;
}