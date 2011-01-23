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

//unsigned CirAigGate::_globalRef_s = 0;

void CirVar::setType(GateType t) {
   switch(type = t) {
      case UNDEF_GATE: str_type = "UNDEF"; break;
      case PI_GATE:    str_type = "PI";    break;
      case PO_GATE:    str_type = "PO";    break;
      case AIG_GATE:   str_type = "AIG";   break;
      case CONST_GATE: str_type = "CONST"; break;
      default: assert(!"invalid gate type"); break;
   }
}

gateval_t CirVar::updateValue() {
   assert(!simulating);

   gateval_t v0;
   dirty = false;
   switch(type) {
      case UNDEF_GATE:
         return val = 0;
      case PI_GATE:
         return val;
      case PO_GATE:
         simulating = true;
         val = mgr.getVarDirectly(in0>>1)->evaluate();
         if(in0&1) val = ~val;
         simulating = false;
         return val;
      case AIG_GATE:
         simulating = true;
         v0 = mgr.getVarDirectly(in0>>1)->evaluate();
         if(in0&1) v0 = ~v0;
         val = mgr.getVarDirectly(in1>>1)->evaluate();
         if(in1&1) val = ~val;
         simulating = false;
         return val &= v0;
      case CONST_GATE:
         return val = 0;
      default:
         return val = 0;
   }
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
   printf("= %s\n", buf);

   printf("= FECs:");
   int myid = getFecLiteral();
   const vector<int> *fec = mgr.getFecGroup(getFecGroupId());
   if(fec) {
      for(int i = 0, n = fec->size(); i < n; ++i) {
         int id = fec->at(i);
         if(id == myid) continue;
         printf(" %s%d", (((id^myid)&1)?"!":""), id>>1);
      }
      if(fec->size() == 0) printf("<none>");
      printf("\n");
   } else
      printf("<not yet simulated>\n");

   sprintf(buf, "Value: ");
   char *p = &buf[strlen(buf)];
   for(int i = sizeof(gateval_t)*8-1; i >= 0; --i)
      *(p++) = ((val >> i) & 1) ? '1' : '0';
   printf("= %s\n", buf);

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

