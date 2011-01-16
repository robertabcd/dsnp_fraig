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

   char patt[nInputs+4];
   char res[nOutputs+4];

   int sim;

   for(sim = 0; sim < 128; ++sim) {
      for(int i = 0; i < nInputs; ++i)
         patt[i] = (rand() & 1) ? '1' : '0';
      patt[nInputs] = '\0';

      simulatePattern(patt, res);
      simulationResult(patt, res);
   }
   printf("%d patterns simulated\n", sim);
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
   if(_simLog)
      *_simLog << patt << " " << result << endl;
   else
      printf("%s %s\n", patt, result);
}

void CirMgr::fileSim(ifstream &ifs) {
   char buf[nInputs+1024];
   char res[nOutputs+1024];
   int sim = 0;
   ifs.getline(buf, nInputs+1024);
   while(!ifs.eof()) {
      sim++;
      if(!simulatePattern(buf, res)) {
         simuationError("Simulation error on line %d\n", sim);
         simuationError("Simulation terminated\n");
         break;
      }
      simulationResult(buf, res);

      ifs.getline(buf, nInputs+1024);
   }
   printf("%d pattern(s) simulated\n", sim);
}

bool CirMgr::simulatePattern(const char *patt, char *result) {
   int len = strlen(patt);

   if(len != nInputs)
      return simuationError("pattern length not equal to input length\n");

   for(int i = 0; i < len; ++i)
      if(patt[i] != '0' && patt[i] != '1')
         return simuationError("pattern should contains 0/1 only\n");

   for(int i = 1; i <= nMaxVar; ++i) vars[i]->resetState();
   for(int i = 0; i < nOutputs; ++i) outputs[i]->resetState();

   for(int i = 0; i < len; ++i)
      inputs[i]->setVal(patt[i] == '1');

   for(int i = 0; i < nOutputs; ++i)
      result[i] = outputs[i]->evaluate() ? '1' : '0';
   result[nOutputs] = '\0';

   for(int i = 0; i <= nMaxVar; ++i)
      if(!vars[i]->isRemoved())
         vars[i]->pushSimulatedVal();
   for(int i = 0; i < nOutputs; ++i)
      outputs[i]->pushSimulatedVal();

   return true;
}

