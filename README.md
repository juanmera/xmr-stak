###### fireice-uk's and psychocrypt's
# XMR-Stak - Cryptonight All-in-One Mining Software

**XMR-Stak is ready for the POW change of Monero-v7, Aeon-v7, stellite-v4 and Sumukoin-v3**

XMR-Stak is a universal Stratum pool miner. This miner supports AMD gpus and can be used to mine the crypto currency Monero and Aeon.

## Video setup guide on Windows

[<img src="https://gist.githubusercontent.com/fireice-uk/3621b179d56f57a8ead6303d8e415cf6/raw/f572faba67cc9418116f3c1dfd7783baf52182ce/vidguidetmb.jpg">](https://youtu.be/YNMa8NplWus)
###### Video by Crypto Sewer

## Overview
* [Features](#features)
* [Supported altcoins](#supported-altcoins)
* [Download](#download)
* [Usage](doc/usage.md)
* [HowTo Compile](doc/compile.md)
* [FAQ](doc/FAQ.md)
* [Developer Donation](#default-developer-donation)
* [Developer PGP Key's](doc/pgp_keys.md)

## Features

- support AMD-GPU
- support all common OS (Linux, Windows and macOS)
- supports algorithm cryptonight for Monero (XMR) and cryptonight-light (AEON)
- easy to use
  - guided start (no need to edit a config file for the first start)
  - auto configuration for each backend
- open source software (GPLv3)
- TLS support

## Supported altcoins

Besides [Monero](https://getmonero.org), following coins can be mined using this miner:

- [Aeon](http://www.aeon.cash)
- [BBSCoin](https://www.bbscoin.xyz)
- [Graft](https://www.graft.network)
- [Haven](https://havenprotocol.com)
- [Intense](https://intensecoin.com)
- [Masari](https://getmasari.org)
- [Ryo](https://ryo-currency.com)
- [TurtleCoin](https://turtlecoin.lol)

If your prefered coin is not listed, you can chose one of the following algorithms:

- 1MiB scratchpad memory
    - cryptonight_lite
    - cryptonight_lite_v7
    - cryptonight_lite_v7_xor (algorithm used by ipbc)
- 2MiB scratchpad memory
    - cryptonight
    - cryptonight_masari
    - cryptonight_v7
    - cryptonight_v7_stellite
- 4MiB scratchpad memory
    - cryptonight_haven
    - cryptonight_heavy

Please note, this list is not complete, and is not an endorsement.

## Download

You can find the latest releases and precompiled binaries on GitHub under [Releases](https://github.com/fireice-uk/xmr-stak/releases).
