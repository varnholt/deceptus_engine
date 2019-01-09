#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <atomic>
#include <memory>
#include <thread>

#include "SFML/Network.hpp"


class WebSocketServer
{

public:

   WebSocketServer();
   ~WebSocketServer();

   void sendData();

   void receiveData();

   void serialize();

   void deserialize();


private:

   sf::IpAddress mIp = "127.0.0.1";
   uint16_t mLocalPort = 8081;
   // int mRemotePort = 8080;

   sf::UdpSocket mSocket;
   std::atomic<bool> mStopped;

   std::unique_ptr<sf::TcpListener> mListener;
   std::unique_ptr<std::thread> mThread;
   std::vector<std::shared_ptr<sf::TcpSocket>> mSockets;
};

#endif // WEBSOCKETSERVER_H
