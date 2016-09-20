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
#include <sstream>
#include <fcntl.h>

using namespace std;

// error macros
#define ARGUMENT_COUNT_ERROR    1
#define PORT_NUMBER_ERROR       2
#define LENGTH_ERROR            3

// size macros
#define BUF_SIZE				52000
#define HEADER_SIZE				9

int send_packet(unsigned int &seq_no, unsigned int length, int udp_socket, char* buf, struct sockaddr_in dst_info) {	
	// get string value of seq # (in ASCII)
	unsigned int t_seq_no = seq_no;
	char seq_no_char[4];
	for (int i = 3; i >= 0; --i) {
		seq_no_char[i] = t_seq_no & 0xff;
		t_seq_no = t_seq_no >> 8;
	}
	string seq_no_str = string(seq_no_char, 4);
	// get string values of length (in ASCII)
	unsigned int t_len = length;
	char length_char[4];
	for (int i = 3; i >= 0; --i) {
		length_char[i] = t_len & 0xff;
		t_len = t_len >> 8;
	}
	string length_str = string(length_char, 4);
    // send DATA packets
	string header = "D" + seq_no_str + length_str;
	string data = string(buf, length);
	string payload = data.substr(0, 4);
	string datagram = header + payload;
	
	sendto(udp_socket, datagram.c_str(), length + 9, 0, (struct sockaddr *) &dst_info, sizeof(dst_info));
	
	cout<<"Blaster sent packet:"<< endl 
	<< "  Sequence number: " << seq_no << endl
	<< "  First 4 bytes of payload: " << payload << endl;
	
	// update seq #
	seq_no += length;
	return 0;
}

void listen (clock_t start, int interval, bool echo, int udp_socket) {
	// Listen for echo or idle for a given interval
	while (clock() - start <= interval) {
		if (echo) {
			// receive packet
			char recv_buffer[BUF_SIZE];
			struct sockaddr_in recv_info;
			unsigned int socklen = sizeof(recv_info);
			int recv_len = recvfrom(udp_socket, &recv_buffer, sizeof(recv_buffer) - 1, 0, (sockaddr *) &recv_info, &socklen);
			
			// print the info once the pkt is received
			if(recv_len >= 9 && recv_buffer[0] == 'C') {
				// Decode seq #
				unsigned int seq_no = 0;
				for(int i = 1; i < 5; ++i) {
					seq_no = seq_no << 8;
					seq_no = seq_no | (recv_buffer[i] & 0xff);
				} 
				string packet = string(recv_buffer, recv_len);
				string payload = packet.substr(9, 4);
				printf("Echo:\n  Sequence number: %u\n  First 4 bytes of payload: %s\n", seq_no, payload.c_str());
			}
		}
	}
}

int main(int argc, char* argv[]) {
    // get all parameters
    char* hostname = NULL;
    unsigned short port = 0;
    int rate = 0, num = 0;
    char* seq_no_str = NULL, * length_str = NULL;
    bool echo = false;
    int opt = 0;
    unsigned int seq_no = 0, length = 0, port_int = 0;
	
    bool s_flag = false, p_flag = false, r_flag = false, n_flag = false, q_flag = false, l_flag = false, c_flag = false;
    while((opt = getopt(argc, argv, "s:p:r:n:q:l:c:")) != -1) {
		switch (opt) {
			case 's':
				s_flag = true;
				hostname = new char[strlen(optarg)];
				strcpy(hostname, optarg);
				break;
			case 'p':
				p_flag = true;
				port_int = (unsigned int)atoi(optarg);
				break;
			case 'r':
				r_flag = true;
				rate = atoi(optarg);
				break;
			case 'n':
				n_flag = true;
				num = atoi(optarg);
				break;
			case 'q':
				q_flag = true;
				seq_no_str = optarg;
				seq_no = (unsigned int) atoi(seq_no_str);
				break;
			case 'l':
				l_flag = true;
				length = (unsigned int) atoi(optarg);
				length_str = new char[strlen(optarg)];
				strcpy(length_str, optarg);
				break;
			case 'c':
				c_flag = true;
				echo = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: blaster [-s hostname] [-p port] [-r rate] [-n num] [-q seq_no] [-l length] [-c echo]\n");
				exit(EXIT_FAILURE);
		}
	}
    
    if (!(s_flag && p_flag && r_flag && n_flag && q_flag && l_flag && c_flag)) {
		fprintf(stderr, "Uninitialized attributes detected\n");
		exit(EXIT_FAILURE);
	}
    
	if (port_int <= 1024 || port_int >= 65536) {
		fprintf(stderr, "Illegal port number\n");
		exit(EXIT_FAILURE);
	}
	
	port = (unsigned short) port_int;
	
	if (rate <= 0) {
		fprintf(stderr, "Illegal rate\n");
		exit(EXIT_FAILURE);
	}
	if (num <=0) {
		fprintf(stderr, "Illegal packet number\n");
		exit(EXIT_FAILURE);
	}
	if (length < 0) {
		fprintf(stderr, "Illegal payload length\n");
		exit(EXIT_FAILURE);
	}
	
	// create UDP socket
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
		fprintf(stderr, "Failure encountered: blaster creates socket\n");
		exit(EXIT_FAILURE);
	}	
	// Set as nonblocking socket
	fcntl(udp_socket, F_SETFL, O_NONBLOCK);
	
	sockaddr_in local_info;
	local_info.sin_family = AF_INET;
	local_info.sin_port = htons(0);
	char my_hostname[1024];
	gethostname(my_hostname, 1023);
	struct hostent* my_hostent = gethostbyname(my_hostname);
	memcpy(&local_info.sin_addr, my_hostent -> h_addr_list[0], my_hostent -> h_length);
	
	if (my_hostent == NULL) {
		fprintf(stderr, "Failure encountered: getting local host by name\n");
		exit(EXIT_FAILURE);
	}
	
    if (bind(udp_socket, (struct sockaddr *) &local_info, sizeof(struct sockaddr_in)) == -1) {
		fprintf(stderr, "Failure encountered: binding\n");
		exit(EXIT_FAILURE);
	}
	
    // get the host name from command line
    struct hostent * remote_hostent = gethostbyname(hostname);
    if (remote_hostent == NULL) {
		fprintf(stderr, "Failure encountered: getting remote host by name\n");
		exit(EXIT_FAILURE);
	}
	
	// Create remote socket
	sockaddr_in dst_info;
	dst_info.sin_family = AF_INET;
	memcpy(&dst_info.sin_addr, remote_hostent -> h_addr_list[0], remote_hostent -> h_length);
	dst_info.sin_port = htons(port);
	
    char buf[BUF_SIZE];
    
    // read in the payload from file
    FILE * fin = fopen("data.in", "r");
    fscanf(fin, "%s", buf);
    fclose(fin);
	
	// Prepare to start sending
	int pkt_sent_num = 0;
	int interval = CLOCKS_PER_SEC / rate;
	clock_t start = clock();
	
	// Send num pkts
	while (pkt_sent_num < num) {
		// Send pkt when the time interval is reached
		send_packet(seq_no, length, udp_socket, buf, dst_info);
		++pkt_sent_num;
		start = clock();
		// wait and receive echo (if needed)
		listen(start, interval, echo, udp_socket);
	}
	
	// send END packet
	char* type = new char[1];
	type[0] = 'E';
	char* header = strcat(strcat(type, seq_no_str), "0");
	sendto(udp_socket, header, 9, 0, (struct sockaddr *) &dst_info, sizeof(dst_info));
	
    exit(EXIT_SUCCESS);
}
