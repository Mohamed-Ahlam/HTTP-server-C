# HTTP-server-C

Part 1 of the project, I implemented a simple HTTP server. I did the necessary socket setup, parse incoming client HTTP requests, and then replied with valid HTTP responses communicating the contents of files stored locally on disk.

In Part 2 of the project, I converted a single-threaded HTTP server into a multi-threaded server with a design much closer to those of the real servers that power the modern web. In particular, I used the thread pool paradigm to create a fixed-size collection of threads each capable of interacting with one client.
