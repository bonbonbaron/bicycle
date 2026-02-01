#pragma once
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
#include <functional>

using Clock = std::chrono::steady_clock;
using Duration = Clock::duration;
using Callback = std::function<void()>;

class Timer {
  public:
    Timer() = default;
    Timer( Callback&& callback, const Duration interval = std::chrono::milliseconds(0), const bool repeat = false, const int reps = -1 );
    ~Timer();

    void stop();
    void pause();
    void resume();
    void setInterval( const Duration interval );  // lets you speed up or slow down its pace

  private:
    Duration _interval{};
    int _reps{-1};
    std::atomic<bool> _running{true};
    Callback _callback{};
    int _id{};
    bool _repeat{ false };
    std::jthread _thread;
};
