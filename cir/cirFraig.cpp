/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2010 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include "cirMgr.h"
#include "cirGate.h"
#include "myHash.h"

using namespace std;

// TODO: You are free to define data members and member functions on your own

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
void
CirMgr::strash()
{
   Hash<VarHashKey, int> hash(127);
   bool vis[nMaxVar+1];
   memset(vis, 0, sizeof(bool) * (nMaxVar+1));

   for(int i = 0; i < nOutputs; ++i)
      outputs[i]->setIN0(strashDFS(vis, hash, outputs[i]->getIN0()));

   calculateRefCount();
   removeUnrefGates();
}

int CirMgr::strashDFS(bool *visited, Hash<VarHashKey, int> &h, int litid) {
   CirVar *v = getVar(litid/2);

   if(v->getType() != AIG_GATE) return litid;

   if(!visited[litid/2]) {
      visited[litid/2] = true;
      v->setIN0(strashDFS(visited, h, v->getIN0()));
      v->setIN1(strashDFS(visited, h, v->getIN1()));
   }

   VarHashKey k(v);
   int identical_varid;
   if(h.check(k, identical_varid)) {
      printf("Merge %d to %d\n", litid/2, identical_varid);
      return (identical_varid << 1) | (litid & 1);
   } else {
      h.forceInsert(k, litid/2);
      return litid;
   }
}

class cmp_depvar {
public:
   bool operator()(const pair<string, int> &a, const pair<string, int> &b) {
      return a.first < b.first;
   }
};

void CirMgr::FecGrouping() {
   if(!fec_groups) {
      vector<int> *s = new vector<int>();
      for(int i = 0; i <= nMaxVar; ++i)
         s->push_back(i);

      fec_groups = new FECGrp();
      fec_groups->push_back(s);
   }

   FECGrp *fec_new = new FECGrp();

   while(fec_groups->size() > 0) {
      vector<int> *s = fec_groups->back();
      fec_groups->pop_back();
      vector<int> *s0 = new vector<int>();
      vector<int> *s1 = new vector<int>();

      for(vector<int>::iterator it = s->begin(), ed = s->end();
            it != ed; ++it) {
         if(getVar(*it)->evaluate())
            s1->push_back(*it);
         else
            s0->push_back(*it);
      }

      if(s0->size() <= 1)
         delete s0;
      else
         fec_new->push_back(s0);

      if(s1->size() <= 1)
         delete s1;
      else
         fec_new->push_back(s1);

      delete s;
   }

   printf("FEC groups: %d\n", (int)fec_new->size());

   delete fec_groups;
   fec_groups = fec_new;
}

void CirMgr::initFecGrps() {
   /*
   bool vis[nMaxVar+1];
   pair<string, int> *depend_var = new pair<string, int>[nMaxVar+1];
   string allzero(nMaxVar+1, '0');

   memset(vis, 0, sizeof(bool) * (nMaxVar+1));

   // init const 0
   vis[0] = true;
   depend_var[0] = make_pair(allzero, 0);
   depend_var[0].first[0] = '1';

   // init all inputs
   for(int i = 0; i < nInputs; ++i) {
      int varid = inputs[i]->getVarId();
      depend_var[varid] = make_pair(allzero, i);
      depend_var[varid].first[varid] = '1';
      vis[varid] = true;
   }

   // recursively fill in all
   for(int i = 0; i < nOutputs; ++i)
      initFecGrpsDFS(vis, depend_var, outputs[i]->getIN0()/2);

   if(is_debug) {
      for(int i = 0; i <= nMaxVar; ++i)
         printf("V[%2d] -> %s\n", i, depend_var[i].first.c_str());
   }

   sort(depend_var, depend_var+nMaxVar+1, cmp_depvar());

   if(fec_groups) delete fec_groups;
   fec_groups = new FECGrp();

   int last = 0, curr_grpid = -1;
   for(int i = 1; i <= nMaxVar; ++i) {
      if(vars[i]->getType() == AIG_GATE) {
         if(depend_var[last] != depend_var[i]) {
            curr_grpid++;
            fec_groups->push_back(set<int>());
         }
         fec_groups->at(curr_grpid).insert(i);
         last = i;
      }
   }

   if(is_debug) {
      for(int i = 0; i <= curr_grpid; ++i)
         printf("G[%2d] -> size: %d\n", i, (int)fec_groups->at(i).size());
   }

   delete[] depend_var;
   */
}

void CirMgr::initFecGrpsDFS(bool *visited, 
      pair<string, int> *dep_var, int varid) {
   if(visited[varid]) return;
   visited[varid] = true;

   CirVar *v = vars[varid];
   if(v->getType() == UNDEF_GATE) {
      dep_var[varid].first[varid] = '1';
      return;
   }

   int v0 = v->getIN0()/2, v1 = v->getIN1()/2;
   initFecGrpsDFS(visited, dep_var, v0);
   initFecGrpsDFS(visited, dep_var, v1);

   // bitwise-OR
   dep_var[varid] = make_pair(dep_var[v0].first, varid);
   for(int i = 0; i <= nMaxVar; ++i)
      if(dep_var[v1].first[i] == '1') dep_var[varid].first[i] = '1';
}

void
CirMgr::fraig()
{
}

