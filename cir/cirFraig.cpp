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

#include "sat.h"

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
   buildRevRef();
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

void CirMgr::FecGrouping() {
   if(!fec_groups)
      initFecGroups();

   // prepare a new container
   FECGrp *fec_new = new FECGrp();

   // use a map to group same values
   map<gateval_t, vector<int> *> valmap;

   while(fec_groups->size() > 0) {
      vector<int> *s = fec_groups->back();
      fec_groups->pop_back();

      // eventually put gates
      for(vector<int>::iterator it = s->begin(), ed = s->end();
            it != ed; ++it) {

         CirVar *v = vars[(*it)>>1];
         if(v->isRemoved()) continue;

         map<gateval_t, vector<int> *>::iterator itmap;

         if((itmap = valmap.find(v->evaluate())) != valmap.end()) {
            // fec eq
            itmap->second->push_back(v->getVarId()<<1);
         }
         else if((itmap = valmap.find(~v->evaluate())) != valmap.end()) {
            // fec inverted
            itmap->second->push_back((v->getVarId()<<1)|1);
         }
         else {
            // not exist pattern
            vector<int> *cont = new vector<int>();
            cont->push_back(v->getVarId()<<1);

            valmap.insert(make_pair(v->evaluate(), cont));
         }
      }

      // grab groups we interest in
      for(map<gateval_t, vector<int> *>::iterator it = valmap.begin(),
            ed = valmap.end(); it != ed; ++it) {

         vector<int> *cont = it->second;

         assert(cont->size() > 0);
         if(cont->size() == 1) {
            // we don't want groups whose #elem=1
            vars[cont->at(0)>>1]->setFecGroupId(-1);

            delete cont;

         } else {
            // add them to global group pool
            int gid = fec_new->size();

            // yah, I have group id now :)
            for(int i = 0, n = cont->size(); i < n; ++i)
               vars[cont->at(i)>>1]->setFecGroup(gid, cont->at(i));

            fec_new->push_back(cont);
         }
      }

      delete s;
      valmap.clear();
   }

   // swap over
   for(int i = 0, n = fec_groups->size(); i < n; ++i)
      delete fec_groups->at(i);
   delete fec_groups;
   fec_groups = fec_new;
}

void CirMgr::initFecGroups() {
   if(!fec_groups) {
      fec_groups = new FECGrp();
   } else {
      // clear container if exists
      for(int i = 0, n = fec_groups->size(); i < n; ++i)
         delete fec_groups->at(i);
      fec_groups->clear();
   }

   // put all gates in the same container
   vector<int> *s = new vector<int>();

   for(int i = 0; i < nGates; ++i) {
      s->push_back(gates[i]->getVarId()<<1);
      gates[i]->setFecGroupId(0);
   }

   fec_groups->push_back(s);
}

void CirMgr::printFecGroups() {
   if(!fec_groups) {
      fprintf(stderr, "fec groups: not yet simulated");
      return;
   }

   for(int i = 0, n = fec_groups->size(); i < n; ++i) {
      vector<int> *s = fec_groups->at(i);

      printf("G[%d] #%d\t->", i, (int)s->size());

      for(int j = 0, sz = s->size(); j < sz; ++j)
         printf(" %s%d", (s->at(j)&1)?"!":"", s->at(j)>>1);

      printf("\n");
   }
}

void
CirMgr::fraig()
{
   bool visited[nMaxVar+1];
   memset(visited, 0, sizeof(bool)*(nMaxVar+1));

   //fraigReducePairs();

   fraig_dfs_leave = 64;

   do {
      sat_merged = 0;

      SatSetupInputs();

      int dfn = 0;
      int eqlit[nMaxVar+1];

      memset(visited, 0, sizeof(bool)*(nMaxVar+1));

      for(int i = 0; i <= nMaxVar; ++i)
         eqlit[i] = i<<1;

      for(int i = 0; i < nOutputs; ++i)
         outputs[i]->setIN0(
               fraigDFS(dfn, visited, eqlit, outputs[i]->getIN0()));

      calculateRefCount();
      mergeTrivial();
      buildRevRef();
      removeUnrefGates();
   } while(sat_merged >= fraig_dfs_leave);
}

