//
// Created by Ruochen WANG on 20/4/2020.
//

#ifndef BO_PIR_PIR_H
#define BO_PIR_PIR_H

#include <bitset>
#include <vector>
// #include <unordered_map>

#include "Enclave.h"
#include "Enclave_t.h"

#define RECORD_LEN (1)
#define K 16
#define L (K/2)
#define RECORD_COUNT (50)

typedef std::bitset<RECORD_LEN> Record;

typedef std::vector<Record> RecordSet;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

std::vector<int> genS(RecordSet &db);

RecordSet genHints(const std::vector<int> &S, RecordSet &db);

RecordSet preprocessing(RecordSet &db);

//RecordSet query(u64 u);

std::vector<int> query(u64 u, std::vector<int> &S, int &hint_index);

Record answer(std::vector<int> s, RecordSet &db);


std::vector<RecordSet> genLHintSets(RecordSet &db, int l, std::vector<std::vector<int>> &S_list);

std::vector<std::vector<int>>
queryLSets(int l, std::vector<int> u, std::vector<std::vector<int>> &S_list);

RecordSet genDb(int n);

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_preprocessing(void);

#if defined(__cplusplus)
}
#endif


#endif //BO_PIR_PIR_H
