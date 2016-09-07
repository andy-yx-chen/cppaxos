# cppaxos

> This project is for demostration only

## Why this project
Lamport keep saying that paxos is very simple, and yes, it is. 

This project is just to demostrate how simple it is. The core algorithm only needs 228 lines of code, including comments and empty lines. That's amazing (comparing to Raft, full implementation requires 1400+ lines of code)

## How to play with it

It's pure c++ code, no OS specific code (thanks to C++ 11), it's shipped with Windows Make file (Makefile.win), all you need to do is

> nmake -f Makfile.win all

> debug\obj\cppaxos.exe

it's all console application, however, very easy to play with

## How far is it to production

If you wan to reuse the code for production, I think it's not far away, just replace the in-memory stuff defined in paxos_server class with real storage, then you will get a Paxos without leader election (the consistency is guaranteed, but progress is not)

## More questions

contact me if you need more details