int CirMgr::fraigDFS(int &dfn, bool *visited, int *eqlit, int litid) {
   int varid = litid>>1;

   if(sat_merged >= fraig_dfs_leave) return litid;

   if(visited[varid])
      return (eqlit[varid]&~1)|((eqlit[varid]^litid)&1);
   visited[varid] = true;
   dfn++;

   CirVar *v = getVar(varid);

   if(v->getType() == AIG_GATE) {
      v->setIN0(fraigDFS(dfn, visited, eqlit, v->getIN0()));
      v->setIN1(fraigDFS(dfn, visited, eqlit, v->getIN1()));
   } else
      return litid;

   SatAddGate(v);

   // it is inefficient to hang on and solve all pairs.
   // when enough key-counter-patterns are collected,
   // try to "subset" pairs using these patterns.
   // thus, causing group id to be changed, a retry is
   // necessary. same pairs are guaranteed not to appear
   // again (because we use the unsatisfiable patterns).
   bool retry = true;
   while(retry) {
      retry = false;

      const vector<int> *s = getFecGroup(v->getFecGroupId());
      if(!s) return litid;

      for(vector<int>::const_iterator it = s->begin(), ed = s->end();
            it != ed; ++it) {
         int svarid = (*it)>>1;
         if(svarid == varid) continue;

         // fec and visited -> solve EQ
         if(visited[svarid]) {
            int inv_flag = ((*it) ^ v->getFecLiteral()) & 1;

            if(SatSolveVarEQ(varid, svarid, inv_flag)) {
               // not-EQ, enqueue simulation pattern to separate sets
               SatStoreKeyPattern();

               // retry now!
               if(SatIsKeyPatternStorageFull()) {
                  retry = true;
                  SatSimulateKeyPatterns();
                  break;
               }
            } else {
               // EQ, merge
               sat_merged++;

               printf("fraig: <%d> merge %d to %d\n", dfn, varid, svarid);
               eqlit[varid] = (svarid<<1)|inv_flag;
               return (svarid<<1)|((litid^inv_flag)&1);
            }
         }
      }
   }

   return litid;
}

void CirMgr::fraigReducePairs() {
   printf("try to reduce group size to average\n");

   int reducible[nMaxVar+1];
   bool visited[nMaxVar+1];
   bool ret;

   do {
      sat_merged = 0;

      SatSetupInputs();

      for(int i = 0; i < nOutputs; ++i)
         SatAddGateDFS(visited, vars[outputs[i]->getIN0()>>1]);

      for(int i = 0; i <= nMaxVar; ++i)
         reducible[i] = i<<1;

      memset(visited, 0, sizeof(bool)*(nMaxVar+1));

      while((ret = fraigReducePairsLoop(reducible))) {
         if(sat_merged >= 64) break;
      }

      for(int i = 0; i < nOutputs; ++i)
         outputs[i]->setIN0(
               fraigReducePairsDFS(visited, reducible, outputs[i]->getIN0()));

      calculateRefCount();
      mergeTrivial();
      buildRevRef();
      removeUnrefGates();
   } while(ret);
}

int CirMgr::fraigReducePairsDFS(bool *visited, int *reducible, int litid) {
   int varid = litid>>1;

   if(!visited[varid]) {
      visited[varid] = true;

      CirVar *v = vars[varid];
      if(v->getType() == AIG_GATE) {
         v->setIN0(fraigReducePairsDFS(visited, reducible, v->getIN0()));
         v->setIN1(fraigReducePairsDFS(visited, reducible, v->getIN1()));
      }
   }

   return (reducible[varid]&~1)|((reducible[varid]^litid)&1);
}

