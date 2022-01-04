all: central.cpp serverT.cpp serverS.cpp serverP.cpp clientA.cpp clientB.cpp
	g++ -o serverC central.cpp --std=c++11

	g++ -o serverT serverT.cpp --std=c++11

	g++ -o serverS serverS.cpp --std=c++11
	
	g++ -o serverP serverP.cpp --std=c++11

	g++ -o clientA clientA.cpp --std=c++11

	g++ -o clientB clientB.cpp --std=c++11

.PHONY: serverC
serverC:
	./serverC

.PHONY: serverT
serverT:
	./serverT

.PHONY: serverS
serverS:
	./serverS

.PHONY: serverP
serverP:
	./serverP

.PHONY: clientA
clientA:
	./clientA

.PHONY: clientB
clientB:
	./clientB
