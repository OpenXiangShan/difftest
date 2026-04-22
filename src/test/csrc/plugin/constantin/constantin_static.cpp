/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* XiangShan is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
*
***************************************************************************************/

#ifdef ENABLE_CONSTANTIN

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include <stdint.h>
#include <assert.h>
using namespace std;

extern map<string, uint64_t> constantinMap;
extern void constantinInit();

extern void constantinUpdate(string name, uint64_t num){
  if(constantinMap.find(name) == constantinMap.end()){
    cout << "[ERROR] constant does not exist: " << name << ", please check the input." << endl;
    assert(0);
  }
  constantinMap[name] = num;
  cout << "[INFO] constant updated: " << name << " = " << num << endl;
  return;
}

extern void constantinLoad(const char *cst_file) {
  constantinInit();

  if (cst_file == nullptr || strlen(cst_file) == 0) {
    cout << "[INFO] init for constantin: loaded from init." << endl;
    return;
  }

  uint64_t num;
  string name;
  if (string(cst_file) == "stdin") {
    cout << "[INFO] stdin for constantin: loaded from stdin." << endl;
    uint64_t total_num;
    cout << "Please input total constant number:" << endl;
    cin >> total_num;
    cout << "Please input each constant ([constant name] [value]):" << endl;
    for (uint64_t i = 0; i < total_num; i++) {
      cin >> name >> num;
      constantinUpdate(name, num);
    }
    return;
  }

  ifstream cf(cst_file, ios::in);
  if (cf.good()) {
    cout << "[INFO] file for constantin: loaded from " << cst_file << "." << endl;
    while (cf >> name >> num) {
      constantinUpdate(name, num);
    }
    cf.close();
    return;
  }else{
    cerr << "[ERROR] constantin file does not exist: " << cst_file << endl;
    assert(0);
  }
}

#endif // ENABLE_CONSTANTIN
