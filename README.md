# QuickCov

## Usage

```bash
# Compile QuickCov, LLVM 12 must be used
cd QuickCov
make clean all CC=clang-12 CXX=clang++-12 LLVM_CONFIG=llvm-config-12 LD_LLD=ld.lld-12

# Compile the target program with clang-deopt and clang-deopt++
export DEOPT_COV=1 # enable coverage instrumentation
export CC=$PWD/clang-deopt
export CXX=$PWD/clang-deopt++
# Compile commands...

# Run target program to show the basic block coverage
COV_BITMAP=./bitmap.bin COV_NAMES=./blocks.txt ./a.out
```
