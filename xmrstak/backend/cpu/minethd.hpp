#pragma once

#include "xmrstak/jconf.hpp"
#include "crypto/cryptonight.h"
#include "xmrstak/backend/miner_work.hpp"
#include "xmrstak/backend/iBackend.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <future>

namespace xmrstak {
namespace cpu {

class minethd : public iBackend {
public:
    typedef void (*cn_hash_fun)(const void*, size_t, void*, cryptonight_ctx*);
	static bool self_test();
	static cn_hash_fun func_selector(xmrstak_algo algo);
	static cryptonight_ctx* minethd_alloc_ctx();
};

} // namespace cpu
} // namespace xmrstak
