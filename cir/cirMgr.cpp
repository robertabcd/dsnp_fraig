/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2010 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <queue>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <cstdarg>
#include "cirMgr.h"
#include "cirGate.h"
#include "myHash.h"

using namespace std;

// TODO: You are free to define data members and member functions on your own
/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;
bool CirMgr::_noopt = false;  // default: do optimization

//-----  For your reference only ------
/*
enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};
*/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
//-----  For your reference only ------
/*
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirAigGate errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate.getTypeStr() << " in line " << errGate.getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}
*/
bool CirParser::printErrorMsg(const char *msgfmt, ...) {
   va_list args;
   va_start(args, msgfmt);
   fprintf(stderr, "[ERROR] ");
   vfprintf(stderr, msgfmt, args);
   fprintf(stderr, "\n");
   va_end(args);
   return false;
}

bool CirParser::printErrorMsgAtLine(const char *msgfmt, ...) {
   va_list args;
   va_start(args, msgfmt);
   fprintf(stderr, "[ERROR] Line %d: ", line);
   vfprintf(stderr, msgfmt, args);
   fprintf(stderr, "\n");
   va_end(args);
   return false;
}

void CirParser::Debug(const char *msg, ...) {
   if(is_debug) {
      va_list args;
      va_start(args, msg);
      vfprintf(stderr, msg, args);
      va_end(args);
   }
}

