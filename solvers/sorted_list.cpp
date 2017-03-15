#include <vector>
#include <cstdint>
#include <algorithm>
#include <limits>

using namespace std;

/* Knowing the elems are randomly generated, we know they are evenly distributed.
 * This gives us a good heuristic for choosing a pivot point for quicksort.
 * So we use this function instead of just calling the STL sort right away. */
void quicksort(vector<uint64_t>& elems) {
   constexpr uint64_t pivot = numeric_limits::max();
   auto mid = partition(elems.begin(), elems.end(),
         [pivot](const auto& em){ return em < pivot; });
   sort(elems.begin(), mid);
   sort(mid, elems.end());
}
