cirCmd.o: cirCmd.cpp cirMgr.h cirGate.h ../../include/myHash.h cirCmd.h \
 ../../include/cmdParser.h ../../include/cmdCharDef.h \
 ../../include/util.h ../../include/rnGen.h ../../include/myUsage.h
cirFraig.o: cirFraig.cpp cirMgr.h cirGate.h ../../include/myHash.h
cirGate.o: cirGate.cpp cirMgr.h cirGate.h ../../include/myHash.h \
 ../../include/util.h ../../include/rnGen.h ../../include/myUsage.h
cirMgr.o: cirMgr.cpp cirMgr.h cirGate.h ../../include/myHash.h
cirSim.o: cirSim.cpp cirMgr.h cirGate.h ../../include/myHash.h
