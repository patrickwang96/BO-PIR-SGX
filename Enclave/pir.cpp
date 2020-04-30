#include "pir.h"
#include <cmath>
// #include <random>
#include <algorithm>
#include <cstdlib>
#include <unordered_set>
#include "sgx_trts.h"
// #include <iostream>

using namespace std;

uint32_t rand() {
    uint32_t buf;
    sgx_read_rand((unsigned char *) &buf, 4);
    return buf;
}

RecordSet genDb(int n) {
    RecordSet db(n);
    for (auto r: db) r = rand() % 2;
    return db;
}

void shuffle_vector(vector<int> &nums) {
    int n = nums.size();
    for (int i = n - 1; i > 0; i--) {
        swap(nums[i], nums[rand() % (i + 1)]);
    }
}

vector<int> genS(RecordSet &db) {
    int n = db.size();
    int j = sqrt(n);

    vector<int> index(n);
    for (int i = 0; i < n; i++) index[i] = i;
//    shuffle(index.begin(), index.end(), rng);
    shuffle_vector(index);
    return index;

}

RecordSet genHints(const vector<int> &S, RecordSet &db) {
    int n = S.size();
    int j = sqrt(n);
    RecordSet hints(j);

    for (int i = 0; i < j; i++) {
        Record sum = 0;
        for (int k = 0; k < j; k++) {
            sum ^= db[S[i * j + k]];
        }
        hints.push_back(sum);
    }
    return hints;
}

RecordSet preprocessing(RecordSet &db) {
    vector<int> S = genS(db);
    RecordSet hints = genHints(S, db);
    return hints;
}


int get_hint_index(int u, vector<int> &S) {
    int hint_index;
    int n = S.size();
    int j = sqrt(n);

    for (hint_index = 0; hint_index < j; hint_index++) {
        for (int i = 0; i < j; i++) {
            if (S[hint_index * j + i] == u) return hint_index;
        }
    }
    return hint_index;
}

bool belongToHint(int u, vector<int> &S, int hint_index) {
    int n = S.size();
    int j = sqrt(n);

    for (int i = 0; i < j; i++) {
        if (S[hint_index * j + i] == u) return true;
    }
    return false;
}

vector<int> extract_query_by_hint(int u, vector<int> &S, int hint_index) {
    int n = S.size();
    int j = sqrt(n);

    vector<int> ret(j -1);
    for (int i = 0; i < j; i++) {
        if (S[hint_index * j + i] == u) continue;
        ret[i] = S[hint_index * j + i];
    }
    return ret;
}

vector<int> query(u64 u, vector<int> &S, int &hint_index) {
    int n = S.size();
    int j = sqrt(n);
    hint_index = get_hint_index(u, S);

    return extract_query_by_hint(u, S, hint_index);
//    return ret;
    // TODO return a probability of decoy index
}


Record answer(vector<int> s, RecordSet &db) {
    Record a;
    for (auto r: s) a ^= db[r];
    return a;
}


Record decode(Record a, u64 u, unordered_map<int, int> &map) {
    u64 index = map[u];
    Record h_j;
    return h_j ^ a;
}

vector<RecordSet> genLHintSets(RecordSet &db, int l, vector<vector<int>> &S_list) {
    S_list.reserve(l);
    vector<RecordSet> hintset(l);

    for (int i = 0; i < l; i++) {
        S_list[i] = genS(db);
        hintset[i] = genHints(S_list[i], db);
    }
    return hintset;
}

vector<vector<int>>
queryLSets(int l, vector<int> u, vector<vector<int>> &S_list) {

    vector<vector<int>> ret(K);
    vector<unordered_set<int>> hashset(l);
    vector<int> failure;

    int hint;
    for (int i = 0; i < l; i++) {
        ret[i] = query(u[i], S_list[i], hint);
        unordered_set<int> cur = {hint};
        hashset[i] = cur;
    }
    for (int i = l; i < K; i++) {
        // check is rest u indexs
        bool found = false;
        for (int j = 0; j < l; j++) {
            hint = get_hint_index(u[i], S_list[l]);
            if (hashset[j].count(hint)) {
                continue;
            } else {
                ret[i] = extract_query_by_hint(u[i], S_list[l], hint);
                found = true;
                break;
            }

        }
        if (!found) failure.push_back(u[i]);
    }
    // cout << "number of failure " << failure.size() << endl;
    printf("Query: Number of failure is %d\n", failure.size());
    return ret;
}

inline double getTimeDelta(uint64_t s1, uint64_t ns1, uint64_t s2, uint64_t ns2) {
    double result = 0;
    result = ns2 - ns1 + (s2 - s1) * 1000000000.0;
    return result / 1000000.0;
}

void ecall_pir(void) {
    RecordSet db = genDb(RECORD_COUNT);

    vector<vector<int>> S;
    vector<RecordSet> hintsets;
    vector<vector<int>> querys;
    vector<int> u(K);

    for (int i = 0; i < K; i++) u[i] = i;

    uint64_t s1, s2, ns1, ns2;

    void ocall_get_time(&s1, &ns1);
    hintsets = genLHintSets(db, L, S);
    void ocall_get_time(&s2, &ns2);

    double delta = getTimeDelta(s1, ns2, s2, ns2);
    printf("The time for preprocessing is %f ms\n", delta);

    void ocall_get_time(&s1, &ns1);
    querys = queryLSets(L, u, S);
    void ocall_get_time(&s2, &ns2);

    delta = getTimeDelta(s1, ns2, s2, ns2);
    printf("The time for query is %f ms\n", delta);

}