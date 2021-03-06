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

#include "xmrstak/jconf.hpp"
#include "executor.hpp"
#include "xmrstak/net/jpsock.hpp"

#include "telemetry.hpp"
#include "xmrstak/backend/miner_work.hpp"
#include "xmrstak/backend/GlobalStates.hpp"
#include "xmrstak/backend/backendConnector.hpp"
#include "xmrstak/backend/iBackend.hpp"

#include "xmrstak/jconf.hpp"
#include "xmrstak/misc/console.hpp"
#include "xmrstak/version.hpp"

#include <thread>
#include <string>
#include <cmath>
#include <algorithm>
#include <functional>
#include <assert.h>
#include <time.h>


#ifdef _WIN32
#define strncasecmp _strnicmp
#endif // _WIN32

void Executor::push_timed_event(ex_event&& ev, size_t sec) {
	std::unique_lock<std::mutex> lck(timed_event_mutex);
	lTimedEvents.emplace_back(std::move(ev), sec_to_ticks(sec));
}

void Executor::ex_clock_thd() {
	size_t tick = 0;
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(size_t(iTickTime)));

		push_event(ex_event(EV_PERF_TICK));

		//Eval pool choice every fourth tick
		if((tick++ & 0x03) == 0)
			push_event(ex_event(EV_EVAL_POOL_CHOICE));

		// Service timed events
		std::unique_lock<std::mutex> lck(timed_event_mutex);
		std::list<timed_event>::iterator ev = lTimedEvents.begin();
		while (ev != lTimedEvents.end()) {
			ev->ticks_left--;
			if(ev->ticks_left == 0) {
				push_event(std::move(ev->event));
				ev = lTimedEvents.erase(ev);
			} else {
				ev++;
			}
		}
		lck.unlock();
	}
}

bool Executor::get_live_pools(std::vector<jpsock*>& eval_pools) {
	size_t limit = jconf::inst()->GetGiveUpLimit();
	size_t wait = jconf::inst()->GetNetRetry();

	if(limit == 0) {
		limit = (-1); //No limit = limit of 2^64-1
	}
	size_t pool_count = 0;
	size_t over_limit = 0;
	for(jpsock& pool : pools) {
		// Only eval live pools
		size_t num, dtime;
		pool.get_disconnects(num, dtime);

		if(dtime == 0 || (dtime >= wait && num <= limit)) {
			eval_pools.emplace_back(&pool);
		}

		pool_count++;
		if(num > limit) {
			over_limit++;
		}
	}

	if(eval_pools.size() == 0) {
		if(xmrstak::GlobalStates::inst().pool_id != invalid_pool_id) {
			Printer::inst()->print_msg(L0, "All pools are dead. Idling...");
			auto work = xmrstak::miner_work();
			xmrstak::pool_data dat;
			xmrstak::GlobalStates::inst().switch_work(work, dat);
		}

		if(over_limit == pool_count) {
			Printer::inst()->print_msg(L0, "All pools are over give up limit. Exiting.");
			exit(0);
		}

		return false;
	}

	return true;
}

/*
 * This event is called by the timer and whenever something relevant happens.
 * The job here is to decide if we want to connect, disconnect, or switch jobs (or do nothing)
 */
