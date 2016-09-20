# UDP sender and receiver

1. After complilation, the sender (blaster) is invoked by:<br>
  blaster -s \<hostname\> -p \<port\> -r \<rate\> -n \<num\> -q \<seq_no\> -l \<length\> -c \<echo\><br>
    \<hostname\> is the hostname of the blastee,<br>
    \<port\> is the port on which the blastee is running,<br>
    \<rate\> is the number of packets to be sent per second,<br>
    \<num\> is the number of packets to be sent by the blaster,<br>
    \<seq_no\> is the initial sequence of the packet exchange,<br>
    \<length\> is the length of the variable payload in bytes in the packets, and<br>
    \<echo\> tells the blaster if it should expect the echo of its packet from the blastee. Values 1 (expect echo) and 0 (no       echo) are valid for this parameter. 

2. After complilation, the receiver (blastee) is invoked by:<br>
  blastee -p \<port\> -c \<echo\><br>
    \<port\> is the port on which the user wants to run blastee, and<br>
    \<echo\> tells the blastee if it should echo back the packet that it received to the blaster. Values 1 (echo back) and 0       (no echo) are valid for this parameter.

3. The packet has 3 types: data, echo and end, which is set up in the header (containing packet type, sequence number and payload length)

4. The blastee will make a summary after receiving the END packet, or after 5 seconds of halting of the packet flow.