/*************************************/
/*   class CirMgr member functions   */
/*************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
   return CirParser(*this).parseFile(fileName.c_str());
}

bool CirMgr::initCircuit(int M, int I, int L, int O, int A) {
   assert(L == 0);
   nMaxVar = M;
   nInputs = I;
   nOutputs = O;
   nGates = A;

   vars     = new CirVar *[M+1];
   inputs   = new CirVar *[I];
   outputs  = new CirVar *[O];
   gates    = new CirVar *[A];

   memset(vars, 0, sizeof(CirVar *) * (M+1));
   memset(inputs, 0, sizeof(CirVar *) * I);
   memset(outputs, 0, sizeof(CirVar *) * O);
   memset(gates, 0, sizeof(CirVar *) * A);

   iInput = iOutput = iGate = 0;
   return true;
}

CirVar *CirMgr::addInput(int varid) {
   if(iInput >= nInputs) return NULL;

   CirVar *node = new CirVar(*this, varid);
   node->setType(PI_GATE);

   return inputs[iInput++] = vars[varid] = node;
}

CirVar *CirMgr::addOutput(int in0) {
   if(iOutput >= nOutputs) return NULL;

   int varid = nMaxVar+1+iOutput;

   CirVar *node = new CirVar(*this, varid);
   node->setType(PO_GATE);
   node->setIN0(in0);

   return outputs[iOutput++] = node;
}

CirVar *CirMgr::addGate(int varid, int in0, int in1) {
   if(iGate >= nGates) return NULL;

   CirVar *gate = new CirVar(*this, varid);
   gate->setType(AIG_GATE);
   gate->setIN0(in0);
   gate->setIN1(in1);

   return gates[iGate++] = vars[varid] = gate;
}

bool CirParser::parseFile(const char *filename) {
   assert(!ifs.is_open());
   ifs.open(filename, ifstream::in);
   if(!ifs.is_open())
      return printErrorMsg("Cannot open file %s", filename);
   line = col = 0;
   parseFileContent();
   ifs.close();
   return true;
}

bool CirParser::readUInt(int &ret, char delim) {
   char buf[1024];

   if(!readStr(buf, sizeof(buf), delim))
      return false;

   for(int i = strlen(buf)-2; i >= 0; --i)
      if(buf[i] < '0' || buf[i] > '9')
         return false;

   ret = atoi(buf);

   return true;
}

bool CirParser::readStr(char *buf, int bufsz, char delim) {
   if(ifs.eof())
      return false;

   ifs.get(buf, bufsz, delim);

   char nxt;
   if(ifs.eof()) return false;
   ifs.get(nxt);
   if(nxt != delim) return false;

   return true;
}

bool CirParser::parseFileContent() {
   if(!parseHeader()) return false;

   for(int i = mgr.getNumPIs()-1; i >= 0; --i)
      if(!parsePI()) return false;

   for(int i = mgr.getNumPOs()-1; i >= 0; --i)
      if(!parsePO()) return false;

   for(int i = mgr.getNumGates()-1; i >= 0; --i)
      if(!parseGate()) return false;

   // There may be some undefined variables, fix it!
   mgr.fixNullVars();

   mgr.calculateRefCount();
   mgr.countFloating();

   if(mgr._noopt) {
      printf("no opt set, skip merge trivial gates.\n");
   } else {
      printf("Merging trivial gates...\n");
      mgr.mergeTrivial();
   }

   mgr.SatSetupInputs();

   mgr.buildRevRef();

   mgr.initFecGroups();

   int nxt = ifs.peek();
   while(nxt != EOF) {
      if(nxt == 'i') {
         if(!parseInputSymbol()) return false;
      } else if(nxt == 'o') {
         if(!parseOutputSymbol()) return false;
      } else if(nxt == 'c') {
         if(!parseCommentHeader()) return false;
         break;
      } else
         return printErrorMsgAtLine("invalid character, expected i|o|c");

      nxt = ifs.peek();
   }

   return true;
}

bool CirParser::parseHeader() {
   line++;

   char buf[1024] = "";
   
   if(!readStr(buf, sizeof(buf)))
      return printErrorMsgAtLine("invalid header '%s'", buf);
   if(0 != strcmp(buf, "aag"))
      return printErrorMsgAtLine("invalid header name '%s'", buf);

   int M, I, L, O, A;
   if(!readUInt(M) || !readUInt(I) || !readUInt(L) || !readUInt(O))
      return printErrorMsgAtLine("an non-negative integer is expected here");
   if(!readUInt(A, '\n'))
      return printErrorMsgAtLine("an non-negative integer and a newline "
            "after it is expected here");

   if(M < I + A)
      return printErrorMsgAtLine("number of vars is too small");

   Debug("initCircuit: %d %d %d %d %d\n", M, I, L, O, A);

   if(L != 0)
      return printErrorMsgAtLine("latches are not support at this moment");

   return mgr.initCircuit(M, I, L, O, A);
}

bool CirParser::parsePI() {
   line++;

   int litid;
   if(!readUInt(litid, '\n'))
      return printErrorMsgAtLine("invalid/missing PI definition");

   if(litid/2 == 0)
      return printErrorMsgAtLine("cannot assign PI on const");

   if((litid & 1) || litid/2 > mgr.getMaxVarNum())
      return printErrorMsgAtLine("invalid literal number for PI");

   CirVar *var = mgr.getVar(litid/2);
   if(var != NULL)
      return printErrorMsgAtLine("redefinition literal %d, previous "
            "definition at line %d", litid, var->getLine());

   Debug("add PI %d\n", litid);

   CirVar *pi = mgr.addInput(litid/2);
   if(pi == NULL) return printErrorMsgAtLine("cannot create PI");

   pi->setLine(line);

   return true;
}

bool CirParser::parsePO() {
   line++;

   int in0;
   if(!readUInt(in0, '\n'))
      return printErrorMsgAtLine("invalid/missing PO definition");

   if(in0/2 > mgr.getMaxVarNum())
      return printErrorMsgAtLine("invalid literal number for PO");

   Debug("add PO %d\n", in0);

   CirVar *po = mgr.addOutput(in0);
   if(po == NULL) return printErrorMsgAtLine("cannot create PO");

   po->setLine(line);

   return true;
}

bool CirParser::parseGate() {
   line++;

   int litid, in0, in1;
   if(!readUInt(litid) || !readUInt(in0) || !readUInt(in1, '\n'))
      return printErrorMsgAtLine("invalid/missing AigGate definition");

   if(litid/2 == 0)
      return printErrorMsgAtLine("cannot put fanout of a gate to const");

   if(litid/2 > mgr.getMaxVarNum())
      return printErrorMsgAtLine("invalid literal number for AigGate");

   CirVar *var = mgr.getVar(litid/2);
   if(var != NULL)
      return printErrorMsgAtLine("redefinition literal %d, previous"
            "definition at line %d", litid, var->getLine());

   if(in0/2 > mgr.getMaxVarNum() || in1/2 > mgr.getMaxVarNum())
      return printErrorMsgAtLine("invalid fanin number");

   if(in0/2 == litid/2 || in1/2 == litid/2)
      return printErrorMsgAtLine("a loop at gate from fanout to fanin");

   Debug("add AigGate %d %d %d\n", litid, in0, in1);

   CirVar *g = mgr.addGate(litid/2, in0, in1);
   if(g == NULL)
      return printErrorMsgAtLine("cannot create AigGate");

   g->setLine(line);

   return true;
}

bool CirParser::checkSymbolValid(map<string, int> &tb, const string &strsym) {
   for(int i = strsym.length()-1; i >= 0; --i)
      if(!isprint(strsym[i]))
         return printErrorMsgAtLine("symbol name contains chars not printable");

   //map<string, int>::iterator it = tb.find(strsym);
   //if(it != tb.end())
   //   return printErrorMsgAtLine("redefinition of symbol '%s', previous "
   //         "definition at line %d", strsym.c_str(), it->second);
   return true;
}

bool CirParser::parseInputSymbol() {
   line++;

   char t;
   ifs.get(t);

   int id;
   char sym[1024];
   
   if(t != 'i' || !readUInt(id) || !readStr(sym, sizeof(sym), '\n'))
      return printErrorMsgAtLine("invalid symbol definition of PI");

   if(id >= mgr.getNumPIs())
      return printErrorMsgAtLine("invalid PI id: %d", id);

   CirVar *pi = mgr.getPI(id);
   if(pi->hasSymbol())
      return printErrorMsgAtLine("symbol of this PI has already declared, "
            "declaration at line %d", pi->getSymbolLine());

   string strsym(sym);
   if(!checkSymbolValid(mgr.symbols_input, strsym)) return false;

   Debug("add symbol %s for PI %d\n", sym, id);

   //mgr.symbols_input.insert(make_pair(strsym, line));
   pi->setSymbol(strsym);
   pi->setSymbolLine(line);

   return true;
}

bool CirParser::parseOutputSymbol() {
   line++;

   char t;
   ifs.get(t);

   int id;
   char sym[1024];
   
   if(t != 'o' || !readUInt(id) || !readStr(sym, sizeof(sym), '\n'))
      return printErrorMsgAtLine("invalid symbol definition of PO");

   if(id >= mgr.getNumPOs())
      return printErrorMsgAtLine("invalid PO id: %d", id);

   CirVar *po = mgr.getPO(id);
   if(po->hasSymbol())
      return printErrorMsgAtLine("symbol of this PO has already declared, "
            "declaration at line %d", po->getSymbolLine());

   string strsym(sym);
   if(!checkSymbolValid(mgr.symbols_output, strsym)) return false;

   Debug("add symbol %s for PO %d\n", sym, id);

   mgr.symbols_output.insert(make_pair(strsym, line));
   po->setSymbol(strsym);
   po->setSymbolLine(line);

   return true;
}

bool CirParser::parseCommentHeader() {
   line++;

   char buf[1024];
   
   if(!readStr(buf, sizeof(buf), '\n') || 0 != strcmp(buf, "c"))
      return printErrorMsgAtLine("invalid comment header");

   return true;
}


void CirMgr::fixNullVars() {
   if(vars[0] == NULL) {
      vars[0] = new CirVar(*this, 0);
      vars[0]->setType(CONST_GATE);
   }

   for(int i = 1; i <= nMaxVar; ++i)
      if(vars[i] == NULL) {
         vars[i] = new CirVar(*this, i);
         vars[i]->setType(UNDEF_GATE);
      }
}

void CirMgr::calculateRefCount() {
   for(int i = 0; i <= nMaxVar; ++i)
      vars[i]->setRefCount(0);

   bool vis[nMaxVar+1];
   memset(vis, 0, sizeof(bool) * (nMaxVar+1));

   for(int i = 0; i < nOutputs; ++i) {
      int varid = outputs[i]->getIN0()/2;
      assert(!vars[varid]->isRemoved());
      vars[varid]->incRefCount();
      refCountDFS(vis, varid);
   }
   for(int i = 0; i < nGates; ++i)
      refCountDFS(vis, gates[i]->getVarId());
}

void CirMgr::refCountDFS(bool *visited, int varid) {
   if(visited[varid] || vars[varid]->isRemoved()) return;
   visited[varid] = true;

   if(is_debug)
      printf(" %d", varid);

   CirVar *v = vars[varid];
   for(int i = 0, n = v->getDependVarCount(); i < n; ++i) {
      int depvarid = v->getDependVar(i);
      vars[depvarid]->incRefCount();
      refCountDFS(visited, depvarid);
   }
}

int CirMgr::countValidGates() const {
   int count = 0;
   for(int i = 0; i < nGates; ++i)
      if(!gates[i]->isRemoved() && gates[i]->getType() == AIG_GATE)
         count++;
   return count;
}

void CirMgr::removeUnrefGates() {
   queue<CirVar *> qu;

   for(int i = 1; i <= nMaxVar; ++i)
      if(!vars[i]->isRemoved() && vars[i]->getType() == AIG_GATE && 
            vars[i]->getRefCount() <= 0)
         qu.push(vars[i]);

   while(qu.size()) {
      CirVar *v = qu.front();
      qu.pop();

      v->markRemoved(true);
      printf("Removed unused gate %d\n", v->getVarId());

      int v0 = v->getIN0()/2;
      if(vars[v0]->decRefCount() <= 0 && vars[v0]->getType() == AIG_GATE)
         qu.push(vars[v0]);

      int v1 = v->getIN1()/2;
      if(vars[v1]->decRefCount() <= 0 && vars[v1]->getType() == AIG_GATE)
         qu.push(vars[v1]);
   }
}

void CirMgr::mergeTrivial() {
   bool vis[nMaxVar+1];
   memset(vis, 0, sizeof(bool) * (nMaxVar+1));

   for(int i = 0; i < nOutputs; ++i)
      outputs[i]->setIN0(mergeTrivialDFS(vis, outputs[i]->getIN0()));

   for(int i = 0; i < nGates; ++i)
      mergeTrivialDFS(vis, gates[i]->getVarId() << 1);

   calculateRefCount();
   buildRevRef();
   removeUnrefGates();
}

int CirMgr::mergeTrivialDFS(bool *visited, int litid) {
   CirVar *v = vars[litid/2];

   // process a var once only
   if(!visited[litid/2]) {
      visited[litid/2] = true;

      if(v->getType() == AIG_GATE) {
         // ananlyze children see if can merge
         v->setIN0(mergeTrivialDFS(visited, v->getIN0()));
         v->setIN1(mergeTrivialDFS(visited, v->getIN1()));
      }
   }

   if(v->getType() != AIG_GATE) return litid;

   int in0 = v->getIN0(), in1 = v->getIN1();

   // return direct link if it is trivial
   if(in0 == in1) {
      printf("Merge %d to %d\n", litid, in0);
      return in0;
   } else if((in0 ^ in1) == 1) {
      return 0;
   } else if(in0 == 0 || in1 == 0) {
      printf("Merge %d to %d\n", litid, (litid & 1)?1:0);
      return (litid & 1) ? 1 : 0;
   } else if(in0 == 1) {
      printf("Merge %d to %d\n", litid, in1);
      return in1;
   } else if(in1 == 1) {
      printf("Merge %d to %d\n", litid, in0);
      return in0;
   } else
      return litid;
}

/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      167
*********************/
void
CirMgr::printSummary() const
{
   if(is_debug) {
      printf("<<< Ref counts >>>\n");
      for(int i = 0; i < nInputs; ++i)
         printf("PI[%2d] v:%2d -> %d\n", i, inputs[i]->getVarId(),
               inputs[i]->getRefCount());
      for(int i = 0; i < nOutputs; ++i)
         printf("PO[%2d] v:%2d -> %d\n", i, outputs[i]->getVarId(),
               outputs[i]->getRefCount());
      for(int i = 0; i < nGates; ++i)
         printf("Ga[%2d] v:%2d -> %d\n", i, gates[i]->getVarId(),
               gates[i]->getRefCount());
   }

   int valid_gates = countValidGates();
   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;
   printf( "  PI      %6d\n", nInputs);
   printf( "  PO      %6d\n", nOutputs);
   printf( "  AIG     %6d\n", valid_gates);
   cout << "==================" << endl;
   printf( "  TOTAL   %6d\n", nInputs+nOutputs+valid_gates);
}

