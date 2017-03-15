#include <chrono>
#include <iostream>

#include "threadpool.h"

int main() {
  qp::threading::Threadpool p;
  std::atomic<bool> stopped(false);

  p.add(
      [](const std::atomic<bool>& stopped) {
        while (!stopped) {
          std::cout << "still going" << std::endl;
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      },
      std::cref(stopped));

  std::this_thread::sleep_for(std::chrono::seconds(2));
  stopped = true;
}
