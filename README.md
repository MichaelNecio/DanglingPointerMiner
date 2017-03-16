# DanglingPointerMiner
The pride of UWindsor cryptocurrency mining.

UWindsor Team A's CS Coin miner for the 2017 CS Games.

Use 'git clone --recursive -j[nprocs] [repo]' to clone this repo and all of its submodules.

My present thinking on how to structure this is to have a "coordinating" process that communicates with the server and spawns and kills Solver processes as needed.
This process will listen on a local port (127.0.0.454) for solutions from the solvers.

This allows us to very cleanly seperate each solver and the buisness of Web Sockets and Wallets.

We should try to get everything to build statically to avoid environment issues when the rubber hits the road.
