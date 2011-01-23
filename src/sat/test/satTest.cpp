#include <iostream>
#include <vector>
#include "sat.h"

using namespace std;

class Gate
{
public:
   Gate(unsigned i = 0): _gid(i) {}
   ~Gate() {}

   Var getVar() const { return _var; }
   void setVar(const Var& v) { _var = v; }

private:
   unsigned   _gid;  // for debugging purpose...
   Var        _var;
};

// 
//[0] PI  1 (x)
//[1] PI  2 (y)
//[2] AIG 6 1 2
//[3] AIG 7 !1 !2
//[4] AIG 3 !6 !7
//[5] PO  8 3 (s)
//[6] PO  9 6 (c)
//
vector<Gate *> gates;

void
initCircuit()
{
   // Init gates
   gates.push_back(new Gate(1)); // gates[0]
   gates.push_back(new Gate(2)); // gates[1]
   gates.push_back(new Gate(6)); // gates[2]
   gates.push_back(new Gate(7)); // gates[3]
   gates.push_back(new Gate(3)); // gates[4]

   // POs are not needed in this demo example
}

void
genProofModel(SatSolver& s)
{
   // Allocate and record variables; No Var ID for POs
   for (size_t i = 0, n = gates.size(); i < n; ++i) {
      Var v = s.newVar();
      gates[i]->setVar(v);
   }

   // Hard code the model construction here...
   // [2] AIG 6 1 2 => [2] = [0] & [1]
   s.addAigCNF(gates[2]->getVar(), gates[0]->getVar(), false,
               gates[1]->getVar(), false);
   // [3] AIG 7 !1 !2 ==> [3] = ![0] & ![1]
   s.addAigCNF(gates[3]->getVar(), gates[0]->getVar(), true,
               gates[1]->getVar(), true);
   // [4] AIG 3 !6 !7 ==> [4] = ![2] & ![3]
   s.addAigCNF(gates[4]->getVar(), gates[2]->getVar(), true,
               gates[3]->getVar(), true);
}

void reportResult(const SatSolver& solver, bool result)
{
   solver.printStats();
   cout << (result? "SAT" : "UNSAT") << endl;
   if (result) {
      cout << solver.getValue(gates[0]->getVar()) << endl;
      cout << solver.getValue(gates[1]->getVar()) << endl;
   }
}

int main()
{
   initCircuit();

   SatSolver solver;
   solver.initialize();

   //
   genProofModel(solver);

   bool result;
   // Solve(Gate(3) && Gate(6))
   solver.assumeRelease();  // Clear assumptions
   solver.assumeProperty(gates[4]->getVar(), true);  // Gate(3) = 1
   solver.assumeProperty(gates[2]->getVar(), true);  // Gate(6) = 1
   result = solver.assumpSolve();
   reportResult(solver, result);

   // Solve(!Gate(3) && !Gate(6))
   solver.assumeRelease();  // Clear assumptions
   solver.assumeProperty(gates[4]->getVar(), false);  // Gate(3) = 0
   solver.assumeProperty(gates[2]->getVar(), false);  // Gate(6) = 0
   result = solver.assumpSolve();
   reportResult(solver, result);

   // Solve(Gate(3))
   solver.assumeRelease();  // Clear assumptions
   solver.assumeProperty(gates[4]->getVar(), true);  // Gate(3) = 1
   result = solver.assumpSolve();
   reportResult(solver, result);
}
