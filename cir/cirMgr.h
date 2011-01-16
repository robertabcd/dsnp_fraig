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
   CirMgr(): _simLog(NULL), rev_ref(NULL), sat_effort(EFFORT_MED),
      fec_groups(NULL) {
      is_debug = true;
   }
   ~CirMgr() { deleteCircuit(); }

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
   void deleteCircuit() { }

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
   void setSatEffort(SATSolveEffort ef) { sat_effort = ef; }

   void initFecGrps();
   void FecGrouping();

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

   void refCountDFS(bool *visited, int varid);
   int countValidGates() const;
   void netlistDFS(int &dfn, bool *visited, const CirVar *v) const;
   int mergeTrivialDFS(bool *visited, int litid);
   int strashDFS(bool *visited, Hash<VarHashKey, int> &h, int litid);
   void initFecGrpsDFS(bool *visited, pair<string, int> *dep_var, int varid);

   void buildRevRef();
   void buildRevRefDFS(bool *visited, int fromvarid, int varid);

   void countFloating();

   bool simuationError(const char *msgfmt, ...);
   void simulationResult(const char *patt, const char *result);

   bool checkSimulationPattern(const char *patt);
   void pushSimulationPattern(const char *patt, gateval_t *vin);
   bool simulate(gateval_t *vin, char **result);
};

class CirParser
{
public:
   CirParser(CirMgr &mgr): mgr(mgr) {
      is_debug = true;
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
