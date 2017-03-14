# DanglingPointerMiner
The pride of UWindsor cryptocurrency mining.

UWindsor Team A's CS Coin miner for the 2017 CS Games.

Use 'git clone --recursive -j[nprocs] [repo]' to clone this repo and all of its submodules.

My present thinking on how to structure this is to have a "coordinating" process that communicates with the server and spawns and kills solvers as needed.
This process will listen on a local port (127.0.0.454) for solutions from the solvers.
This WebSocket process will probably be written in Go, since their standard library has everything it needs.

Then we will have a bunch of Solver processes for each problem that might be posed to us.
These will take arguments through command line flags, and run until the WebSocket process kills them.
These will generate nonce's, solve those nonce's and check if it matches the given prefix.
If it does they will send a packet to the WebSocket process, which will double check the prefix and forward it to the server.

This allows us to very cleanly seperate each Solver and the buisness of Web Sockets and Wallets.

We should try to get everything to build statically to avoid environment issues when the rubber hits the road.
