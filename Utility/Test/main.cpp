#include <Allocators/PoolAllocator.hpp>
#include <Thread/ThreadPool.hpp>
#include <random>
#include <vector>

static std::vector<int> gVector;

void foo() {}

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
  utilities::ThreadPool<10> a;
  for (auto i{0}; i < 10; i++) {
    a.addTask(std::function([]() {
      for (auto x{0}; x < 100000; x++) {
        std::cout << "Nfopjeappp\n" << std::flush;
      }
    }));
  }
  a.dispatch();
#endif
  return 0;
}