#pragma once

class Printer;
class jconf;
class Executor;

namespace xmrstak
{

struct GlobalStates;
struct params;

struct Environment {
	static inline Environment& inst(Environment* init = nullptr) {
		static Environment* env = nullptr;

		if(env == nullptr) {
			if(init == nullptr) {
				env = new Environment;
			} else {
				env = init;
			}
		}
		return *env;
	}

	Printer* printer = nullptr;
	GlobalStates* pglobalStates = nullptr;
	jconf* pJconfConfig = nullptr;
	Executor* pExecutor = nullptr;
	params* pParams = nullptr;
};

} // namespace xmrstak
