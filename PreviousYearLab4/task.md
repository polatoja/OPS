Matching bits 

Write client/server UDP(INET do-main)application. Client generates a random 7 digit number, prints it on stdout then sends it in binary form to the server. Server holds internal key (the first input parameter, 7 digit number) and an internal probability value(the second input parameter from the range [0 100]). The server reads the data from client and with given probability, sends back a sum of a key and received value( modulo le8).

If client is able to read the message it prints out received value. Otherwise it informs that no answer was received. The server is also able to read the commands from the terminal. If U was typed, the probability is increased by 10% (still within 0-100 range). If D - decreased by 10%. E-termina the sever, P prints the local key and probability.

Both client and server application must be single process and single threaded.

Stages:
1. Client sends random number to the server, server just prints everything it receives. 

2. Server always sends responses as described, client waits for it and prints it.

3. The server is closed on E and prints values on P.

4. Server sometimes does not send the response (along with handling D and U commands)




匹配位数
编写一个使用 UDP（INET 域）的客户端/服务器应用程序。客户端生成一个随机的7位数，将其打印在标准输出上，然后以二进制形式发送给服务器。服务器持有一个内部密钥（第一个输入参数，一个7位数）和一个内部概率值（第二个输入参数，范围在 [0, 100]）。服务器读取来自客户端的数据，并根据给定的概率，发送回密钥和接收值之和（模10^8)

如果客户端能读取到消息，它会打印接收到的值。否则，它会通知没有收到回答。服务器也能读取来自终端的命令。如果输入了 U，概率增加10%（仍在0-100%范围内）。如果输入了 D，概率减少10%。E 命令终止服务器，P 命令打印本地密钥和概率值。

客户端和服务器应用程序都必须是单进程和单线程的。

阶段：

1. 客户端发送随机数给服务器，服务器仅打印它接收到的所有内容。
2. 服务器始终按描述发送响应，客户端等待并打印它。
3. 服务器在输入 E 时关闭，并在输入 P 时打印值。
4. 服务器有时不发送响应（同时处理 D 和 U 命令）。


这个任务涉及到网络编程的核心概念，如数据的发送和接收，以及如何通过简单的命令控制逻辑动态调整服务器的行为。如果你需要进一步的解释或示例代码，随时可以提问。