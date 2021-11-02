#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define RAMSIZE (128 * 1024 * 1024)
#define PRED_WIDTH 16
#define MAX_BR_NUM 20000000

using namespace std;
int reset =1;

#define BRANCH_TYPE_BR 0
#define BRANCH_TYPE_JR 1

typedef struct branch_record {
  uint64_t pc;
  uint64_t target;
  uint8_t type;
  uint8_t taken;
} BR;

BR *record;

inline void output_branch_record(BR rec, int idx) {
  printf("br[%d]: pc(0x%lx), target(0x%lx), type(%d), taken(%d)\n", idx, rec.pc, rec.target, rec.type, rec.taken);
}

inline void print_record() {
  int idx = 0;
  BR *rec = record;
  while (rec->pc != 0) {
    output_branch_record(*rec, idx);
    idx++;
    rec++;
  }
}

string Trim(string& str)
{
	str.erase(0,str.find_first_not_of(" \t\r\n"));
	str.erase(str.find_last_not_of(" \t\r\n") + 1);
	return str;
}


// reads branch records into global arrays
void init_branch_record(const char *branch, const uint64_t rate) {
  if (branch == NULL) {
    printf("Branch trace file not provided, oracle branch should not work!\n");
    return ;
  }
  // assert(branch != NULL);
  record = (BR *)(malloc(MAX_BR_NUM * sizeof(BR)));
  // initiate the whole buffer to zero
  memset((void *) record, 0, MAX_BR_NUM * sizeof(BR));

  ifstream fin(branch);
  printf("Use %s as the branch golden trace\n",branch);
  string line;
  int idx = 0;
  while (getline(fin, line)) {
    istringstream sin(line); //将整行字符串line读入到字符串流istringstream中
    vector<string> fields; //声明一个字符串向量
    string field;
    while (getline(sin, field, ',')) { fields.push_back(field); }
    string pc = Trim(fields[0]);
    string taken = Trim(fields[1]);
    string type = Trim(fields[2]);
    string target = Trim(fields[3]);
    stringstream ss, ss1;
    ss << std::hex << pc;
    ss >> record[idx].pc;
    ss1 << std::hex << target;
    ss1 >> record[idx].target;
    record[idx].taken = taken[0] - '0';
    record[idx].type = type[0] - '0';
    idx++;
  }

  int miss_rate = rate;
  // default set to zero
  if (miss_rate < 0) miss_rate = 0;
  int num_reverted = 0;
  if (miss_rate > 0) {
    srand( (unsigned)time( NULL ) );
    for (int i = 0; i < idx; i++) {
      // revert as a rate of miss_rate
      if (rand() % 100 < miss_rate) {
        record[i].taken = ~record[i].taken;
        num_reverted++;
      }
    }
  }
  printf("Branch miss rate is set to %d%%, totally %d predictions are reverted\n", miss_rate, num_reverted);
  reset = 0;
}

void free_branch_record(){
  free(record);
}

// int main() {
//   init_branch_record("");
//   return 0;
// }

