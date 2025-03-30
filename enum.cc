#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <random>
#include <span>
#include <sstream>
#include <thread>

using namespace std;

const int N = 17;
long double tau;
long double C[N];
int T[N];

const int THREAD_COUNT = 40;
const int LOG_STEP = 10000;

long double g_R_sum{numeric_limits<long double>::infinity()};
long long g_count{0};
mutex g_mtx;

// Response time analysis for current P[]

std::optional<long double> check(const int P[N]) {
  long double R_sum = 0;
  for (int i = 0; i < N; ++i) {
    int pi = P[i];

    // Find B = max C where Pj >= Pi
    long double B = 0;
    for (int j = 0; j < N; ++j) {
      if (P[j] >= pi) {
        B = max(B, C[j]);
      }
    }

    long double Q = B, RHS;
    while (true) {
      RHS = B;
      for (int j = 0; j < N; ++j) {
        if (P[j] < pi) {
          RHS += ceil((Q + tau) / T[j]) * C[j];
        }
      }
      if (Q == RHS)
        break;
      Q = RHS;
    }

    long double R = Q + C[i];
    if (R > T[i])
      return nullopt;
    R_sum += R;
  }
  return R_sum;
}

constexpr unsigned long long factorial(unsigned int n) {
  unsigned long long result = 1;
  for (unsigned int i = 2; i <= n; ++i) {
    result *= i;
  }
  return result;
}

unsigned long long f17 = factorial(17);

void read_data() {
  ifstream file("myinput.dat");

  int n;
  file >> n >> tau;

  for (int i = 0; i < N; ++i) {
    long double c;
    int t;
    file >> c >> t;
    C[i] = c;
    T[i] = t;
  }
}

bool in(const int elem, const vector<int> v) {
  return find(v.begin(), v.end(), elem) != v.end();
}

void enumerate_by_prefix(const vector<int> &prefix) {
  int P[N];
  int idx = prefix.size();

  // init p
  copy(prefix.begin(), prefix.end(), P);
  for (int i = 0; i < N; ++i) {
    if (!in(i, prefix))
      P[idx++] = i;
  }

  int counter = 0;
  long double R_sum = numeric_limits<long double>::infinity();
  // cout << "prefix size " << prefix.size() << endl;
  /*
  for (int i = 0; i < N; ++i)
    cout << P[i] << " ";
  cout << endl;
  */

  // mt19937 rng(random_device{}());
  do {
    // shuffle(P, P + N, rng);
    /*
for (int i = 0; i < N; ++i)
cout << P[i] << " ";
    */
    auto sum = check(P);
    if (sum && *sum < R_sum) {
      R_sum = *sum;
    }

    if (++counter % LOG_STEP == 0) {
      lock_guard<mutex> guard(g_mtx);
      g_count += LOG_STEP;
      if (R_sum < g_R_sum)
        g_R_sum = R_sum;
      else
        R_sum = g_R_sum;
      cout << g_count << ": " << g_R_sum << "\n";
    }

  } while (next_permutation(P + prefix.size(), P + N));

  lock_guard<mutex> guard(g_mtx);
  g_count += counter;
  if (R_sum < g_R_sum)
    g_R_sum = R_sum;
  else
    R_sum = g_R_sum;
  cout << g_count << ": " << g_R_sum << "\n";
}

void enumerate(span<vector<int>> prefixes) {
  for (auto p : prefixes)
    enumerate_by_prefix(p);
}

void load_prefixes(vector<vector<int>> &prefixes, char *filename) {
  cout << "load prefixes from " << filename << endl;
  string line;
  ifstream f(filename);
  while (getline(f, line)) {
    stringstream ss(line);
    vector<int> prios;
    int prio;
    while (ss >> prio) {
      prios.push_back(prio);
    }
    prefixes.push_back(std::move(prios));
  }
  cout << "Successfully loaded " << prefixes.size() << " prefixes" << endl;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "Usage: enum {prefix_file}" << endl;
    return 1;
  }
  char *prefix_file = argv[1];
  vector<vector<int>> prefixes; // = load_prefixes();
  load_prefixes(prefixes, prefix_file);

  read_data();
  // for (int i = 0; i < N; ++i) cout << C[i] << " " << T[i] << endl;

  thread threads[THREAD_COUNT];

  const int chunk_size = prefixes.size() / THREAD_COUNT;
  assert(prefixes.size() % THREAD_COUNT == 0);
  for (int i = 0; i < THREAD_COUNT; ++i) {
    auto st = prefixes.begin() + chunk_size * i;
    threads[i] = thread(enumerate, span(st, st + chunk_size));
  }

  for (int i = 0; i < THREAD_COUNT; ++i) {
    threads[i].join();
  }

  return 0;
}
