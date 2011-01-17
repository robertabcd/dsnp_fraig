/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2010 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdarg>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "cirMgr.h"
#include "cirGate.h"

using namespace std;

// TODO: You are free to define data members and member functions on your own

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
   static int done_srand = 0;
   if(!done_srand) {
      srand(time(NULL));
      done_srand = 1;
   }

   int per_batch = sizeof(gateval_t)*8;

   gateval_t vin[nInputs];
   char **pattern = new char *[per_batch];
   char **result = new char *[per_batch];

   memset(vin, 0, sizeof(gateval_t)*nInputs);
   
   for(int i = 0; i < per_batch; ++i) {
      pattern[i] = new char[nInputs+1024];
      result[i] = new char[nOutputs+1024];
   }

   int sim, failed_count = 0;

   while(failed_count < surrender) {
      sim++;

      for(int i = 0; i < nInputs; ++i)
         vin[i] = rand();

      for(int i = 0; i < nInputs; ++i) {
         gateval_t v = vin[i];
         for(int pid = per_batch-1; pid >= 0; --pid) {
            pattern[pid][i] = (v & 1) ? '1' : '0';
            v >>= 1;
         }
      }
      for(int i = 0; i < per_batch; ++i)
         pattern[i][nInputs] = '\0';

      if(simulate(vin, result) == 0)
         failed_count++;
      else
         failed_count = 0;

      for(int i = 0; i < per_batch; ++i)
         simulationResult(pattern[i], result[i]);
   }

   if(sim > 0 && fec_groups)
      printf("#FEC groups: %d\n", (int)fec_groups->size());
   printf("%d patterns simulated\n", sim*per_batch);

   if(is_debug) printFecGroups();

   for(int i = 0; i < per_batch; ++i) {
      delete pattern[i];
      delete result[i];
   }
   delete[] pattern;
   delete[] result;
}

bool CirMgr::simuationError(const char *msgfmt, ...) {
   va_list args;
   va_start(args, msgfmt);
   if(_simLog) {
      char buf[1024];
      vsprintf(buf, msgfmt, args);
      *_simLog << buf;
   } else
      vfprintf(stderr, msgfmt, args);
   va_end(args);
   return false;
}

void CirMgr::simulationResult(const char *patt, const char *result) {
   if(_simLog) {
      *_simLog << patt << " " << result << endl;
   } else {
      //printf("%s %s\n", patt, result);
   }
}

void CirMgr::fileSim(ifstream &ifs) {
   int per_batch = sizeof(gateval_t)*8;

   char buf[nInputs+1024];
   gateval_t vin[nInputs];
   char **pattern = new char *[per_batch];
   char **result = new char *[per_batch];

   memset(vin, 0, sizeof(gateval_t)*nInputs);
   
   for(int i = 0; i < per_batch; ++i) {
      pattern[i] = new char[nInputs+1024];
      result[i] = new char[nOutputs+1024];
   }

   int sim = 0, in_queue = 0;

   ifs.getline(buf, nInputs+1024);
   while(!ifs.eof()) {
      sim++;
      if(!checkSimulationPattern(buf)) {
         simuationError("Simulation error on line %d\n", sim);
         simuationError("Simulation terminated\n");
         break;
      }

      pushSimulationPattern(buf, vin);
      strcpy(pattern[in_queue], buf);

      if(++in_queue >= per_batch) {
         in_queue = 0;

         simulate(vin, result);

         for(int i = 0; i < per_batch; ++i)
            simulationResult(pattern[i], result[i]);
      }

      ifs.getline(buf, nInputs+1024);
   }

   if(in_queue > 0) {
      simulate(vin, result);

      for(int i = 0; i < in_queue; ++i)
         simulationResult(pattern[i], result[per_batch-in_queue+i]);

      in_queue = 0;
   }

   if(sim > 0 && fec_groups)
      printf("#FEC groups: %d\n", (int)fec_groups->size());
   printf("%d pattern(s) simulated\n", sim);

   if(is_debug) printFecGroups();

   for(int i = 0; i < per_batch; ++i) {
      delete pattern[i];
      delete result[i];
   }
   delete[] pattern;
   delete[] result;
}

bool CirMgr::checkSimulationPattern(const char *patt) {
   int len = strlen(patt);

   if(len != nInputs)
      return simuationError("pattern length not equal to input length\n");

   for(int i = 0; i < len; ++i)
      if(patt[i] != '0' && patt[i] != '1')
         return simuationError("pattern should contains 0/1 only\n");

   return true;
}

void CirMgr::pushSimulationPattern(const char *patt, gateval_t *vin) {
   for(int i = 0; i < nInputs; ++i)
      vin[i] = (vin[i] << 1)|(patt[i] == '1'?1:0);
}

int CirMgr::simulate(gateval_t *vin, char **result) {
   for(int i = 1; i <= nMaxVar; ++i) vars[i]->resetState();
   for(int i = 0; i < nOutputs; ++i) outputs[i]->resetState();

   for(int i = 0; i < nInputs; ++i)
      inputs[i]->setVal(vin[i]);

   int n_patt = sizeof(gateval_t)*8;
   for(int i = 0; i < nOutputs; ++i) {
      gateval_t v = outputs[i]->evaluate();

      if(result) {
         for(int pid = n_patt-1; pid >= 0; --pid) {
            result[pid][i] = (v & 1) ? '1' : '0';
            v >>= 1;
         }
      }
   }
   if(result) {
      for(int pid = n_patt-1; pid >= 0; --pid)
         result[pid][nOutputs] = '\0';
   }

   int ret = FecGrouping();
   printf("#FEC groups: %d\r", (int)fec_groups->size());
   fflush(stdout);

   return ret;
}

