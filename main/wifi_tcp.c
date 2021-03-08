#include "wifi_tcp.h"

#define HOST_IP_ADDR_1 "192.168.43.1"
#define PORT_1         9100
int tcpsock_1 = 0;
struct sockaddr_in dest_addr_1;
TaskHandle_t xtcpRecvTask_1 = NULL;

void tcp_recv_data_1(void *pvParameters);
xTaskHandle xHandle_1;
void init_socket_1() {
	tcpsock_1 = socket(AF_INET, SOCK_STREAM, 0);
	printf("sock:%d\n", tcpsock_1);
	if (tcpsock_1 < 0) {
		printf("Unable to create socket 1: errno %d", errno);
		return;
	}
	printf("Socket created, sending to %s:%d\r\n", HOST_IP_ADDR_1, PORT_1);
	dest_addr_1.sin_addr.s_addr = inet_addr(HOST_IP_ADDR_1);
	dest_addr_1.sin_family = AF_INET;
	dest_addr_1.sin_port = htons(PORT_1);
}

void connect_socket_1(int reConflag) {
	init_socket_1();
	int err = connect(tcpsock_1, (struct sockaddr * ) &dest_addr_1,
			sizeof(dest_addr_1));
	if (err != 0) {
		printf("Socket unable to connect: errno %d\r\n", errno);
		close(tcpsock_1);
		tcpsock_1 = -1;
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		connect_socket_1(reConflag);
	} else {
		printf("Successfully connected\n");
		if (!reConflag) {
			xTaskCreate(tcp_recv_data_1, "tcp_recv_data_1",
					OPENSSL_DEMO_THREAD_STACK_WORDS, NULL,
					OPENSSL_DEMO_THREAD_PRORIOTY, &xHandle_1);
		}

	}


}
//周期定时器的回调函数
void tcp_reconnect_1(int reConflag) {
	printf("-------tcp_reconnect-----");
	connect_socket_1(reConflag);

}

void tcp_recv_data_1(void *pvParameters) {
	uint8_t rx_buffer[1024] = { 0 };
	while (1) {
		int len = recv(tcpsock_1, rx_buffer, sizeof(rx_buffer) - 1, 0);
		if (len > 0) {
			rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
			printf("Received %d bytes from %s:\n", len, rx_buffer);
//			int err = send(tcpsock, rx_buffer, len, 0);
//			if (err < 0) {
//				printf("Error occurred during sending: errno %d", errno);
//				return;
//			}
//			printf("Message sent");
		} else {
			close(tcpsock_1);
			tcpsock_1 = -1;
			tcp_reconnect_1(1);

		}
	}

}


#define HOST_IP_ADDR_2 "192.168.43.102"
#define PORT_2         9100
int tcpsock_2 = 0;
struct sockaddr_in dest_addr_2;
TaskHandle_t xtcpRecvTask_2 = NULL;

void tcp_recv_data_2(void *pvParameters);
xTaskHandle xHandle_2;
void init_socket_2() {
	tcpsock_2 = socket(AF_INET, SOCK_STREAM, 0);
	printf("sock:%d\n", tcpsock_2);
	if (tcpsock_2 < 0) {
		printf("Unable to create socket 2: errno %d", errno);
		return;
	}
	printf("Socket created, sending to %s:%d\r\n", HOST_IP_ADDR_2, PORT_2);
	dest_addr_2.sin_addr.s_addr = inet_addr(HOST_IP_ADDR_2);
	dest_addr_2.sin_family = AF_INET;
	dest_addr_2.sin_port = htons(PORT_2);
}

void connect_socket_2(int reConflag) {
	init_socket_2();
	int err = connect(tcpsock_2, (struct sockaddr * ) &dest_addr_2,
			sizeof(dest_addr_2));
	if (err != 0) {
		printf("Socket unable to connect: errno %d\r\n", errno);
		close(tcpsock_2);
		tcpsock_2 = -1;
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		connect_socket_2(reConflag);
	} else {
		printf("Successfully connected\n");
		if (!reConflag) {
			xTaskCreate(tcp_recv_data_2, "tcp_recv_data_2",
					OPENSSL_DEMO_THREAD_STACK_WORDS, NULL,
					OPENSSL_DEMO_THREAD_PRORIOTY, &xHandle_2);
		}

	}


}
//周期定时器的回调函数
void tcp_reconnect_2(int reConflag) {
	printf("-------tcp_reconnect-----");
	connect_socket_2(reConflag);

}

void tcp_recv_data_2(void *pvParameters) {
	uint8_t rx_buffer[1024] = { 0 };
	while (1) {
		int len = recv(tcpsock_2, rx_buffer, sizeof(rx_buffer) - 1, 0);
		if (len > 0) {
			rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
			printf("Received %d bytes from %s:\n", len, rx_buffer);
//			int err = send(tcpsock, rx_buffer, len, 0);
//			if (err < 0) {
//				printf("Error occurred during sending: errno %d", errno);
//				return;
//			}
//			printf("Message sent");
		} else {
			close(tcpsock_2);
			tcpsock_2 = -1;
			tcp_reconnect_2(1);

		}
	}

}


void tcp_init_client(void) {
	connect_socket_1(0);
//	connect_socket_2(0);
}




