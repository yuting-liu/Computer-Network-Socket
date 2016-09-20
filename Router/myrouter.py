#!/usr/bin/env python3

'''
Basic IPv4 router (static routing) in Python.
'''

import sys
import os
import time
import queue
from switchyard.lib.packet import *
from switchyard.lib.address import *
from switchyard.lib.common import *

class Router(object):
    def __init__(self, net):
        self.net = net
        # other initialization stuff here


    def router_main(self):    
        '''
        Main method for router; we stay in a loop in this method, receiving
        packets until the end of time.
        '''
        # HashMap for ARP
        arp_table = {}
        # Forwarding table for routing
        forwarding_table = []
        # Collect all interfaces' IP address
        ip_addr_pool = []
        for intf in self.net.interfaces():
            ip_addr_pool.append(intf.ipaddr)
        # Read the input and build the forwarding table
        input_file = open("forwarding_table.txt", "r")
        # queue to maintain ARP requests
        arp_queue = queue.Queue(0);
        for line in input_file:
            # Store the entry information (4 attributes)
            item = []
            for word in line.split():
                item.append(word)
            # Add entry into the forwarding table
            forwarding_table.append(item)

        while True:
            gotpkt = True
            try:
                dev,pkt = self.net.recv_packet(timeout=1.0)
            except NoPackets:
                log_debug("No packets available in recv_packet")
                gotpkt = False
            except Shutdown:
                log_debug("Got shutdown signal")
                break

            if gotpkt:
                log_debug("Got a packet: {}".format(str(pkt)))                

                # update queue to remove time-out arp requests
                while not arp_queue.empty() and time.time() - arp_queue.queue[0][1] > 1:
                    current_request = arp_queue.get()
                    if current_request[2] <= 5:
                        # rebroadcast
                        for intf in self.net.interfaces():
                            if intf != dev:
                                self.net.send_packet(intf, current_request[3])
                        ++current_request[2]
                        arp_queue.add(current_request)

                # judge whether it's an ARP request:
                # with 3 addresses filled and IP dst unfilled
                if pkt.has_header(Arp):
                    arp = pkt.get_header(Arp)
                    # arp request
                    if str(arp.senderhwaddr) != "00.00.00.00.00.00" and str(arp.senderprotoaddr) != "0.0.0.0" and str(arp.targetprotoaddr) != "0.0.0.0" and str(arp.targethwaddr) == "00.00.00.00.00.00":
                        # if we have ipdst in the interface, send an arp_reply back
                        for intf in self.net.interfaces():
                            if str(arp.targetprotoaddr) == str(intf.ipaddr):
                                arp_reply = create_ip_arp_reply(arp.senderhwaddr, intf.ethaddr, arp.senderprotoaddr, arp.targetprotoaddr)
                                self.net.send_packet(dev, arp_reply)
                    
                    # arp reply
                    elif str(arp.senderhwaddr) != "00.00.00.00.00.00" and str(arp.senderprotoaddr) != "0.0.0.0" and str(arp.targetprotoaddr) != "0.0.0.0" and str(arp.targethwaddr) != "00.00.00.00.00.00":
                        if str(arp.senderprotoaddr) == str(arp_queue.queue[0][0].dst):
                            ip_header = arp_queue.get()
                            pkt[Ethernet].src = self.net.interface_by_name(dev).ethaddr
                            pkt[Ethernet].dst = arp.senderhwaddr
                            pkt += IPv4()
                            pkt[IPv4].src = ip_header[0].src
                            pkt[IPv4].dst = ip_header[0].dst
                            self.net.send_packet(dev, pkt)
                            arp_table[arp.senderprotoaddr] = arp.senderhwaddr
                    
                # not ARP request
                else:
                    send_interface = []                   
                    max_prefix_len = 0
                    found = False

                    # check if directly reachable via layer 2
                    for intf in net.interfaces():
                        localnet = IPv4Network(str(intf.ipaddr) + '/' + str(intf.netmask))
                        if pkt[IPv4].dst in localnet:
                            send_interface.append(str(intf.ipaddr))
                            send_interface.append(str(intf.netmask))
                            send_interface.append(str(pkt[IPv4].dst))
                            send_interface.append(intf.name)
                            found = True
                    
                    # find the possible next hop for further sending
                    if not found:
                        for intf in forwarding_table:
                            # check next_hop first
                            if str(pkt[IPv4].dst) == intf[2]:
                                send_interface = intf
                                found = True
                                break
                            # find the longest prefix match in forwarding table                          
                            netaddr = IPv4Network(intf[0] + '/' + intf[1])
                            if pkt[IPv4].dst in netaddr and netaddr.prefixlen > max_prefix_len:
                                max_prefix_len = netaddr.prefixlen
                                send_interface = intf
                                found = True;

                    # send the packet to next hop if not in the following situations:
                    # 1. no match in the forwarding table
                    # 2. the packet is directly reachable in layer 2 by the router
                    
                    if found and pkt[IPv4].dst not in ip_addr_pool:
                        my_interface = self.net.interface_by_name(send_interface[3])
                        # send ARP request if not in arp_table
                        if send_interface[2] not in arp_table.keys():
                            # create arp request
                            pkt_arp = create_ip_arp_request(my_interface.ethaddr, my_interface.ipaddr, send_interface[2])
                            # send arp request
                            self.net.send_packet(send_interface[3], pkt_arp)
                                
                            # add entry in arp request queue
                            attribute = []
                            attribute.append(send_interface[2])
                            attribute.append(time.time())
                            attribute.append(1)
                            arp_queue.put(attribute)
                        else:
                            e = Ethernet()
                            e.dst = arp_table[send_interface[2]]
                            e.src = my_interface.ethaddr
                            pkt = e + pkt
                            self.net.send_packet(send_interface[3], pkt)
                            

def switchy_main(net):
    '''
    Main entry point for router.  Just create Router
    object and get it going.
    '''
    r = Router(net)
    r.router_main()
    net.shutdown()