// TODO: read branch record arrays using idx, and give pc for validation
// read rNum records from array starting at rIdx
// the management of rIdx is left to BPU
// we don't need rNum because we always want to read records in order
extern "C" void branch_prediction_helper(
    uint64_t rIdx,
    uint64_t *target1,  uint64_t *target2,
    uint64_t *target3,  uint64_t *target4,
    uint64_t *target5,  uint64_t *target6,
    uint64_t *target7,  uint64_t *target8,
    uint64_t *target9,  uint64_t *target10,
    uint64_t *target11,  uint64_t *target12,
    uint64_t *target13,  uint64_t *target14,
    uint64_t *target15,  uint64_t *target16,
    uint64_t *pc1,  uint64_t *pc2,
    uint64_t *pc3,  uint64_t *pc4,
    uint64_t *pc5,  uint64_t *pc6,
    uint64_t *pc7,  uint64_t *pc8,
    uint64_t *pc9,  uint64_t *pc10,
    uint64_t *pc11,  uint64_t *pc12,
    uint64_t *pc13,  uint64_t *pc14,
    uint64_t *pc15,  uint64_t *pc16,
    uint8_t *taken1,  uint8_t *taken2,
    uint8_t *taken3,  uint8_t *taken4,
    uint8_t *taken5,  uint8_t *taken6,
    uint8_t *taken7,  uint8_t *taken8,
    uint8_t *taken9,  uint8_t *taken10,
    uint8_t *taken11,  uint8_t *taken12,
    uint8_t *taken13,  uint8_t *taken14,
    uint8_t *taken15,  uint8_t *taken16,
    uint8_t *type1, uint8_t *type2,
    uint8_t *type3, uint8_t *type4,
    uint8_t *type5, uint8_t *type6,
    uint8_t *type7, uint8_t *type8,
    uint8_t *type9, uint8_t *type10,
    uint8_t *type11, uint8_t *type12,
    uint8_t *type13, uint8_t *type14,
    uint8_t *type15, uint8_t *type16,
    uint64_t redirectIdx,
    uint64_t *redirectpc1, uint64_t *redirectpc2, uint64_t *redirectpc3, uint64_t *redirectpc4, uint64_t *redirectpc5, uint64_t *redirectpc6, uint64_t *redirectpc7, uint64_t *redirectpc8, 
    uint64_t *redirectpc9, uint64_t *redirectpc10, uint64_t *redirectpc11, uint64_t *redirectpc12, uint64_t *redirectpc13, uint64_t *redirectpc14, uint64_t *redirectpc15, uint64_t *redirectpc16
    ) {
  if (reset) return;

  if (rIdx >= MAX_BR_NUM) {
    printf("ERROR: branch record idx = %ld out of bound!\n", rIdx);
    return;
  }
  if (redirectIdx >= MAX_BR_NUM) {
    printf("Error: branch redirect record idx = %ld out of bound!\n", redirectIdx);
    return;
  }
  //printf("-- rIdx :%ld pc: %lx taken:%d\n",rIdx,record[rIdx].pc,record[rIdx].taken);
  *taken1 = record[rIdx].taken;
  *taken2 = record[rIdx + 1].taken;
  *taken3 = record[rIdx + 2].taken;
  *taken4 = record[rIdx + 3].taken;
  *taken5 = record[rIdx + 4].taken;
  *taken6 = record[rIdx + 5].taken;
  *taken7 = record[rIdx + 6].taken;
  *taken8 = record[rIdx + 7].taken;
  *taken9 = record[rIdx + 8].taken;
  *taken10 = record[rIdx + 9].taken;
  *taken11 = record[rIdx + 10].taken;
  *taken12 = record[rIdx + 11].taken;
  *taken13 = record[rIdx + 12].taken;
  *taken14 = record[rIdx + 13].taken;
  *taken15 = record[rIdx + 14].taken;
  *taken16 = record[rIdx + 15].taken;
  *pc1 = record[rIdx].pc;
  *pc2 = record[rIdx + 1].pc;
  *pc3 = record[rIdx + 2].pc;
  *pc4 = record[rIdx + 3].pc;
  *pc5 = record[rIdx + 4].pc;
  *pc6 = record[rIdx + 5].pc;
  *pc7 = record[rIdx + 6].pc;
  *pc8 = record[rIdx + 7].pc;
  *pc9 = record[rIdx + 8].pc;
  *pc10 = record[rIdx + 9].pc;
  *pc11 = record[rIdx + 10].pc;
  *pc12 = record[rIdx + 11].pc;
  *pc13 = record[rIdx + 12].pc;
  *pc14 = record[rIdx + 13].pc;
  *pc15 = record[rIdx + 14].pc;
  *pc16 = record[rIdx + 15].pc;
  *target1 = record[rIdx].target;
  *target2 = record[rIdx + 1].target;
  *target3 = record[rIdx + 2].target;
  *target4 = record[rIdx + 3].target;
  *target5 = record[rIdx + 4].target;
  *target6 = record[rIdx + 5].target;
  *target7 = record[rIdx + 6].target;
  *target8 = record[rIdx + 7].target;
  *target9 = record[rIdx + 8].target;
  *target10 = record[rIdx + 9].target;
  *target11 = record[rIdx + 10].target;
  *target12 = record[rIdx + 11].target;
  *target13 = record[rIdx + 12].target;
  *target14 = record[rIdx + 13].target;
  *target15 = record[rIdx + 14].target;
  *target16 = record[rIdx + 15].target;
  *type1 = record[rIdx].type;
  *type2 = record[rIdx + 1].type;
  *type3 = record[rIdx + 2].type;
  *type4 = record[rIdx + 3].type;
  *type5 = record[rIdx + 4].type;
  *type6 = record[rIdx + 5].type;
  *type7 = record[rIdx + 6].type;
  *type8 = record[rIdx + 7].type;
  *type9 = record[rIdx + 8].type;
  *type10 = record[rIdx + 9].type;
  *type11 = record[rIdx + 10].type;
  *type12 = record[rIdx + 11].type;
  *type13 = record[rIdx + 12].type;
  *type14 = record[rIdx + 13].type;
  *type15 = record[rIdx + 14].type;
  *type16 = record[rIdx + 15].type;
  *redirectpc1 = record[redirectIdx].pc;
  *redirectpc2 = record[redirectIdx + 1].pc;
  *redirectpc3 = record[redirectIdx + 2].pc;
  *redirectpc4 = record[redirectIdx + 3].pc;
  *redirectpc5 = record[redirectIdx + 4].pc;
  *redirectpc6 = record[redirectIdx + 5].pc;
  *redirectpc7 = record[redirectIdx + 6].pc;
  *redirectpc8 = record[redirectIdx + 7].pc;
  *redirectpc9 = record[redirectIdx + 8].pc;
  *redirectpc10 = record[redirectIdx + 9].pc;
  *redirectpc11 = record[redirectIdx + 10].pc;
  *redirectpc12 = record[redirectIdx + 11].pc;
  *redirectpc13 = record[redirectIdx + 12].pc;
  *redirectpc14 = record[redirectIdx + 13].pc;
  *redirectpc15 = record[redirectIdx + 14].pc;
  *redirectpc16 = record[redirectIdx + 15].pc;
}