void
CirMgr::printNetlist() const
{
   int dfn = 0;
   bool vis[nMaxVar+1+nOutputs];
   memset(vis, 0, sizeof(bool) * (nMaxVar+1+nOutputs));

   for(int i = 0; i < nOutputs; ++i)
      netlistDFS(dfn, vis, outputs[i]);
}

void CirMgr::netlistDFS(int &dfn, bool *visited, const CirVar *v) const {
   if(visited[v->getVarId()]) return;
   visited[v->getVarId()] = true;

   int n = v->getDependVarCount();
   for(int i = 0; i < n; ++i)
      netlistDFS(dfn, visited, vars[v->getDependVar(i)]);

   if(v->getType() == UNDEF_GATE) return;

   printf("[%d] %-3s %d", dfn++, v->getTypeStr().c_str(), v->getVarId());

   for(int i = 0; i < n; ++i) {
      int chvarid = v->getDependVar(i);
      CirVar *ch = getVar(chvarid);
      printf(" %s%s%d", (ch->getType() == UNDEF_GATE ? "*" : ""),
            (v->isDependVarNegated(i) ? "!" : ""), chvarid);
   }

   if(v->hasSymbol())
      printf(" (%s)", v->getSymbol().c_str());

   /*if(v->getType() == PI_GATE)
      printf(" (%dGAT)", v->getVarId());
   else if(v->getType() == PO_GATE)
      printf(" (%dGAT$PO)", v->getDependVar(0));*/

   printf("\n");
}