void Executor::eval_pool_choice() {
	std::vector<jpsock*> eval_pools;
	eval_pools.reserve(pools.size());

	if(!get_live_pools(eval_pools)) {
		return;
	}

	size_t running = 0;
	for(jpsock* pool : eval_pools) {
		if(pool->is_running())
			running++;
	}

	// Special case - if we are without a pool, connect to all find a live pool asap
	if(running == 0) {
		for(jpsock* pool : eval_pools) {
			if(pool->can_connect()) {
				Printer::inst()->print_msg(L1, "Fast-connecting to %s pool ...", pool->get_pool_addr());
				std::string error;
				if(!pool->connect(error))
					log_socket_error(pool, std::move(error));
			}
		}

		return;
	}

	std::sort(eval_pools.begin(), eval_pools.end(), [](jpsock* a, jpsock* b) { return b->get_pool_weight(true) < a->get_pool_weight(true); });
	jpsock* goal = eval_pools[0];

	if(goal->get_pool_id() != xmrstak::GlobalStates::inst().pool_id) {
		if(!goal->is_running() && goal->can_connect()) {
			Printer::inst()->print_msg(L1, "Connecting to %s pool ...", goal->get_pool_addr());

			std::string error;
			if(!goal->connect(error)) {
				log_socket_error(goal, std::move(error));
			}
			return;
		}

		if(goal->is_logged_in()) {
			pool_job oPoolJob;
			if(!goal->get_current_job(oPoolJob)) {
				goal->disconnect();
				return;
			}

			size_t prev_pool_id = current_pool_id;
			current_pool_id = goal->get_pool_id();
			on_pool_have_job(current_pool_id, oPoolJob);

			jpsock* prev_pool = pick_pool_by_id(prev_pool_id);
			reset_stats();
			last_usr_pool_id = invalid_pool_id;
			return;
		}
	} else {
		/* All is good - but check if we can do better */
		std::sort(eval_pools.begin(), eval_pools.end(), [](jpsock* a, jpsock* b) { return b->get_pool_weight(false) < a->get_pool_weight(false); });
		jpsock* goal2 = eval_pools[0];

		if(goal->get_pool_id() != goal2->get_pool_id()) {
			if(!goal2->is_running() && goal2->can_connect()) {
				Printer::inst()->print_msg(L1, "Background-connect to %s pool ...", goal2->get_pool_addr());
				std::string error;
				if(!goal2->connect(error)) {
					log_socket_error(goal2, std::move(error));
				}
				return;
			}
		}
	}

	for(jpsock& pool : pools) {
		if(goal->is_logged_in() && pool.is_logged_in() && pool.get_pool_id() != goal->get_pool_id()) {
			pool.disconnect(true);
		}
	}
}

void Executor::log_socket_error(jpsock* pool, std::string&& sError) {
	std::string pool_name;
	pool_name.reserve(128);
	pool_name.append("[").append(pool->get_pool_addr()).append("] ");
	sError.insert(0, pool_name);

	vSocketLog.emplace_back(std::move(sError));
	Printer::inst()->print_msg(L1, "SOCKET ERROR - %s", vSocketLog.back().msg.c_str());

	push_event(ex_event(EV_EVAL_POOL_CHOICE));
}

void Executor::log_result_error(std::string&& sError) {
	size_t i = 1, ln = vMineResults.size();
	for(; i < ln; i++) {
		if(vMineResults[i].compare(sError)) {
			vMineResults[i].increment();
			break;
		}
	}

	//Not found
	if(i == ln) {
		vMineResults.emplace_back(std::move(sError));
	} else {
		sError.clear();
	}
}

void Executor::log_result_ok(uint64_t iActualDiff) {
	iPoolHashes += iPoolDiff;

	size_t ln = iTopDiff.size() - 1;
	if(iActualDiff > iTopDiff[ln]) {
		iTopDiff[ln] = iActualDiff;
		std::sort(iTopDiff.rbegin(), iTopDiff.rend());
	}

	vMineResults[0].increment();
}

jpsock* Executor::pick_pool_by_id(size_t pool_id) {
	if(pool_id == invalid_pool_id) {
		return nullptr;
	}

	for(jpsock& pool : pools) {
		if (pool.get_pool_id() == pool_id) {
			return &pool;
		}
	}

	return nullptr;
}

void Executor::on_sock_ready(size_t pool_id) {
	jpsock* pool = pick_pool_by_id(pool_id);

	Printer::inst()->print_msg(L1, "Pool %s connected. Logging in...", pool->get_pool_addr());

	if(!pool->cmd_login()) {
		if(pool->have_call_error()) {
			std::string str = "Login error: " +  pool->get_call_error();
			log_socket_error(pool, std::move(str));
		}

		if(!pool->have_sock_error())
			pool->disconnect();
	}
}

void Executor::on_sock_error(size_t pool_id, std::string&& sError, bool silent)
{
	jpsock* pool = pick_pool_by_id(pool_id);

	pool->disconnect();

	if(pool_id == current_pool_id)
		current_pool_id = invalid_pool_id;

	if(silent)
		return;

	log_socket_error(pool, std::move(sError));
}

