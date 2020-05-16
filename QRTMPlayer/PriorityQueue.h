#pragma execution_character_set("utf-8")
#pragma once
#include <queue>
#include <mutex>
#include "Packet.h"

class cmp
{
public:
  bool operator()( Packet* p1, Packet* p2 )
  {
    if ( p1->timestamp() > p2->timestamp() )
      return true;
    else if ( p1->timestamp() == p2->timestamp() )
      return p1->seq() > p2->seq();
    return false;
  }
};

class PriorityQueue
{
  std::priority_queue<Packet*, std::vector<Packet*>, cmp> encPacketQueue;
  std::mutex fMx;
public:
  void push( Packet *packet )
  {
    std::unique_lock<std::mutex> lock( fMx );
    encPacketQueue.push( packet );
  }
  Packet *top()
  {
    std::unique_lock<std::mutex> lock( fMx );
    return encPacketQueue.top();
  }
  void pop()
  {
    std::unique_lock<std::mutex> lock( fMx );
    encPacketQueue.pop();
  }
  bool empty()
  {
    return encPacketQueue.empty();
  }
};

