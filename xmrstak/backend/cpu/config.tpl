R"===(
/*
 * Thread configuration for each thread. Make sure it matches the number above.
 * low_power_mode - This can either be a boolean (true or false), or a number between 1 to 5. When set to true,
 *                  this mode will double the cache usage, and double the single thread performance. It will
 *                  consume much less power (as less cores are working), but will max out at around 80-85% of
 *                  the maximum performance. When set to a number N greater than 1, this mode will increase the
 *                  cache usage and single thread performance by N times.
 *
 * no_prefetch -    Some systems can gain up to extra 5% here, but sometimes it will have no difference or make
 *                  things slower.
 *
 * On the first run the miner will look at your system and suggest a basic configuration that will work,
 * you can try to tweak it from there to get the best performance.
 *
 * A filled out configuration should look like this:
 * "cpu_threads_conf" :
 * [
 *      { "low_power_mode" : false, "no_prefetch" : true },
 *      { "low_power_mode" : false, "no_prefetch" : true },
 * ],
 * If you do not wish to mine with your CPU(s) then use:
 * "cpu_threads_conf" :
 * null,
 */

"cpu_threads_conf" :
[
CPUCONFIG
],

)==="
