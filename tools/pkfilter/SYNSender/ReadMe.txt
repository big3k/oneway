Index (Attacker)
-------------------

1. How does it work ?
2. Requirments !
3. Contact us.

1. How does it work ?
---------------------

In this software there are 3 kinds of attacks:

A. Syn flood:

TCP requires a three way handshake - when I my computer wants to connect
it sends out a SYN packet, the other computer sends back a SYN+ACK packet
(saying I'm ready to connect), and my computer sends back a SYN+ACK (saying connection
established).

When I send only the SYN, and spoof the address of a non working address
(no host over there to reset the connection), the remote system will never
get the SYN+ACK response and it will wait until that connection will time out
(around 20 secons), assume I'll send 60,000 of this sockets, the amout of
resource I'll tie up will do some damage.

B. UDP flooding:

Sends alot of UDP data from a spoofed address.

C. Echo attach:

If you find an echo service (on port 7), the attacker will send a packet
to port 7 on that system (from port 7, and it will put a spoofed localhost address)
The echo service will reply to that address and port (which is itself)
and will end up in an infinite loop.

2. Requirments !
---------------

Currently working only under w2k.

3. Contact us.
--------------

Site : http://www.komodia.com
email : barak@komodia.com
