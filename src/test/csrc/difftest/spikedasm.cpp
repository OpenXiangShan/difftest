/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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
***************************************************************************************/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int test_spike()
{
  return system("echo \"DASM(deadbeef)\" | spike-dasm > /dev/null");
}

void execute_cmd(const char *cmd, char *result)   
{   
  char buf_ps[1024];   
  char ps[1024]={0};   
  FILE *ptr;   
  strcpy(ps, cmd);   
  if((ptr=popen(ps, "r"))!=NULL)   
  {   
    while(fgets(buf_ps, 1024, ptr)!=NULL)   
    {   
       strcat(result, buf_ps);   
       if(strlen(result)>1024)   
         break;   
    }   
    pclose(ptr);   
    ptr = NULL;   
  }   
  else  
  {   
    printf("popen %s error\n", ps);   
  }   
}  

void spike_dasm(char* result, char* input)
{
  char cmd[1024] = {0};
  strcat(cmd, "echo \"DASM(");
  strcat(cmd, input);
  strcat(cmd, ")\" | spike-dasm");
  // printf("%s ", cmd);
  execute_cmd(cmd, result);
  // printf("%s\n", result);
  // remove \n
  char* first_n_occ = strpbrk(result, "\n");
  if(first_n_occ)
    *first_n_occ = '\0';
}

int usage()
{
  char input[] = "10500073";
  char dasm_result[64] = {0};

  int spike_invalid = test_spike();
  if(!spike_invalid)
    spike_dasm(dasm_result, input);
    
  printf("%s", dasm_result);
  return 0;
}