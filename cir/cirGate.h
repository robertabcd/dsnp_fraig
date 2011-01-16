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

typedef unsigned int gateval_t;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: You are free to define data members and member functions on your own
class CirVar
{
public:
   CirVar(CirMgr &mgr, int varid): mgr(mgr), id(0), line(0), symline(0),
      ref_count(0), has_symbol(false), is_removed(false) {
      dirty = true;
      simulating = false;
      val = false;
      in0 = in1 = 0;
      fec_id = 0;
      topo_ord = 0;
      setVarId(varid);
   }
   virtual ~CirVar() {}

   void setType(GateType t);
   GateType getType() const { return type; }
   string getTypeStr() const { return str_type; }

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

   inline void resetState() { dirty = true; }
   inline gateval_t evaluate() {
      if(dirty) return updateValue();
      return val;
   }
   inline void setVal(gateval_t v) { dirty = false; val = v; }

   /* deprecated */
   int getDependVarCount() const {
      if(type == AIG_GATE) return 2;
      else if(type == PO_GATE) return 1;
      else return 0;
   }
   int getDependVar(int idx) const {
      if(idx == 0) {
         assert(type == AIG_GATE || type == PO_GATE);
         return in0>>1;
      } else if(idx == 1) {
         assert(type == AIG_GATE);
         return in1>>1;
      } else
         assert(!"invalid index");
   }
   bool isDependVarNegated(int idx) const {
      if(idx == 0) {
         assert(type == AIG_GATE || type == PO_GATE);
         return in0&1;
      } else if(idx == 1) {
         assert(type == AIG_GATE);
         return in1&1;
      } else
         assert(!"invalid index");
   }

   inline int getIN0() const { return in0; }
   inline int getIN1() const { return in1; }
   void setIN0(int in0) {
      assert(type == AIG_GATE || type == PO_GATE);
      this->in0 = in0;
   }
   void setIN1(int in1) {
      assert(type == AIG_GATE);
      this->in1 = in1;
   }

   void setRefCount(int c) { ref_count = c; }
   int getRefCount() const { return ref_count; }
   int incRefCount() { return ++ref_count; }
   int decRefCount() { return --ref_count; }

   void markRemoved(bool r) { is_removed = r; }
   bool isRemoved() const { return is_removed; }

   /* reporting */
   void reportGate() const;
   void reportFanin(unsigned level) const;
   void reportFanout(unsigned level) const;

   inline void setFecGroup(int id, int lit) { fec_id = id; fec_lit = lit; }
   inline void setFecGroupId(int id) { fec_id = id; }
   inline int  getFecGroupId() const { return fec_id; }
   inline void setFecLiteral(int lit) { fec_lit = id; }
   inline int  getFecLiteral() const { return fec_lit; }

   inline void setTopologicalOrder(int ord) { topo_ord = ord; }
   inline int  getTopologicalOrder() const { return topo_ord; }

protected:
   CirMgr &mgr;
   int id, line, symline, ref_count;
   bool has_symbol, is_removed;
   string symbol;

   GateType type;
   string str_type;

   int in0, in1;
   gateval_t val;
   bool dirty, simulating;

   int fec_id, fec_lit;

   gateval_t updateValue();

   int topo_ord;

   void reportFanoutDFS(int level, int maxlevel, int caller) const;
   void reportFaninDFS(int level, int maxlevel, bool inverted) const;
};
typedef CirVar CirAigGate;


/*
class XXXCirGate
{
public:
   CirGate(CirMgr &mgr, int varid, int in0, int in1){//: CirVar(mgr, varid) {
      setFanin(in0, in1);
      resetState();
   }
   ~CirGate() {}

   // Basic access methods

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


   // Methods about circuit construction

   // Methods about circuit simulation

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
*/

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
