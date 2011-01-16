/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2010 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <stdarg.h>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: You are free to define data members and member functions on your own

unsigned int CirVar::s_MaxSimHistory = 32;

//unsigned CirAigGate::_globalRef_s = 0;

bool CirPO::evaluate() {
   if(dirty) {
      val = mgr.getVar(in0/2)->evaluate();
      if(in0 & 1) val = !val;
      dirty = false;
   }
   return val;
}

void CirGate::setFanin(int in0, int in1) {
   //if(in0 > in1)
      //setFanin(in1, in0);
   //else {
      this->in0 = in0;
      this->in1 = in1;
      resetState();
   //}
}

bool CirGate::evaluate() {
   if(dirty) {
      bool v0 = mgr.getVar(in0/2)->evaluate();
      if(in0 & 1) v0 = !v0;
      if(v0) {
         bool v1 = mgr.getVar(in1/2)->evaluate();
         if(in1 & 1) v1 = !v1;
         val = v1;
      } else
         val = false;
      dirty = false;
   }
   return val;
}

char *CirVar::getSimHistoryStr() const {
   static char buf[1024];

   unsigned int n = sim_history.size();

   for(unsigned int i = 0; i < n; ++i)
      buf[i] = (sim_history[i] ? '1' : '0');
   buf[n] = '\0';

   return buf;
}

void
CirVar::reportGate() const
{
   char buf[1024];

   printf("==================================================\n");
   if(has_symbol) {
      sprintf(buf, "%s(%d)\"%s\", line %d", getTypeStr().c_str(), 
            getVarId(), symbol.c_str(), getLine());
   } else {
      sprintf(buf, "%s(%d), line %d", getTypeStr().c_str(), 
            getVarId(), getLine());
   }
   printf("= %-46s =\n", buf);
   sprintf(buf, "FECs: ???");
   printf("= %-46s =\n", buf);
   sprintf(buf, "Value: %s", 
         (sim_history.size() > 0)?getSimHistoryStr():"<not yet simulated>");
   printf("= %-46s =\n", buf);
   printf("==================================================\n");
}

void
CirVar::reportFanin(unsigned level) const
{
   reportFaninDFS(0, level, false);
}

void CirVar::reportFaninDFS(int level, int maxlevel, bool inverted) const {
   static set<int> reported;

   if(level > maxlevel) return;
   if(level == 0) reported.clear();

   int varid = getVarId();

   for(int i = 0; i < level; ++i) printf("  ");
   printf("%s%s %d", inverted?"!":"", getTypeStr().c_str(), varid);
   if(hasSymbol()) printf(" (%s)", getSymbol().c_str());

   if(reported.find(varid) == reported.end()) {
      printf("\n");
      reported.insert(varid);

      if(getType() == AIG_GATE) {
         mgr.getVar(getIN0()/2)->reportFaninDFS(level+1, maxlevel, getIN0()&1);
         mgr.getVar(getIN1()/2)->reportFaninDFS(level+1, maxlevel, getIN1()&1);
      } else if(getType() == PO_GATE) {
         mgr.getVar(getIN0()/2)->reportFaninDFS(level+1, maxlevel, getIN0()&1);
      }
   } else if(getType() == AIG_GATE)
      printf(" (*)\n");
   else
      printf("\n");
}

void
CirVar::reportFanout(unsigned level) const
{
   reportFanoutDFS(0, level, -1);
}

void CirVar::reportFanoutDFS(int level, int maxlevel, int caller) const {
   static set<int> reported;

   if(level > maxlevel) return;
   if(level == 0) reported.clear();

   bool inverted = false;
   if(getType() == AIG_GATE) {
      if(getIN0()/2 == caller && (getIN0() & 1)) inverted = true;
      else if(getIN1()/2 == caller && (getIN1() & 1)) inverted = true;
   } else if(getType() == PO_GATE) {
      if(getIN0()/2 == caller && (getIN0() & 1)) inverted = true;
   }

   int varid = getVarId();

   for(int i = 0; i < level; ++i) printf("  ");
   printf("%s%s %d", inverted?"!":"", getTypeStr().c_str(), varid);
   if(hasSymbol()) printf(" (%s)", getSymbol().c_str());

   if(reported.find(varid) == reported.end()) {
      printf("\n");

      reported.insert(varid);

      const multiset<int> *rev = mgr.queryVarRevRef(getVarId());

      for(multiset<int>::const_iterator it = rev->begin(), ed = rev->end();
            it != ed; ++it) {

         mgr.getVar(*it)->reportFanoutDFS(level + 1, maxlevel, varid);
      }
   } else if(getType() == AIG_GATE)
      printf(" (*)\n");
   else
      printf("\n");
}