bool CirMgr::fraigReducePairsLoop(int *reducible) {
   int n = fec_groups->size(), avg = 0;
   if(n == 0) return false;

   for(int i = 0; i < n; ++i)
      avg += fec_groups->at(i)->size();

   printf("total is %d\n", avg);
   avg /= n;
   printf("average is %d\n", avg);

   for(int i = 0; i < n; ++i) {
      vector<int> *s = fec_groups->at(i);
      int sz = s->size();

      if(sz > 1 && sz > 20) {
         int litid0 = s->at(0);
         int varid0 = litid0>>1;

         for(int j = 1; j < sz; ++j) {
            int litid = s->at(j);
            int varid = litid>>1;
            int inv_flag = (litid0 ^ litid) & 1;

            printf("[g:%d, %d/%d] ", i, j, sz);
            if(SatSolveVarEQ(varid0, varid, inv_flag)) {
               // not-EQ, enqueue simulation pattern to separate sets
               SatStoreKeyPattern();

               // retry now!
               if(SatIsKeyPatternStorageFull()) {
                  SatSimulateKeyPatterns();
                  return true;
               }
            } else {
               // EQ, merge
               // make sure the topology is correct
               if(vars[varid0]->getTopologicalOrder() >
                     vars[varid]->getTopologicalOrder()) {
                  printf("fraig: merge %d to %d\n", varid0, varid);
                  reducible[varid0] = (varid<<1)|(inv_flag&1);
               } else {
                  printf("fraig: merge %d to %d\n", varid, varid0);
                  reducible[varid] = (varid0<<1)|(inv_flag&1);
               }
               sat_merged++;
            }
         }
      }
   }

   return false;
}

bool CirMgr::SatSolveVarEQ(int v0, int v1, bool inv_flag) {
   // EQ
   Var feq = sat_solver.newVar();

   sat_solver.addXorCNF(feq, 
         sat_var[v0], false, sat_var[v1], inv_flag);

   sat_solver.assumeRelease();
   sat_solver.assumeProperty(feq, true);

   printf("SAT: %d == %s%d ?\r", v0, inv_flag?"!":"", v1);
   fflush(stdout);

   return sat_solver.assumpSolve();
}

void CirMgr::SatSetupInputs() {
   sat_solver.initialize();

   if(sat_var) delete sat_var;
   sat_var = new Var[nMaxVar+1];

   for(int i = 0; i <= nMaxVar; i++) {
      sat_var[i] = sat_solver.newVar();
   }
}

void CirMgr::SatAddGate(CirVar *v) {
   assert(v && v->getType() == AIG_GATE);
   sat_solver.addAigCNF(sat_var[v->getVarId()],
         sat_var[v->getIN0()>>1], v->getIN0()&1,
         sat_var[v->getIN1()>>1], v->getIN1()&1);
}

void CirMgr::SatAddGateDFS(bool *visited, CirVar *v) {
   int varid = v->getVarId();

   if(visited[varid]) return;
   visited[varid] = true;

   if(v->getType() == AIG_GATE) {
      SatAddGateDFS(visited, vars[v->getIN0()>>1]);
      SatAddGateDFS(visited, vars[v->getIN1()>>1]);
      SatAddGate(v);
   }
}

void CirMgr::SatStoreKeyPattern() {
   if(!sat_keypat) {
      sat_keypat = new gateval_t[nInputs];
      sat_keypat_size = 0;
   }

   sat_keypat_size++;
   for(int i = 0; i < nInputs; ++i) {
      int varid = inputs[i]->getVarId();
      sat_keypat[i] <<= 1;
      sat_keypat[i] |= (1 & sat_solver.getValue(sat_var[varid]));
   }
}

bool CirMgr::SatIsKeyPatternStorageFull() const {
   return sat_keypat_size == (sizeof(gateval_t)*8);
}

void CirMgr::SatSimulateKeyPatterns() {
   printf("fraig: simulating key patterns\n");

   simulate(sat_keypat, NULL);

   printf("fraig: current #FEC groups: %d\n", (int)fec_groups->size());

   sat_keypat_size = 0;
}