void Executor::on_pool_have_job(size_t pool_id, pool_job& oPoolJob)
{
	if(pool_id != current_pool_id)
		return;

	jpsock* pool = pick_pool_by_id(pool_id);

	xmrstak::miner_work oWork(oPoolJob.sJobID, oPoolJob.bWorkBlob, oPoolJob.iWorkLen, oPoolJob.iTarget, pool->is_nicehash(), pool_id);

	xmrstak::pool_data dat;
	dat.iSavedNonce = oPoolJob.iSavedNonce;
	dat.pool_id = pool_id;

	xmrstak::GlobalStates::inst().switch_work(oWork, dat);

	if(dat.pool_id != pool_id)
	{
		jpsock* prev_pool;
		if((prev_pool = pick_pool_by_id(dat.pool_id)) != nullptr)
			prev_pool->save_nonce(dat.iSavedNonce);
	}

	if(iPoolDiff != pool->get_current_diff())
	{
		iPoolDiff = pool->get_current_diff();
		Printer::inst()->print_msg(L2, "Difficulty changed. Now: %llu.", int_port(iPoolDiff));
	}

	if(dat.pool_id != pool_id)
	{
		jpsock* prev_pool;
		if(dat.pool_id != invalid_pool_id && (prev_pool = pick_pool_by_id(dat.pool_id)) != nullptr)
		{
			Printer::inst()->print_msg(L2, "Pool switched.");
		}
		else
			Printer::inst()->print_msg(L2, "Pool logged in.");
	}
	else
		Printer::inst()->print_msg(L3, "New block detected.");
}

void Executor::on_miner_result(size_t pool_id, job_result& oResult)
{
	jpsock* pool = pick_pool_by_id(pool_id);

	const char* backend_name = xmrstak::iBackend::getName(pvThreads->at(oResult.iThreadId)->backendType);
	uint64_t backend_hashcount, total_hashcount = 0;

	backend_hashcount = pvThreads->at(oResult.iThreadId)->iHashCount.load(std::memory_order_relaxed);
	for(size_t i = 0; i < pvThreads->size(); i++)
		total_hashcount += pvThreads->at(i)->iHashCount.load(std::memory_order_relaxed);

	if (!pool->is_running() || !pool->is_logged_in())
	{
		log_result_error("[NETWORK ERROR]");
		return;
	}

	size_t t_start = get_timestamp_ms();
	bool bResult = pool->cmd_submit(oResult.sJobID, oResult.iNonce, oResult.bResult,
		backend_name, backend_hashcount, total_hashcount, oResult.algorithm
	);
	size_t t_len = get_timestamp_ms() - t_start;

	if(t_len > 0xFFFF)
		t_len = 0xFFFF;
	iPoolCallTimes.push_back((uint16_t)t_len);

	if(bResult)
	{
		uint64_t* targets = (uint64_t*)oResult.bResult;
		log_result_ok(jpsock::t64_to_diff(targets[3]));
		Printer::inst()->print_msg(L3, "Result accepted by the pool.");
	}
	else
	{
		if(!pool->have_sock_error())
		{
			Printer::inst()->print_msg(L3, "Result rejected by the pool.");

			std::string error = pool->get_call_error();

			if(strncasecmp(error.c_str(), "Unauthenticated", 15) == 0)
			{
				Printer::inst()->print_msg(L2, "Your miner was unable to find a share in time. Either the pool difficulty is too high, or the pool timeout is too low.");
				pool->disconnect();
			}

			log_result_error(std::move(error));
		}
		else
			log_result_error("[NETWORK ERROR]");
	}
}

#ifndef _WIN32

#include <signal.h>
void disable_sigpipe()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if (sigaction(SIGPIPE, &sa, 0) == -1)
		Printer::inst()->print_msg(L1, "ERROR: Call to sigaction failed!");
}

#else
inline void disable_sigpipe() {}
#endif

