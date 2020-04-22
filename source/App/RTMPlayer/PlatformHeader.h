#pragma once
#include <stdint.h>
#include <string>
#include <chrono>
#include <unordered_map>
#include <xhash>
#include <process.h>
#include <atomic>
#include <mutex>
#include <queue>
#if _WIN32
#define NOMINMAX
#include <WinSock2.h>
#include <WinUser.h>
#else
#endif


#if _WIN32
#define msleep(ms)	Sleep(ms)
#else
#define ntohll		be64toh
#define htonll		htobe64
#define msleep(ms)	usleep(1000 * ms)
#endif

inline uint64_t get_timestamp_ms()
{
#ifdef _WIN32
  return std::chrono::duration_cast< std::chrono::milliseconds >
    ( std::chrono::system_clock::now().time_since_epoch() ).count();
#else
  struct timeval tv;
  gettimeofday( &tv, nullptr );
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

static std::atomic<uint64_t> g_u64Id = 1000;
inline uint64_t uuid64()
{
  return g_u64Id++;
}


