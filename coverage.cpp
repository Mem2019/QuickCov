#include <tuple>
#include <numeric>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/DebugInfo.h"

using namespace llvm;

namespace
{

class CoverageTracer : public ModulePass
{
public:
	static char ID;
	CoverageTracer() : ModulePass(ID) { }

	bool runOnModule(Module &M) override;
};

char CoverageTracer::ID = 0;

void getDebugLoc(const Instruction *I, std::string &Filename, unsigned &Line)
{
	if (DILocation *Loc = I->getDebugLoc())
	{
		Line = Loc->getLine();
		Filename = Loc->getFilename().str();

		if (Filename.empty())
		{
			DILocation *oDILoc = Loc->getInlinedAt();
			if (oDILoc)
			{
				Line = oDILoc->getLine();
				Filename = oDILoc->getFilename().str();
			}
		}
	}
}

std::string getBlockName(Module &M, BasicBlock& BB)
{
	for (Instruction& I : BB)
	{
		std::string filename;
		unsigned line = 0;
		getDebugLoc(&I, filename, line);

		/* Don't worry about external libs */
		static const std::string Xlibs("/usr/");
		if (filename.empty() || line == 0 ||
			!filename.compare(0, Xlibs.size(), Xlibs))
			continue;

		std::size_t found = filename.find_last_of("/\\");
		if (found != std::string::npos)
			filename = filename.substr(found + 1);

		return filename + ":" + std::to_string(line);
	}

	return "";
}

bool CoverageTracer::runOnModule(Module &M)
{
	LLVMContext &C = M.getContext();
	IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
	IntegerType *Int64Ty = IntegerType::getInt64Ty(C);

	// Map each basic block to index to name,
	// note multiple basic blocks can be mapped to same index
	std::unordered_map<BasicBlock*, size_t> bb_names;

	// Map each name index to actual name string.
	std::vector<std::string> names;
	// Map each name to its index, reverse of `names`.
	std::unordered_map<std::string, size_t> name_idxs;

	// Firstly, according to name of each basic block,
	// we allocate a index for all possible names.
	for (Function& F : M)
	{
		for (BasicBlock& BB : F)
		{
			auto name = getBlockName(M, BB);
			if (name.empty()) // Skip blocks without name
				continue;
			auto it = name_idxs.find(name);
			if (it == name_idxs.end())
			{ // If the name is new, we allocate a new index
				size_t idx = names.size();
				name_idxs.emplace(name, idx);
				names.push_back(std::move(name));
				bb_names.emplace(&BB, idx);
			}
			else
			{ // Otherwiese, we use the existing index.
				bb_names.emplace(&BB, it->second);
			}
		}
	}

	std::ostringstream oss;
	for (const auto& name : names)
		oss << name << std::endl;
	std::string all_names = oss.str();

	// String that stores all basic block names ordered by indexes.
	new GlobalVariable(M, ArrayType::get(Int8Ty, all_names.length() + 1),
		false, GlobalValue::ExternalLinkage,
		ConstantDataArray::getString(C, all_names.c_str()),
		"__cov_all_block_names");

	// Bitmap for tracing coverage

	ArrayType* BitmapType = ArrayType::get(Int8Ty, (names.size() + 7) / 8);
	new GlobalVariable(M, BitmapType,
		false, GlobalValue::ExternalLinkage,
		Constant::getNullValue(BitmapType), "__cov_trace_bitmap");

	// Number of named basic blocks
	new GlobalVariable(M, Int64Ty, true, GlobalValue::ExternalLinkage,
		ConstantInt::get(Int64Ty, names.size()), "__cov_num_names");

	Type *Arg64[] = {Int64Ty};
	FunctionType* FTy64 = FunctionType::get(Type::getVoidTy(C), Arg64, false);

	// Instrumentation
	for (const auto& bb_name : bb_names)
	{
		IRBuilder<> IRB(&(*bb_name.first->getFirstInsertionPt()));
		ConstantInt* Idx = ConstantInt::get(Int64Ty, bb_name.second);
		IRB.CreateCall(M.getOrInsertFunction("__cov_update_bit", FTy64), {Idx});
	}

	return true;
}

}

static void registerAFLPass(const PassManagerBuilder &, legacy::PassManagerBase &PM)
{
	PM.add(new CoverageTracer());
}

static RegisterStandardPasses RegisterAFLPassLTO(
	PassManagerBuilder::EP_FullLinkTimeOptimizationLast, registerAFLPass);
static RegisterStandardPasses RegisterAFLPass0(
	PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);