#pragma once
#include <chrono>
#include <string>

class DevProfiler {
  public:
  std::chrono::steady_clock::time_point startAt;
  std::string markerName;

  inline DevProfiler(std::string markerName)
  {
    startAt = std::chrono::steady_clock::now();
    this->markerName = markerName;
  }

  inline ~DevProfiler()
  {
    printf("%s execution time: %f ms\n", this->markerName.c_str(), std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - this->startAt).count());
  }
};