void
CirMgr::printPIs() const
{
   printf("PIs of the circuit:");
   for(int i = 0; i < nInputs; ++i)
      printf(" %d", inputs[i]->getVarId());
   printf("\n");
}

void
CirMgr::printPOs() const
{
   printf("POs of the circuit:");
   for(int i = 0; i < nOutputs; ++i)
      printf(" %d", outputs[i]->getVarId());
   printf("\n");
}

void CirMgr::countFloating() {
   for(int i = 0; i < nGates; ++i)
      if(vars[gates[i]->getDependVar(0)]->getType() == UNDEF_GATE ||
            vars[gates[i]->getDependVar(1)]->getType() == UNDEF_GATE) {
         floating_gates.push_back(gates[i]->getVarId());
      }

   for(int i = 0; i < nGates; ++i)
      if(gates[i]->getRefCount() == 0) {
         unref_gates.push_back(gates[i]->getVarId());
      }
}

void
CirMgr::printFloatGates() const
{
   printf("Gates with floating fanin(s):");
   if(floating_gates.size() == 0)
      printf("<none>");
   else {
      for(int i = 0, n = floating_gates.size(); i < n; ++i)
         printf(" %d", floating_gates[i]);
   }
   printf("\n");

   printf("Gates defined but not used  :");
   if(unref_gates.size() == 0)
      printf("<none>");
   else {
      for(int i = 0, n = unref_gates.size(); i < n; ++i)
         printf(" %d", unref_gates[i]);
   }
   printf("\n");
}

