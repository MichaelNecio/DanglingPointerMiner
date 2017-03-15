#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

using namespace std;

/* Knowing the elems are randomly generated, we know they are evenly distributed.
 * This gives us a good heuristic for choosing a pivot point for quicksort.
 * So we use this function instead of just calling the STL sort right away. */
void quicksort(vector<uint64_t>& elems) {
   constexpr uint64_t pivot = numeric_limits<uint32_t>::max() / 2;
   auto mid = partition(elems.begin(), elems.end(),
         [pivot](const auto& em){ return em < pivot; });
   sort(elems.begin(), mid);
   sort(mid, elems.end());
}

uint64_t gen_seed(string last_hash, string nonce) {
   //TODO: The buisness with the SHA256 algorithm
   return 1337;
} 

vector<uint64_t> gen_elems(size_t nb_elements, uint64_t seed) {
   mt19937_64 twister(seed);   
   vector<uint64_t> elems(nb_elements);
   generate_n(elems.begin(), nb_elements, twister);
   return elems;
}

bool check_prefix(string given_prefix, string found_prefix) {
   //TODO: The buisness with the checking of the final solutions
   return true;
}

void submit_nonce(string nonce) {
   //TODO: The buisness with the Unix IPC
   cout << nonce << endl;
}

int main(int argc, char *argv[]) {
   if(argc <= 3) {
      cout << "Usage: sorted_list [last_hash] [given_prefix] [nb_elements]" << endl;
      return -1;
   }
   return 0;
}

