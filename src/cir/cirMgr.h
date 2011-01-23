/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2010 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include <fstream>

#include "cirGate.h"
#include "myHash.h"

#include "sat.h"

//class CirAigGate;

using namespace std;

enum SATSolveEffort {
   EFFORT_LOW,
   EFFORT_MED,
   EFFORT_HIGH,
   EFFORT_UNLIMITED
};

// TODO: You are free to define data members and member functions on your own
class CirMgr
{
   typedef vector<vector<int> *> FECGrp;

   friend class CirParser;

public:
   CirMgr() {
      is_debug = false;

      _simLog = NULL;

      rev_ref = NULL;
      fec_groups = NULL;

      sat_var = NULL;
      sat_keypat = NULL;
      sat_keypat_size = 0;

      sat_effort = EFFORT_MED;
      surrender = 20;
   }
   ~CirMgr() { deleteCircuit(); }
   void deleteCircuit() {
      if(fec_groups) {
         for(int i = 0, n = fec_groups->size(); i < n; ++i)
            delete fec_groups->at(i);
         delete fec_groups;
         fec_groups = NULL;
      }

      if(sat_var) {
         delete[] sat_var;
         sat_var = NULL;
      }

      if(sat_keypat) {
         delete[] sat_keypat;
         sat_keypat = NULL;
      }

      if(vars) {
         for(int i = 0; i <= nMaxVar; ++i)
            delete vars[i];
         delete[] vars;
         vars = NULL;
         inputs = NULL;
      }

      if(outputs) {
         for(int i = 0; i < nOutputs; ++i)
            delete outputs[i];
         delete[] outputs;
         outputs = NULL;
      }
   }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   // gid == varid | PO id
   CirAigGate* getAigGate(unsigned gid) const { return getVar(gid); }
   CirAigGate* getGate(unsigned gid) const { return getVar(gid); }

   CirVar *getVar(int varid) const {
      if(varid <= nMaxVar) {
         if(vars[varid] == NULL || vars[varid]->isRemoved())
            return NULL;
         else
            return vars[varid];
      } else if(varid <= nMaxVar+nOutputs)
         return outputs[varid-nMaxVar-1];
      else
         return NULL;
   }
   // this is intended to speed up simulation only!
   inline CirVar *getVarDirectly(int varid) { return vars[varid]; }
   CirVar  *getPI(int id) const { return inputs[id]; }
   CirVar  *getPO(int id) const { return outputs[id]; }

   int getNumPIs() const { return nInputs; }
   int getNumPOs() const { return nOutputs; }
   int getNumGates() const { return nGates; }
   int getMaxVarNum() const { return nMaxVar; }

   const multiset<int> *queryVarRevRef(int idx) const {
      if(rev_ref) return &rev_ref[idx];
      else return NULL;
   }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   bool initCircuit(int M, int I, int L, int O, int A);
   CirVar *addInput(int varid);
   CirVar *addOutput(int in0);
   CirVar *addGate(int varid, int in0, int in1);

   void fixNullVars();

   // Member functions about circuit optimization
   static void setNoOpt(bool noopt) { _noopt = noopt; }

   void calculateRefCount();
   void mergeTrivial();
   void removeUnrefGates();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;

   // Member functions about fraig
   void strash();
   void setSimLog(ofstream *logFile) { _simLog = logFile; }
   void randomSim();
   void fileSim(ifstream&);
   bool simulatePattern(const char *patt, char *result);
   void fraig();
   void setSatEffort(SATSolveEffort ef) {
      switch(sat_effort = ef) {
         case EFFORT_LOW: surrender = 5; break;
         case EFFORT_MED: surrender = 20; break;
         case EFFORT_HIGH: surrender = 50; break;
         default: surrender = 100;
      }
   }
   void SatStoreKeyPattern();
   inline bool SatIsKeyPatternStorageFull() const;
   int SatSimulateKeyPatterns();

   void initFecGroups();
   int  FecGrouping();
   const vector<int> *getFecGroup(int i) const {
      if(!fec_groups || i < 0 || i >= (int)fec_groups->size())
         return NULL;
      return fec_groups->at(i);
   }
   void printFecGroups();

   // Member functions about flags

   void outputAAG(FILE *fp);

private:
   bool is_debug;

   // lists of pi, po, aig, total, dfs, floating, FEC...
   // Simulation, fraig related...
   ofstream    *_simLog;
   static bool  _noopt;


   // private member functions for circuit parsing
   int nMaxVar, nInputs, nOutputs, nGates;
   int iInput, iOutput, iGate;
   CirVar   **vars;
   CirVar   **inputs;
   CirVar   **outputs;
   CirVar   **gates;

   map<string, int> symbols_input, symbols_output;

   vector<int> floating_gates, unref_gates;

   multiset<int> *rev_ref;

   SATSolveEffort sat_effort;
   FECGrp *fec_groups;

   SatSolver sat_solver;
   Var *sat_var;

   gateval_t *sat_keypat;
   int sat_keypat_size;
   int sat_merged;

   int fraig_dfs_leave;
   vector<pair<int, int> > fraig_sim_pairs;

   // use for effort setting
   int surrender;

   void refCountDFS(bool *visited, int varid);
   int countValidGates() const;
   void netlistDFS(int &dfn, bool *visited, const CirVar *v) const;
   int mergeTrivialDFS(bool *visited, int litid);
   int strashDFS(bool *visited, Hash<VarHashKey, int> &h, int litid);

   int  fraigDFS(int &dfn, bool *visited, int *eqlit, int litid);
   void SatSetupInputs();
   void SatAddGate(CirVar *v);
   void SatAddGateDFS(bool *visited, CirVar *v);
   bool SatSolveVarEQ(int v0, int v1, bool inv_flag);
   void SatBlacklistNonseparatedVars();
   void fraigReducePairs();
   bool fraigReducePairsLoop(int *reducible);
   int  fraigReducePairsDFS(bool *visited, int *reducible, int litid);

   void buildRevRef();
   void buildRevRefDFS(int &topo, bool *visited, int fromvarid, int varid);

   void countFloating();

   bool simuationError(const char *msgfmt, ...);
   void simulationResult(const char *patt, const char *result);

   bool checkSimulationPattern(const char *patt);
   void pushSimulationPattern(const char *patt, gateval_t *vin);
   int  simulate(gateval_t *vin, char **result);
};

class CirParser
{
public:
   CirParser(CirMgr &mgr): mgr(mgr) {
      is_debug = mgr.is_debug;
   }
   ~CirParser() {}

   bool parseFile(const char *filename);

private:
   CirMgr &mgr;
   ifstream ifs;

   int line, col;

   bool is_debug;

   bool printErrorMsg(const char *msgfmt, ...);
   bool printErrorMsgAtLine(const char *msgfmt, ...);

   void Debug(const char *msg, ...);

   bool readUInt(int &ret, char delim = ' ');
   bool readStr(char *buf, int bufsz, char delim = ' ');

   bool parseFileContent();
   bool parseHeader();
   bool parsePI();
   bool parsePO();
   bool parseGate();
   bool parseInputSymbol();
   bool parseOutputSymbol();
   bool parseCommentHeader();

   bool checkSymbolValid(map<string, int> &tb, const string &strsym);
};


#endif // CIR_MGR_H