void Executor::ex_main()
{
	disable_sigpipe();

	assert(1000 % iTickTime == 0);

	xmrstak::miner_work oWork = xmrstak::miner_work();

	// \todo collect all backend threads
	pvThreads = xmrstak::BackendConnector::thread_starter(oWork);

	if(pvThreads->size()==0)
	{
		Printer::inst()->print_msg(L1, "ERROR: No miner backend enabled.");
		win_exit();
	}

	telem = new xmrstak::telemetry(pvThreads->size());

	size_t pc = jconf::inst()->GetPoolCount();
	bool already_have_cli_pool = false;
	size_t i=0;
	for(; i < pc; i++)
	{
		jconf::pool_cfg cfg;
 		jconf::inst()->GetPoolConfig(i, cfg);
#ifdef CONF_NO_TLS
		if(cfg.tls) {
			Printer::inst()->print_msg(L1, "ERROR: No miner was compiled without TLS support.");
			win_exit();
		}
#endif
		if(!xmrstak::params::inst().poolURL.empty() && xmrstak::params::inst().poolURL == cfg.sPoolAddr) {
			auto& params = xmrstak::params::inst();
			already_have_cli_pool = true;

			const char* wallet = params.poolUsername.empty() ? cfg.sWalletAddr : params.poolUsername.c_str();
			const char* rigid = params.userSetRigid ? params.poolRigid.c_str() : cfg.sRigId;
			const char* pwd = params.userSetPwd ? params.poolPasswd.c_str() : cfg.sPasswd;
			bool nicehash = cfg.nicehash || params.nicehashMode;

			pools.emplace_back(i+1, cfg.sPoolAddr, wallet, rigid, pwd, 9.9, params.poolUseTls, cfg.tls_fingerprint, nicehash);
		} else {
			pools.emplace_back(i+1, cfg.sPoolAddr, cfg.sWalletAddr, cfg.sRigId, cfg.sPasswd, cfg.weight, cfg.tls, cfg.tls_fingerprint, cfg.nicehash);
		}
	}

	if(!xmrstak::params::inst().poolURL.empty() && !already_have_cli_pool)
	{
		auto& params = xmrstak::params::inst();
		if(params.poolUsername.empty())
		{
			Printer::inst()->print_msg(L1, "ERROR: You didn't specify the username / wallet address for %s", xmrstak::params::inst().poolURL.c_str());
			win_exit();
		}

		pools.emplace_back(i+1, params.poolURL.c_str(), params.poolUsername.c_str(), params.poolRigid.c_str(), params.poolPasswd.c_str(), 9.9, params.poolUseTls, "", params.nicehashMode);
	}

	ex_event ev;
	std::thread clock_thd(&Executor::ex_clock_thd, this);

	eval_pool_choice();

	// Place the default success result at position 0, it needs to
	// be here even if our first result is a failure
	vMineResults.emplace_back();

	// If the user requested it, start the autohash Printer
	if(jconf::inst()->GetVerboseLevel() >= 4)
		push_timed_event(ex_event(EV_HASHRATE_LOOP), jconf::inst()->GetAutohashTime());

	size_t cnt = 0;
	while (true)
	{
		ev = oEventQ.pop();
		switch (ev.iName)
		{
		case EV_SOCK_READY:
			on_sock_ready(ev.iPoolId);
			break;

		case EV_SOCK_ERROR:
			on_sock_error(ev.iPoolId, std::move(ev.oSocketError.sSocketError), ev.oSocketError.silent);
			break;

		case EV_POOL_HAVE_JOB:
			on_pool_have_job(ev.iPoolId, ev.oPoolJob);
			break;

		case EV_MINER_HAVE_RESULT:
			on_miner_result(ev.iPoolId, ev.oJobResult);
			break;

		case EV_EVAL_POOL_CHOICE:
			eval_pool_choice();
			break;

		case EV_GPU_RES_ERROR:
			log_result_error(std::string(ev.oGpuError.error_str + std::string(" GPU ID ") + std::to_string(ev.oGpuError.idx)));
			break;

		case EV_PERF_TICK:
			for (i = 0; i < pvThreads->size(); i++)
				telem->push_perf_value(i, pvThreads->at(i)->iHashCount.load(std::memory_order_relaxed),
				pvThreads->at(i)->iTimestamp.load(std::memory_order_relaxed));

			if((cnt++ & 0xF) == 0) //Every 16 ticks
			{
				double fHps = 0.0;
				double fTelem;
				bool normal = true;

				for (i = 0; i < pvThreads->size(); i++)
				{
					fTelem = telem->calc_telemetry_data(10000, i);
					if(std::isnormal(fTelem))
					{
						fHps += fTelem;
					}
					else
					{
						normal = false;
						break;
					}
				}

				if(normal && fHighestHps < fHps)
					fHighestHps = fHps;
			}
			break;

		case EV_USR_HASHRATE:
		case EV_USR_RESULTS:
		case EV_USR_CONNSTAT:
			print_report(ev.iName);
			break;

		case EV_HASHRATE_LOOP:
			print_report(EV_USR_HASHRATE);
			push_timed_event(ex_event(EV_HASHRATE_LOOP), jconf::inst()->GetAutohashTime());
			break;

		case EV_INVALID_VAL:
		default:
			assert(false);
			break;
		}
	}
}

inline const char* hps_format(double h, char* buf, size_t l)
{
	if(std::isnormal(h) || h == 0.0)
	{
		snprintf(buf, l, " %6.1f", h);
		return buf;
	}
	else
		return "   (na)";
}

