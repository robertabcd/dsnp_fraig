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
               vars[cont->at(i)>>1]->setFecGroupId(gid);

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
}

