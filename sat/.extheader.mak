sat.d: ../../include/sat.h 
../../include/sat.h: sat.h
	@rm -f ../../include/sat.h
	@ln -fs ../src/sat/sat.h ../../include/sat.h
