CC ?= clang
CXX ?= clang++
LLVM_CONFIG ?= llvm-config
LD_LLD ?= ld.lld
DEOPT_DIR ?= $(shell pwd)

all: clang-deopt driver

coverage.so: coverage.cpp
	${CXX} ${CXXFLAGS} -shared -fPIC $(shell ${LLVM_CONFIG} --cxxflags) coverage.cpp -o coverage.so $(shell ${LLVM_CONFIG} --ldflags)

runtime.o: runtime.c
	${CC} ${CFLAGS} -O2 -c runtime.c -o runtime.o

clang-deopt: clang-deopt.c coverage.so runtime.o
	${CC} ${CFLAGS} -O2 -DLD_LLD_PATH="\"$(shell which ${LD_LLD})\"" -DDEOPT_DIR="\"${DEOPT_DIR}\"" -DDEOPT_CC="\"${CC}\"" -DDEOPT_CXX="\"${CXX}\"" clang-deopt.c -o clang-deopt
	ln -sf clang-deopt clang-deopt++

driver: clang-deopt driver.c
	./clang-deopt ${CFLAGS} -std=c11 -fPIC -c driver.c -o driver.o
	ar rc driver.a driver.o
	rm driver.o

clean:
	rm -f clang-deopt clang-deopt++ *.so *.o *.a