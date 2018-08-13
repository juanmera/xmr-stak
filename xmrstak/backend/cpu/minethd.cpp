/*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include "crypto/cryptonight_aesni.h"

#include "xmrstak/misc/console.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend/GlobalStates.hpp"
#include "xmrstak/misc/configEditor.hpp"
#include "xmrstak/params.hpp"

#include "xmrstak/misc/executor.hpp"
#include "minethd.hpp"
#include "xmrstak/jconf.hpp"

#include "xmrstak/backend/miner_work.hpp"

#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <chrono>
#include <cstring>
#include <thread>
#include <bitset>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>

#if defined(__APPLE__)
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"
#elif defined(__FreeBSD__)
#include <pthread_np.h>
#endif //__APPLE__

#endif //_WIN32

namespace xmrstak {
namespace cpu {

static constexpr size_t MAX_N = 5;

cryptonight_ctx* minethd::minethd_alloc_ctx() {
	alloc_msg msg = { 0 };

	auto ctx = cryptonight_alloc_ctx(&msg);
	if (ctx == nullptr) {
		Printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
	}
	return ctx;
}


bool minethd::self_test() {
	alloc_msg msg = { 0 };

	auto res = cryptonight_init(&msg);
	if(msg.warning != nullptr) {
	    Printer::inst()->print_msg(L0, "MEMORY INIT ERROR: %s", msg.warning);
	}

	if(res == 0) {
	    return false;
	}

	cryptonight_ctx *ctx[MAX_N] = {0};
	for (size_t i = 0; i < MAX_N; i++) {
		if ((ctx[i] = minethd_alloc_ctx()) == nullptr) {
			for (size_t j = 0; j < i; j++) {
				cryptonight_free_ctx(ctx[j]);
			}
			return false;
		}
	}

	auto result = true;

	if(::jconf::inst()->GetCurrentCoinSelection().GetDescription().GetMiningAlgo() == cryptonight) {
		unsigned char out[32 * MAX_N];

		auto hashf = func_selector(xmrstak_algo::cryptonight);
		hashf("This is a test", 14, out, ctx[0]);
		result &= memcmp(out, "\xa0\x84\xf0\x1d\x14\x37\xa0\x9c\x69\x85\x40\x1b\x60\xd4\x35\x54\xae\x10\x58\x02\xc5\xf5\xd8\xa9\xb3\x25\x36\x49\xc0\xbe\x66\x05", 32) == 0;
	}
	for (auto &c: ctx) {
        cryptonight_free_ctx(c);
	}
	if(!result) {
	    Printer::inst()->print_msg(L0, "Cryptonight hash self-test failed. This might be caused by bad compiler optimizations.");
	}
	return result;
}

minethd::cn_hash_fun minethd::func_selector(xmrstak_algo algo) {
    switch(algo) {
	case cryptonight_lite:
        return cryptonight_hash<cryptonight_lite>;
	case cryptonight:
        return cryptonight_hash<cryptonight>;
	case cryptonight_heavy:
        return cryptonight_hash<cryptonight_heavy>;
	case cryptonight_aeon:
        return cryptonight_hash<cryptonight_aeon>;
	case cryptonight_ipbc:
        return cryptonight_hash<cryptonight_ipbc>;
	case cryptonight_stellite:
        return cryptonight_hash<cryptonight_stellite>;
	case cryptonight_masari:
        return cryptonight_hash<cryptonight_masari>;
	case cryptonight_haven:
        return cryptonight_hash<cryptonight_haven>;
	}
    return cryptonight_hash<cryptonight_monero>;
}

} // namespace cpu
} // namespace xmrstak
