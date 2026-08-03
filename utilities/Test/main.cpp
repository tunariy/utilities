#include <Allocators/PoolAllocator.hpp>
#include <Thread/ThreadPool.hpp>
#include <climits>
#include <exception>
#include <iostream>
#include <ostream>
#include <random>
#include <thread>
#include <vector>

static std::vector<int> gVector = {0};

using namespace std::chrono_literals;

inline int random_int(int lowerBound = 0, int upperBound = INT_MAX) {
    if (lowerBound >= upperBound) throw std::exception();
    uint32_t seed = (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count();
    std::knuth_b engine(seed);
    std::uniform_int_distribution<int> distribution(lowerBound, upperBound);

    return distribution(engine);
};

int main() {
#if 0
  utilities::PoolAllocator<100> test;
  char* a = static_cast<char*>(test.allocate(8));
  for (auto x{0}; x < 8; x++) {
    a[x] = static_cast<char>('a' + x);
  }
  a[7] = '\0';
  std::clog << a << std::endl;
  std::clog << *a << std::endl;
  utilities::TyPoolAllocator<int, 5> test2;
  int* b = static_cast<int*>(test2.allocate(5));
  *b = 5;
  std::clog << b << std::endl;
  int* c = static_cast<int*>(test2.allocate(5));
#elif 1
    gVector.resize(10000);
    for (auto& x : gVector) {
        x = random_int(0, 1000);
    }
    utilities::ThreadPool<100, void()> a;

    for (auto i{0}; i < 2000; i++) {
        a.addTask(std::function([]() {
            thread_local auto rand_int = random_int(0, 10000);
            std::cout << rand_int << "\n" << std::flush;
            for (auto x{0}; x < 100000000; x++) {
                auto read = gVector[random_int(0, gVector.size() - 1)];
            }
            std::this_thread::sleep_for(10ms);
        }));
    }
    std::this_thread::sleep_for(1000ms);
#endif
    return 0;
}