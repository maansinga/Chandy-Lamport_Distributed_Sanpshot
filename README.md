# Chandy Lamport Distributed Snapshot Implementation
#### THIS IS AN ACAD. SUBMISSION. PLEASE DON'T MAKE DERIVATIVES or COPY IT.
## Team
### Java Version
--------
[Haneesha] Gurugubelli - Java
Sunkollu Nagaraj [Sushma] - Java

### C Versiono
--------
Sree Teja [Simha] Gemaraju - C

## About
Chandy Lamport algorithm can be found here 
https://citemaster.net/get/e079492a-7aea-11e4-8773-00163e009cc7/10.1.1.119.7694.pdf

In the current implementation or my C implementation, a shell script launches 10 to 50 instances of distributed nodes. Every node
has a set number of messages to be exchnged randomly with neighboring nodes. Once in a while, the assigned root node(0th node) shall call for 
a system wide snapshot. This is the part where the Chandy Lamport Snapshots come in. Using the Chandy Lamport protocol, the snapshots of the
individual systems are taken. The node passon their version of snapshot to the invocator recursively and finally feeeding the master 
invovator - the root node to gather all the snapshots. The snapshots are verified for consistency.

## How to Run
**Compilation:**
	make

**Launch:**
	sh launcher.sh

**Cleanup:(Delete logfiles and dumpfiles)**
	sh cleanup.sh

**Force Termination:(If a run has failed)**
	sh killall.sh

**Purge:(Sanitize system)**
	sh purge.sh
