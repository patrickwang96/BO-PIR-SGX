#include "pir.h"
#include <cmath>
// #include <random>
#include <algorithm>
#include <cstdlib>
#include <unordered_set>
#include "sgx_trts.h"
#include "sgx_tcrypto.h"
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
    for (int i = 0; i < j - 1; i++) {
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


vector<RecordSet> genLHintSets(RecordSet &db, int l, vector<vector<int>> &S_list) {
    //S_list.reserve(l);
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
            hint = get_hint_index(u[i], S_list[j]);
            if (hashset[j].count(hint)) {
                continue;
            } else {
                ret[i] = extract_query_by_hint(u[i], S_list[j], hint);
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

void decode(char* key, sgx_ec256_private_t* ecc_key, sgx_ecc_state_handle_t handle) {
	Record a = 10;
	for (int i = 0; i < K; i++) {
		Record b = rand();
		a ^= b;
        sgx_cmac_128bit_tag_t hash;
        sgx_rijndael128_cmac_msg(key, &a.to_ulong(), 1, &hash);
        // a.to_ulong ^ hash;
        sgx_ec256_signature_t signature;
        sgx_ecdsa_sign(&a.to_ulong(), 1, ecc_key, &signature, handle);
	}
}

inline double getTimeDelta(uint64_t s1, uint64_t ns1, uint64_t s2, uint64_t ns2) {
    double result = 0;
    result = ((long)ns2 - (long)ns1)/1000000.0 + ((long)s2-(long)s1)*1000.0 ;
    return result;
}

void ecall_pir(void) {
    RecordSet db = genDb(RECORD_COUNT);
	printf("db size is %d\n", db.size());

    vector<vector<int>> S(L);
    vector<RecordSet> hintsets;
    vector<vector<int>> querys;
    vector<int> u(K);

    for (int i = 0; i < K; i++) u[i] = i;

    uint64_t s1, s2, ns1, ns2;

    ocall_get_time(&s1, &ns1);
    hintsets = genLHintSets(db, L, S);
    ocall_get_time(&s2, &ns2);

    double delta = getTimeDelta(s1, ns1, s2, ns2);
    printf("The time for preprocessing is %f ms\n", delta);

    ocall_get_time(&s1, &ns1);
    querys = queryLSets(L, u, S);
    ocall_get_time(&s2, &ns2);

    delta = getTimeDelta(s1, ns1, s2, ns2);
    printf("The time for query is %f ms\n", delta);

    ocall_get_time(&s1, &ns1);
    decode();
    ocall_get_time(&s2, &ns2);

 	delta = getTimeDelta(s1, ns1, s2, ns2);
    printf("Decode time is %f ms\n", delta);


}

void ecall_pir_with_net(void) {
    RecordSet db = genDb(RECORD_COUNT);
    int n = db.size();
    int sqrtn = sqrt(n);
	printf("db size is %d\n", db.size());

    vector<vector<int>> S(L);
    vector<RecordSet> hintsets;
    vector<vector<int>> querys;
    vector<int> u(K);

    char prf_key[12] = {0};

    sgx_ecc_state_handle_t ecc_handle;
    sgx_ecc256_open_context(&ecc_handle);

    sgx_ec256_private_t ecc_private_key;
    sgx_ec256_public_t ecc_public_key;
    sgx_ecc256_create_key_pair(&ecc_private_key, &ecc_public_key, ecc_handle);



    for (int i = 0; i < K; i++) u[i] = i;

    uint64_t s1, s2, ns1, ns2;
    
    ocall_get_time(&s1, &ns1);
    vector<int> total(K * (sqrtn - 1));
    for (int t = 0; t < NUM_TRAIL; t++) {
        hintsets = genLHintSets(db, L, S);
        querys = queryLSets(L, u, S);
        for (int i = 0; i < K; i++) {
            for (int j = 0; j < (sqrtn-1); j++) {
                total[i * (sqrtn-1) + j] = querys[i][j];
            }
        }
        ocall_send((char*)total.data(), total.size() * sizeof(int));

        vector<uint8_t> answer(K);
        ocall_recv((char*)answer.data(), answer.size() * sizeof(uint8_t));
        // sock.read_some(buffer(answer));
        decode(prf_key, &ecc_private_key, ecc_handle);
    }    
    ocall_get_time(&s2, &ns2);
    double delta = getTimeDelta(s1, ns1, s2, ns2);
    printf("Time is %f ms\n", delta);

    sgx_ecc256_close_context(ecc_handle);


}
