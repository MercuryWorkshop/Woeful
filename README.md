# Woeful
<img src="https://raw.githubusercontent.com/MercuryWorkshop/Woeful/refs/heads/main/public/wordmark.png" height="180">

A high performance [Wisp](https://github.com/MercuryWorkshop/wisp-protocol) server that has packet analysis.

# Features
- Wisp V1 Support
- Pcap network capture
- Blacklisting & Whitelisting ports and domains

# Is it fast?
It's faster then [WispServerCpp](https://github.com/FoxMoss/WispServerCpp/). 
The other benchmarks are more nuanced.
![Benchmark Images](./public/bench.png)

# Dependencies
Big thanks to all of these projects:
- [uWebSockets](https://github.com/uNetworking/uWebSockets) A high performant web socket server library.
- [Cli11](https://github.com/CLIUtils/CLI11.git) for configuration.
- [pugixml](https://github.com/leethomason/tinyxml2) for xml configuration.
- [tl::expected](https://github.com/TartanLlama/expected) a polyfill for the c++23 feature for better error handling.
- [PcapPlusPlus](https://github.com/seladb/PcapPlusPlus.git) for recording traffic going through your wisp server.
- [BS_thread_pool](https://github.com/bshoshany/thread-pool) a basic thread pool library.
