## CS640 Lab 1
## Semester: fall 2015
## Team member: Shixuan Fan
##              Yuting Liu
## Instructer: Paul Barford
#!/usr/bin/env python3

'''
Ethernet learning switch in Python: HW3.

'''
from switchyard.lib.address import *
from switchyard.lib.packet import *
from switchyard.lib.common import *
import time

def switchy_main(net):
    my_interfaces = net.interfaces() 
    mymacs = [intf.ethaddr for intf in my_interfaces]
	# the hashtable to record the pair of macaddr and pair<port #, time>
    forwarding_table = {}

    while True:
        try:
            dev,packet = net.recv_packet()
        except NoPackets:
            continue
        except Shutdown:
            return

        log_debug ("In {} received packet {} on {}".format(net.name, packet, dev))
		
        # learning from incoming packet
        # pair[] stores two elements:
        # i. dev: the port number for source address
        # ii. time: the time when receiving the packet
        pair = []
        pair.append(dev)
        pair.append(time.time())
        # put the source address and corresponding pair into HashMap
        forwarding_table[packet[0].src] = pair

        # traverse the table to check:
        # delete all entries in forwarding_table that is out-dated
        # delete_table_item[]: records all indices of out-dated elements
        delete_table_item = []
        for item in forwarding_table.keys():
            # set TTL = 30s
            if time.time() - forwarding_table[item][1] > 30.00:
                delete_table_item.append(item)
        for del_item in delete_table_item:
            del forwarding_table[del_item]
        
        # if packet is destined for this learning switch
        # do nothing
        if packet[0].dst in mymacs:
            log_debug ("Packet intended for me")
        # if forwarding_table contains the destination addr
        # forward the packet to the corresponding port
        elif packet[0].dst in forwarding_table.keys():
            net.send_packet(forwarding_table[packet[0].dst][0], packet)
            # update learnt destination entry's time
            forwarding_table[packet[0].dst][1] = time.time()
        # if forwarding_table doesn't contain the destination addr
        # broadcast to all ports except input port
        else: 
            for intf in my_interfaces:
                if dev != intf.name:
                    log_debug ("Flooding packet {} to {}".format(packet, intf.name))
                    net.send_packet(intf.name, packet)
    net.shutdown()
