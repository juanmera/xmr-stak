# Tuning Guide

## Content Overview
* [Benchmark](#benchmark)
* [Windows](#windows)
* [AMD Backend](#amd-backend)
  * [Choose `intensity` and `worksize`](#choose-intensity-and-worksize)
  * [Add more GPUs](#add-more-gpus)
  * [disable comp_mode](#disable-comp_mode)
  * [change the scratchpad memory pattern](change-the-scratchpad-memory-pattern)
  * [Increase Memory Pool](#increase-memory-pool)
  * [Scratchpad Indexing](#scratchpad-indexing)
* [CPU Backend](#cpu-backend)
  * [Choose Value for `low_power_mode`](#choose-value-for-low_power_mode)

## Benchmark
To benchmark the miner speed there are two ways.
  - Mine against a pool end press the key `h` after 30 sec to see the hash report.
  - Start the miner with the cli option `--benchmark BLOCKVERSION`. The miner will not connect to any pool and performs a 60sec performance benchmark with all enabled back-ends.

## Windows
"Run As Administrator" prompt (UAC) confirmation is needed to use large pages on Windows 7.
On Windows 10 it is only needed once to set up the account to use them.
Disable the dialog with the command line option `--noUAC`


## AMD Backend

By default the AMD backend can be tuned in the config file `amd.txt`

### Choose `intensity` and `worksize`

Intensity means the number of threads used to mine. The maximum intensity is GPU_MEMORY_MB / 2 - 128, however for cards with 4GB and more, the optimum is likely to be lower than that.
`worksize` is the number of threads working together to increase the miner performance.
In the most cases a `worksize` of `16` or `8` is optimal.

### Add More GPUs

To add a new GPU you need to add a new config set to `gpu_threads_conf`. `index` is the OpenCL index of the gpu.
`platform_index`is the index of the OpenCL platform AMD.
If you are unsure of either GPU or platform index value, you can use `clinfo` tool that comes with AMD APP SDK to dump the values.

```
"gpu_threads_conf" :
[
    { "index" : 0, "intensity" : 1000, "worksize" : 8,
      "strided_index" : true, "mem_chunk" : 2, "comp_mode" : true
    },
    { "index" : 1, "intensity" : 1000, "worksize" : 8,
      "strided_index" : true, "mem_chunk" : 2, "comp_mode" : true
    },
],

"platform_index" : 0,
```

### disable comp_mode

`comp_mode` means compatibility mode and removes some checks in compute kernel those takes care that the miner can be used on a wide range of AMD/OpenCL GPU devices.
To avoid miner crashes the `intensity` should be a multiple of `worksize` if `comp_mode` is `false`.

### change the scratchpad memory pattern

By changing `strided_index` to `2` the number of contiguous elements (a 16 byte) for one miner thread can be fine tuned with the option `mem_chunk`.

### Increase Memory Pool

By setting the following environment variables before the miner is started OpenCl allows the miner to more threads.
This variables must be set each time before the miner is started else it could be that the miner can not allocate enough memory and is crashing.

```
export GPU_FORCE_64BIT_PTR=1
export GPU_MAX_HEAP_SIZE=100
export GPU_MAX_ALLOC_PERCENT=100
export GPU_SINGLE_ALLOC_PERCENT=100
```

*Note:* Windows user must use `set` instead of `export` to define an environment variable.

### Scratchpad Indexing

The layout of the hash scratchpad memory can be changed for each GPU with the option `strided_index` in `amd.txt`.
Try to change the value from the default `true` to `false`.

### Choose Value for `low_power_mode`

The optimal value for `low_power_mode` depends on the cache size of your CPU, and the number of threads.

The `low_power_mode` can be set to a number between `1` to `5`. When set to a value `N` greater than `1`, this mode increases the single thread performance by `N` times, but also requires at least `2*N` MB of cache per thread. It can also be set to `false` or `true`. The value `false` is equivalent to `1`, and `true` is equivalent to `2`.

This setting is particularly useful for CPUs with very large cache. For example the Intel Crystal Well Processors are equipped with 128MB L4 cache, enough to run 8 threads at an optimal `low_power_mode` value of `5`.
