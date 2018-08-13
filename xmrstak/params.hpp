#pragma once

#include "xmrstak/misc/Environment.hpp"

#include <string>

namespace xmrstak {
struct params {
	static inline params& inst() {
		auto& env = Environment::inst();
		if(env.pParams == nullptr)
			env.pParams = new params;
		return *env.pParams;
	}

	std::string binaryName;
	std::string executablePrefix;
	bool useAMD;
	bool AMDCache;
	// user selected OpenCL vendor
	std::string openCLVendor;

	bool poolUseTls = false;
	std::string poolURL;
	bool userSetPwd = false;
	std::string poolPasswd;
	bool userSetRigid = false;
	std::string poolRigid;
	std::string poolUsername;
	bool nicehashMode = false;

	std::string currency;

	std::string configFile;
	std::string configFilePools;
	std::string configFileAMD;

	bool allowUAC = true;
	std::string minerArg0;
	std::string minerArgs;

	// block_version >= 0 enable benchmark
	int benchmark_block_version = -1;
	int benchmark_wait_sec = 30;
	int benchmark_work_sec = 60;

	params() :
		binaryName("xmr-stak"),
		executablePrefix(""),
		useAMD(true),
		AMDCache(true),
		openCLVendor("AMD"),
		configFile("config.txt"),
		configFilePools("pools.txt"),
		configFileAMD("amd.txt") {
	}

};
} // namespace xmrstak
