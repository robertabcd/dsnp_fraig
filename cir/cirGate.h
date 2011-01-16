/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2010 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <deque>

using namespace std;

enum GateType
{
   UNDEF_GATE = 0,
   PI_GATE    = 1,
   PO_GATE    = 2,
   AIG_GATE   = 3,
   CONST_GATE = 4,

   TOT_GATE
};

static string gateTypeStr[TOT_GATE] = { "", "PI", "PO", "AIG" };

class CirMgr;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: You are free to define data members and member functions on your own
class CirVar
{
public:
   CirVar(CirMgr &mgr, int varid): mgr(mgr), id(0), line(0), symline(0),
      ref_count(0), has_symbol(false), is_removed(false) {
      setVarId(varid);
   }
   virtual ~CirVar() {}

   virtual GateType getType() const = 0;
   virtual string getTypeStr() const = 0;

   void setVarId(int i) { id = i; }
   int getVarId() const { return id; }

   void setLine(int l) { line = l; }
   int getLine() const { return line; }

   void setSymbol(const string &sym) { symbol = sym; has_symbol = true; }
   string getSymbol() const { return symbol; }
   void setSymbolLine(int l) { symline = l; }
   int getSymbolLine() const { return symline; }
   void removeSymbol() { symbol = ""; has_symbol = false; }
   bool hasSymbol() const { return has_symbol; }

   virtual void resetState() = 0;
   virtual bool evaluate() = 0;
   void pushSimulatedVal() {
      sim_history.push_back(evaluate());
      if(sim_history.size() > s_MaxSimHistory)
         sim_history.pop_front();
   }
   const deque<bool> &getSimHistory() const { return sim_history; }

   /* deprecated */
   virtual int getDependVarCount() const = 0;
   virtual int getDependVar(int idx) const = 0;
   virtual bool isDependVarNegated(int idx) const = 0;

   virtual int getIN0() const { assert(!"Invalid query"); return 0; }
   virtual int getIN1() const { assert(!"Invalid query"); return 0; }
   virtual void setIN0(int in0) { assert(!"Invalid operation"); }
   virtual void setIN1(int in1) { assert(!"Invalid operation"); }

   void setRefCount(int c) { ref_count = c; }
   int getRefCount() const { return ref_count; }
   int incRefCount() { return ++ref_count; }
   int decRefCount() { return --ref_count; }

   void markRemoved(bool r) { is_removed = r; }
   bool isRemoved() const { return is_removed; }

   /* reporting */
   virtual void reportGate() const;
   virtual void reportFanin(unsigned level) const;
   virtual void reportFanout(unsigned level) const;

protected:
   CirMgr &mgr;
   int id, line, symline, ref_count;
   bool has_symbol, is_removed;
   string symbol;

   deque<bool> sim_history;

   static unsigned int s_MaxSimHistory;


   char *getSimHistoryStr() const;

   void reportFanoutDFS(int level, int maxlevel, int caller) const;
   void reportFaninDFS(int level, int maxlevel, bool inverted) const;
};
typedef CirVar CirAigGate;

class CirConst : public CirVar
{
public:
   CirConst(CirMgr &mgr, int varid): CirVar(mgr, varid) {}
   ~CirConst() {}

   GateType getType() const { return CONST_GATE; }
   string getTypeStr() const { return "CONST"; }

   void resetState() {}
   bool evaluate() { return false; }

   int getDependVarCount() const { return 0; }
   int getDependVar(int idx) const { assert(!"Invalid query"); }
   bool isDependVarNegated(int idx) const { assert(!"Invalid query"); }
};

class CirUndef : public CirConst
{
public:
   CirUndef(CirMgr &mgr, int varid): CirConst(mgr, varid) {}
   ~CirUndef() {}

   GateType getType() const { return UNDEF_GATE; }
   string getTypeStr() const { return "UNDEF"; }
};

class CirPI : public CirVar
{
public:
   CirPI(CirMgr &mgr, int varid): CirVar(mgr, varid), val(false) {}
   ~CirPI() {}

   GateType getType() const { return PI_GATE; }
   string getTypeStr() const { return "PI"; }

   void resetState() {}
   bool evaluate() { return val; }

   int getDependVarCount() const { return 0; }
   int getDependVar(int idx) const { assert(!"Invalid query"); }
   bool isDependVarNegated(int idx) const { assert(!"Invalid query"); }

   void setVal(bool b) { val = b; }

private:
   int val;
};

class CirPO : public CirVar
{
public:
   CirPO(CirMgr &mgr, int varid, int in0 = 0): CirVar(mgr, varid), in0(in0) {
      resetState();
   }
   ~CirPO() {}

   GateType getType() const { return PO_GATE; }
   string getTypeStr() const { return "PO"; }

   void resetState() { dirty = true; }
   bool evaluate();

   int getDependVarCount() const { return 1; }
   int getDependVar(int idx) const { assert(idx == 0); return in0/2; }
   bool isDependVarNegated(int idx) const { assert(idx == 0); return in0&1; }

   int getIN0() const { return in0; }
   void setIN0(int in0) { this->in0 = in0; }

private:
   int in0;
   bool val;
   bool dirty;
};

class CirGate : public CirVar
{
public:
   CirGate(CirMgr &mgr, int varid, int in0, int in1): CirVar(mgr, varid) {
      setFanin(in0, in1);
      resetState();
   }
   ~CirGate() {}

   // Basic access methods
   GateType getType() const { return AIG_GATE; }
   string getTypeStr() const { return "AIG"; }

   int getDependVarCount() const { return 2; }
   int getDependVar(int idx) const {
      if(idx == 0) return in0/2;
      if(idx == 1) return in1/2;
      assert(!"invalid query");
      return 0;
   }
   bool isDependVarNegated(int idx) const {
      if(idx == 0) return in0&1;
      if(idx == 1) return in1&1;
      assert(!"invalid query");
      return 0;
   }

   int getIN0() const { return in0; }
   int getIN1() const { return in1; }
   void setIN0(int in0) { this->in0 = in0; resetState(); }
   void setIN1(int in1) { this->in1 = in1; resetState(); }

   // Methods about circuit construction
   void setFanin(int in0, int in1);

   // Methods about circuit simulation
   void resetState() { dirty = true; }
   bool evaluate();

   // Methods about fraig operation

   // Printing functions

   // Methods about _globalRef_s
   //bool isGlobalRef() const { return (_ref == _globalRef_s); }
   //void setToGlobalRef() { _ref = _globalRef_s; }
   //static void setGlobalRef() { _globalRef_s++; }

private:

protected:
   int in0, in1;
   bool val;
   bool dirty;
   // gate ID, in0, in1, line number, type, value...?
   //unsigned                _ref;

   //static unsigned         _globalRef_s;
};

class VarHashKey
{
public:
   VarHashKey() { setIN(0, 0); }
   VarHashKey(int in0, int in1) { setIN(in0, in1); }
   VarHashKey(CirVar *v) { setIN(v->getIN0(), v->getIN1()); }

   void setIN(int in0, int in1) {
      if(in0 <= in1) {
         this->in0 = in0;
         this->in1 = in1;
      } else
         setIN(in1, in0);
   }

   size_t operator()() const {
      return 37*(in0+1)+17*(in1+1);
   }

   bool operator==(const VarHashKey& k) const {
      return in0 == k.in0 && in1 == k.in1;
   }
private:
   int in0, in1;
};

#endif // CIR_GATE_H
