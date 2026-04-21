
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

#endif // CONSTANTIN
