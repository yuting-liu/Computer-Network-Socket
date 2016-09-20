#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

using namespace std;

#define BUF_SIZE			52000
#define RECV_DATA			0
#define RECV_END			1
#define RECV_ERROR			2

bool first = true;

int recv_pkt(int udp_socket, unsigned short port, int& total_pkt, int& total_byte, clock_t start, bool echo) {
	char recv_buffer[BUF_SIZE];
	struct sockaddr_in recv_info;
	unsigned int socklen = sizeof(recv_info);
	int recv_len = recvfrom(udp_socket, &recv_buffer, sizeof(recv_buffer) - 1, 0, (sockaddr *) &recv_info, &socklen);
	
	if(recv_len >= 9) {
		// receive DATA packet
		if (recv_buffer[0] == 'D') {	
			// Update receiving info
			++total_pkt;
			total_byte += recv_len;		
			// Treat receiving pkt
			char * src_ip_addr = inet_ntoa(recv_info.sin_addr);
			unsigned int seq_no = 0;
			for(int i = 1; i < 5; ++i) {
				seq_no = seq_no << 8;
				seq_no = seq_no | (recv_buffer[i] & 0xff);
			} 
			string packet = string(recv_buffer, recv_len);
			string payload = packet.substr(9, 4);
			
			// Mark the first packet as start time
			if (first) {
				printf("Blastee received packet\n  Source IP address: %s\n  Received port: %d\n  Packet's size (byte): %d\n  Sequence number: %u\n  Received time (ms): 0\n  First 4 bytes of payload: %s\n", 
						src_ip_addr, port, recv_len, seq_no, payload.c_str());
				first = false;
			}
			else {
				printf("Blastee received packet\n  Source IP address: %s\n  Received port: %d\n  Packet's size (byte): %d\n  Sequence number: %u\n  Received time (ms): %d\n  First 4 bytes of payload: %s\n", 
						src_ip_addr, port, recv_len, seq_no, (int) ((clock() - start) * 1000 / CLOCKS_PER_SEC), payload.c_str());
			}
			// Echo if needs to
			if (echo) {
				packet[0] = 'C';
				sendto(udp_socket, packet.c_str(), recv_len, 0, (struct sockaddr *) &recv_info, sizeof(recv_info));
			}
			
			return RECV_DATA;
		}
		// receive END packet
		else if(recv_buffer[0] == 'E') {
			return RECV_END;
		}
	}
	
	return RECV_ERROR;
}

int main(int argc, char* argv[]) {
	unsigned short port = 0;
	bool echo;
	int opt = 0;
	unsigned int port_int;
	while((opt = getopt(argc, argv, "p:c:")) != -1) {
		switch (opt) {
			case 'p':
				port_int = (unsigned short)atoi(optarg);
				break;
			case 'c':
				echo = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s [-p port] [-c echo] name\n",
						argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	
	if(port_int <= 1024 || port_int >= 65536) {
		fprintf(stderr, "Illegal port number\n");
	}
	port = (unsigned short) port_int;
	// create UDP socket
	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_socket == -1) {
		fprintf(stderr, "Failure encountered: blastee creates socket\n");
		exit(EXIT_FAILURE);
	}
	// Set as nonblocking socket
	fcntl(udp_socket, F_SETFL, O_NONBLOCK);
	
	// Create local socket	
	sockaddr_in local_info;
	local_info.sin_family = AF_INET;
	local_info.sin_port = htons(port);
	char hostname[1024];
	gethostname(hostname, 1023);
	struct hostent* my_hostent = gethostbyname(hostname);
	memcpy(&local_info.sin_addr, my_hostent -> h_addr_list[0], my_hostent -> h_length);
	
    if (bind(udp_socket, (struct sockaddr *) &local_info, sizeof(struct sockaddr_un)) == -1) {
		fprintf(stderr, "Failure encountered: binding\n");
		exit(EXIT_FAILURE);
	}
	
	// keep listening
	clock_t pkt_start = clock();
	clock_t start = clock();
	int total_pkt = 0, total_byte = 0;
	while(true) {
		int status = recv_pkt(udp_socket, port, total_pkt, total_byte, start, echo);
		if(status == RECV_DATA) {
			pkt_start = clock();
			if (total_pkt == 1) {
				start = pkt_start;
			}
		}
		else if(clock() - pkt_start >= 5 * CLOCKS_PER_SEC || status == RECV_END) {
			break;
		}
	}
	
	// Summarize
	clock_t end = clock();
	printf("Summary:\n  Total packets: %d\n  total bytes: %d\n  average packets/second: %f\n  average bytes/second: %f\n  duration (ms): %d\n", 
			total_pkt, total_byte, total_pkt * CLOCKS_PER_SEC / (float) (end - start), total_byte * CLOCKS_PER_SEC / (float) (end - start), (int) ((end - start) * 1000 / CLOCKS_PER_SEC));
}