bool Executor::motd_filter_console(std::string& motd)
{
	if(motd.size() > motd_max_length)
		return false;

	motd.erase(std::remove_if(motd.begin(), motd.end(), [](int chr)->bool { return !((chr >= 0x20 && chr <= 0x7e) || chr == '\n');}), motd.end());
	return motd.size() > 0;
}

void Executor::hashrate_report(std::string& out)
{
	out.reserve(2048 + pvThreads->size() * 64);

	if(jconf::inst()->PrintMotd())
	{
		std::string motd;
		for(jpsock& pool : pools)
		{
			motd.empty();
			if(pool.get_pool_motd(motd) && motd_filter_console(motd))
			{
				out.append("Message from ").append(pool.get_pool_addr()).append(":\n");
				out.append(motd).append("\n");
				out.append("-----------------------------------------------------\n");
			}
		}
	}

	char num[32];
	double fTotal[3] = { 0.0, 0.0, 0.0};

	for( uint32_t b = 0; b < 4u; ++b)
	{
		std::vector<xmrstak::iBackend*> backEnds;
		std::copy_if(pvThreads->begin(), pvThreads->end(), std::back_inserter(backEnds),
			[&](xmrstak::iBackend* backend)
			{
				return backend->backendType == b;
			}
		);

		size_t nthd = backEnds.size();
		if(nthd != 0)
		{
			size_t i;
			auto bType = static_cast<xmrstak::iBackend::BackendType>(b);
			std::string name(xmrstak::iBackend::getName(bType));
			std::transform(name.begin(), name.end(), name.begin(), ::toupper);

			out.append("HASHRATE REPORT - ").append(name).append("\n");
			out.append("| ID |    10s |    60s |    15m |");
			if(nthd != 1)
				out.append(" ID |    10s |    60s |    15m |\n");
			else
				out.append(1, '\n');

			double fTotalCur[3] = { 0.0, 0.0, 0.0};
			for (i = 0; i < nthd; i++)
			{
				double fHps[3];

				uint32_t tid = backEnds[i]->iThreadNo;
				fHps[0] = telem->calc_telemetry_data(10000, tid);
				fHps[1] = telem->calc_telemetry_data(60000, tid);
				fHps[2] = telem->calc_telemetry_data(900000, tid);

				snprintf(num, sizeof(num), "| %2u |", (unsigned int)i);
				out.append(num);
				out.append(hps_format(fHps[0], num, sizeof(num))).append(" |");
				out.append(hps_format(fHps[1], num, sizeof(num))).append(" |");
				out.append(hps_format(fHps[2], num, sizeof(num))).append(1, ' ');

				fTotal[0] += (std::isnormal(fHps[0])) ? fHps[0] : 0.0;
				fTotal[1] += (std::isnormal(fHps[1])) ? fHps[1] : 0.0;
				fTotal[2] += (std::isnormal(fHps[2])) ? fHps[2] : 0.0;

				fTotalCur[0] += (std::isnormal(fHps[0])) ? fHps[0] : 0.0;
				fTotalCur[1] += (std::isnormal(fHps[1])) ? fHps[1] : 0.0;
				fTotalCur[2] += (std::isnormal(fHps[2])) ? fHps[2] : 0.0;

				if((i & 0x1) == 1) //Odd i's
					out.append("|\n");
			}

			if((i & 0x1) == 1) //We had odd number of threads
				out.append("|\n");

			out.append("Totals (").append(name).append("): ");
			out.append(hps_format(fTotalCur[0], num, sizeof(num)));
			out.append(hps_format(fTotalCur[1], num, sizeof(num)));
			out.append(hps_format(fTotalCur[2], num, sizeof(num)));
			out.append(" H/s\n");

			out.append("-----------------------------------------------------------------\n");
		}
	}

	out.append("Totals (ALL):  ");
	out.append(hps_format(fTotal[0], num, sizeof(num)));
	out.append(hps_format(fTotal[1], num, sizeof(num)));
	out.append(hps_format(fTotal[2], num, sizeof(num)));
	out.append(" H/s\nHighest: ");
	out.append(hps_format(fHighestHps, num, sizeof(num)));
	out.append(" H/s\n");
	out.append("-----------------------------------------------------------------\n");
}

char* time_format(char* buf, size_t len, std::chrono::system_clock::time_point time)
{
	time_t ctime = std::chrono::system_clock::to_time_t(time);
	tm stime;

	/*
	 * Oh for god's sake... this feels like we are back to the 90's...
	 * and don't get me started on lack strcpy_s because NIH - use non-standard strlcpy...
	 * And of course C++ implements unsafe version because... reasons
	 */

#ifdef _WIN32
	localtime_s(&stime, &ctime);
#else
	localtime_r(&ctime, &stime);
#endif // __WIN32
	strftime(buf, len, "%F %T", &stime);

	return buf;
}

