#include "websocketserver.h"

#include <iostream>


#include "playerconfiguration.h"


// http://www.gamefromscratch.com/post/2012/02/28/Network-programming-with-SFML-and-Nodejs-Part-2.aspx
// http://www.gamefromscratch.com/post/2012/03/14/Network-programming-with-SFML-and-Nodejs-Part-3.aspx


// https://nodejs.org/api/dgram.html



WebSocketServer::WebSocketServer()
{
   mThread = std::make_unique<std::thread>(&WebSocketServer::receiveData, this);

   uint16_t localPort = 8080;
   if (mSocket.bind(localPort) == sf::Socket::Done)
   {
      printf("server bound at port %d\n", localPort);
   }
}


WebSocketServer::~WebSocketServer()
{
   mStopped = false;
   mSocket.setBlocking(false);
   mThread->join();
}


void WebSocketServer::sendData()
{
   sf::IpAddress ip("127.0.0.1");

   // serialize player config
   PlayerConfiguration pc;
   std::string config = pc.serialize();
   mSocket.send(config.c_str(), config.size(), ip, 8080);
}


void WebSocketServer::receiveData()
{
   char buffer[512];
   size_t respSize = 0;
   unsigned short remotePort = 0;

   while (!mStopped)
   {
      mSocket.receive(buffer, 512, respSize, mIp, remotePort);
   }
}


void WebSocketServer::serialize()
{
   sf::UdpSocket socket;
   sf::Packet packet;

   if (socket.send(packet, mIp, mLocalPort) != sf::Socket::Done)
   {
      std::cout << "An error ocurred sending packet" << std::endl;
   }
}



void WebSocketServer::deserialize()
{

}