void
CirMgr::printFECPairs() const
{
   if(!fec_groups) {
      fprintf(stderr, "not yet simulated.\n");
      return;
   }

   for(int gid = 0, n = fec_groups->size(); gid < n; ++gid) {
      printf("[%d]", gid);

      vector<int> *s = fec_groups->at(gid);

      for(vector<int>::iterator it = s->begin(), ed = s->end();
            it != ed; ++it) {

         printf(" %s%d", ((*it)&1)?"!":"", (*it)>>1);
      }

      printf("\n");
   }
}

void CirMgr::buildRevRef() {
   if(!rev_ref)
      rev_ref = new multiset<int>[nMaxVar+1+nOutputs];
   else {
      for(int i = 0, n = nMaxVar+1+nOutputs; i < n; ++i)
         rev_ref[i].clear();
   }

   bool vis[nMaxVar+1+nOutputs];
   int topo = 0;

   memset(vis, 0, sizeof(bool) * (nMaxVar+1+nOutputs));

   for(int i = 0; i < nOutputs; ++i)
      buildRevRefDFS(topo, vis, outputs[i]->getVarId(), outputs[i]->getIN0()/2);
}

void CirMgr::buildRevRefDFS(int &topo, bool *visited, int fromvarid, int varid) {
   CirVar *v = getVar(varid);

   if(!visited[varid]) {
      visited[varid] = true;
      if(v->getType() == AIG_GATE) {
         buildRevRefDFS(topo, visited, varid, v->getIN0()/2);
         buildRevRefDFS(topo, visited, varid, v->getIN1()/2);
      }
   }

   rev_ref[v->getVarId()].insert(fromvarid);

   v->setTopologicalOrder(++topo);
}

void CirMgr::outputAAG(FILE *fp) {
   fprintf(fp, "aag %d %d 0 %d %d\n", 
         nMaxVar, nInputs, nOutputs, countValidGates());

   for(int i = 0; i < nInputs; ++i)
      fprintf(fp, "%d\n", inputs[i]->getVarId()*2);

   for(int i = 0; i < nOutputs; ++i)
      fprintf(fp, "%d\n", outputs[i]->getIN0());

   for(int i = 0; i < nGates; ++i) {
      CirVar *g = gates[i];
      if(!g->isRemoved())
         fprintf(fp, "%d %d %d\n", g->getVarId()*2, g->getIN0(), g->getIN1());
   }

   // export symbols
   for(int i = 0; i < nInputs; ++i)
      if(inputs[i]->hasSymbol())
         fprintf(fp, "i%d %s\n", i, inputs[i]->getSymbol().c_str());

   for(int i = 0; i < nOutputs; ++i)
      if(outputs[i]->hasSymbol())
         fprintf(fp, "o%d %s\n", i, outputs[i]->getSymbol().c_str());

   fprintf(fp, "c\ngenerated by fraig (b98902060)\n");
}

