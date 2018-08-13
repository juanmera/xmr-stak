#pragma once

#include "xmrstak/backend/cryptonight.hpp"

#include <stdlib.h>
#include <string>


namespace xmrstak
{
	struct coinDescription
	{
		xmrstak_algo algo;
		xmrstak_algo algo_root;
		uint8_t fork_version;

		coinDescription() = default;

		inline xmrstak_algo GetMiningAlgo() const { return algo; }
		inline xmrstak_algo GetMiningAlgoRoot() const { return algo_root; }
		inline uint8_t GetMiningForkVersion() const { return fork_version; }
	};

	struct coin_selection
	{
		const char* coin_name = nullptr;
		/* [0] -> user pool
		 * [1] -> dev pool
		 */
		coinDescription pool_coin[2];

		coin_selection() = default;

		coin_selection(
			const char* in_coin_name,
			const coinDescription user_coinDescription,
			const coinDescription dev_coinDescription
		) :
			coin_name(in_coin_name)
		{
			pool_coin[0] = user_coinDescription;
			pool_coin[1] = dev_coinDescription;
		}

		inline coinDescription GetDescription() const {
			coinDescription tmp = pool_coin[0];
			return tmp;
		}
	};
} // namespace xmrstak