void Executor::result_report(std::string& out)
{
	char num[128];
	char date[32];

	out.reserve(1024);

	size_t iGoodRes = vMineResults[0].count, iTotalRes = iGoodRes;
	size_t ln = vMineResults.size();

	for(size_t i=1; i < ln; i++)
		iTotalRes += vMineResults[i].count;

	out.append("RESULT REPORT\n");
	if(iTotalRes == 0)
	{
		out.append("You haven't found any results yet.\n");
		return;
	}

	double dConnSec;
	{
		using namespace std::chrono;
		dConnSec = (double)duration_cast<seconds>(system_clock::now() - tPoolConnTime).count();
	}

	snprintf(num, sizeof(num), " (%.1f %%)\n", 100.0 * iGoodRes / iTotalRes);

	out.append("Difficulty       : ").append(std::to_string(iPoolDiff)).append(1, '\n');
	out.append("Good results     : ").append(std::to_string(iGoodRes)).append(" / ").
		append(std::to_string(iTotalRes)).append(num);

	if(iPoolCallTimes.size() != 0)
	{
		// Here we use iPoolCallTimes since it also gets reset when we disconnect
		snprintf(num, sizeof(num), "%.1f sec\n", dConnSec / iPoolCallTimes.size());
		out.append("Avg result time  : ").append(num);
	}
	out.append("Pool-side hashes : ").append(std::to_string(iPoolHashes)).append(2, '\n');
	out.append("Top 10 best results found:\n");

	for(size_t i=0; i < 10; i += 2)
	{
		snprintf(num, sizeof(num), "| %2llu | %16llu | %2llu | %16llu |\n",
			int_port(i), int_port(iTopDiff[i]), int_port(i+1), int_port(iTopDiff[i+1]));
		out.append(num);
	}

	out.append("\nError details:\n");
	if(ln > 1)
	{
		out.append("| Count | Error text                       | Last seen           |\n");
		for(size_t i=1; i < ln; i++)
		{
			snprintf(num, sizeof(num), "| %5llu | %-32.32s | %s |\n", int_port(vMineResults[i].count),
				vMineResults[i].msg.c_str(), time_format(date, sizeof(date), vMineResults[i].time));
			out.append(num);
		}
	}
	else
		out.append("Yay! No errors.\n");
}

void Executor::connection_report(std::string& out)
{
	char num[128];
	char date[32];

	out.reserve(512);

	jpsock* pool = pick_pool_by_id(current_pool_id);

	out.append("CONNECTION REPORT\n");
	out.append("Pool address    : ").append(pool != nullptr ? pool->get_pool_addr() : "<not connected>").append(1, '\n');
	if(pool != nullptr && pool->is_running() && pool->is_logged_in())
		out.append("Connected since : ").append(time_format(date, sizeof(date), tPoolConnTime)).append(1, '\n');
	else
		out.append("Connected since : <not connected>\n");

	size_t n_calls = iPoolCallTimes.size();
	if (n_calls > 1)
	{
		//Not-really-but-good-enough median
		std::nth_element(iPoolCallTimes.begin(), iPoolCallTimes.begin() + n_calls/2, iPoolCallTimes.end());
		out.append("Pool ping time  : ").append(std::to_string(iPoolCallTimes[n_calls/2])).append(" ms\n");
	}
	else
		out.append("Pool ping time  : (n/a)\n");

	out.append("\nNetwork error log:\n");
	size_t ln = vSocketLog.size();
	if(ln > 0)
	{
		out.append("| Date                | Error text                                             |\n");
		for(size_t i=0; i < ln; i++)
		{
			snprintf(num, sizeof(num), "| %s | %-54.54s |\n",
				time_format(date, sizeof(date), vSocketLog[i].time), vSocketLog[i].msg.c_str());
			out.append(num);
		}
	}
	else
		out.append("Yay! No errors.\n");
}

void Executor::print_report(ex_event_name ev)
{
	std::string out;
	switch(ev)
	{
	case EV_USR_HASHRATE:
		hashrate_report(out);
		break;

	case EV_USR_RESULTS:
		result_report(out);
		break;

	case EV_USR_CONNSTAT:
		connection_report(out);
		break;
	default:
		assert(false);
		break;
	}

	Printer::inst()->print_str(out.c_str());
}
