#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <libnotify/notify.h>

#include "libsong.h"

#define EXT_ERR_TOO_FEW_ARGS 1
#define EXT_ERR_BASE_PATH_NULL 2
#define EXT_ERR_INIT_LIBNOTIFY 3
#define EXT_ERR_ESTABLISH_CONN 4
#define EXT_ERR_INIT_CONSTRUCTION 5

#define PORT 5521

void sig_shutdown_handler(int signal) {

	printf("Exit signal received!\nuninitialising with libnotify...\n");

	notify_uninit();
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {

	char *programTitle = "smartwatch";
	char *notificationMessage = NULL;
	bool notifyInitStatus;

	uint8_t socketBuffer[SO_PKT_MAXIMUM_SIZE];

	NotifyNotification *notifyHandle;

	int socketFd = -1;
	int bytesRead = -1;
	struct sockaddr_in serverAddress;

	struct song_msg *readMsg, *sendMsg;
	struct serialize_result *result;

	if (argc < 3) {
		fprintf(stderr, "USAGE: %s HOST PATH\n", programTitle);
		return EXT_ERR_TOO_FEW_ARGS;
	}

	notifyInitStatus = notify_init (programTitle);

	if (!notifyInitStatus) {
		fprintf(stderr, "Error initialising with libnotify!\n");
		exit(EXT_ERR_INIT_LIBNOTIFY);
	}

	signal(SIGABRT, sig_shutdown_handler);
	signal(SIGINT, sig_shutdown_handler);
	signal(SIGTERM, sig_shutdown_handler);

	socketFd = socket(AF_INET, SOCK_STREAM, 0);

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
	serverAddress.sin_port = htons(PORT);

	if (connect(socketFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
		fprintf(stderr, "Error connecting to rolexhound with params %s %d!\n", argv[1], PORT);
		exit(EXT_ERR_ESTABLISH_CONN);
	}

	readMsg = (struct song_msg *)malloc(sizeof(struct song_msg));
	song_msg_factory(readMsg);

	sendMsg = (struct song_msg *)malloc(sizeof(struct song_msg));
	song_msg_factory(sendMsg);

	result = (struct serialize_result *)malloc(sizeof(struct serialize_result));
	serialize_result_factory(result);

	sendMsg->action = SO_ACT_WATCH;
	sendMsg->option = SO_WATCH_ADD;

	sendMsg->dataLen = 1;
	sendMsg->data = (char **)malloc(sizeof(char *)*1);
	sendMsg->data[0] = argv[2];

	serialize(socketBuffer, sendMsg, result);

	if (result->reply != SO_REPLY_VALID) {
		fprintf(stderr, "Error %x constructing init packet!\n", result->reply);
		exit(EXT_ERR_INIT_CONSTRUCTION);
	}

	write(socketFd, socketBuffer, sizeof(socketBuffer));

	free(sendMsg->data);
	song_msg_factory(sendMsg);

	while (true) {

		bytesRead = read(socketFd, socketBuffer, sizeof(socketBuffer));

		if (bytesRead == 0) {
			fprintf(stderr, "Error reading from socket!\n");
			exit(EXT_ERR_ESTABLISH_CONN);
		}

		deserialize(socketBuffer, readMsg, result);
		if (result->reply != SO_REPLY_VALID) {
			fprintf(stderr, "Received %x from client!\n", result->reply);

			sendMsg->action = SO_ACT_REPLY;
			sendMsg->option = result->reply;
			sendMsg->size = 0;

			bzero(socketBuffer, SO_PKT_MAXIMUM_SIZE);

			serialize(socketBuffer, sendMsg, result);

			write(socketFd, socketBuffer, sizeof(socketBuffer));

			song_msg_factory(sendMsg);
			continue;
		}

		if (readMsg->action != SO_ACT_NOTIFY) {
			continue;
		}

		notificationMessage = NULL;

		switch (readMsg->option) {
			case SO_NOTIFY_CREATE:
				notificationMessage = "File created.\n";
				break;
			case SO_NOTIFY_DELETE:
				notificationMessage = "File deleted.\n";
				break;
			case SO_NOTIFY_ACCESS:
				notificationMessage = "File accessed.\n";
				break;
			case SO_NOTIFY_CLOSE:
				notificationMessage = "File written and closed.\n";
				break;
			case SO_NOTIFY_MODIFY:
				notificationMessage = "File modified.\n";
				break;
			case SO_NOTIFY_MOVE:
				notificationMessage = "File moved.\n";
				break;
		}

		notifyHandle = notify_notification_new(readMsg->data[0],
			notificationMessage, "dialog-information");

		if (notifyHandle == NULL) {
			fprintf(stderr, "Got a null notify handle!\n");
			continue;
		}
		notify_notification_set_urgency(notifyHandle, NOTIFY_URGENCY_CRITICAL);
		notify_notification_show(notifyHandle, NULL);
	}

	return 0;
}
