#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/inotify.h>

#include "libsong.h"

#define EXT_ERR_INIT_INOTIFY 1
#define EXT_ERR_ADD_WATCH 2
#define EXT_ERR_READ_INOTIFY 3
#define EXT_ERR_SOCKET 4
#define EXT_ERR_BIND 5
#define EXT_ERR_LISTEN 6
#define EXT_ERR_ACCEPT 7
#define EXT_ERR_ESTABLISH_CONN 11
#define EXT_ERR_CONSTRUCT_NOTIF 12
#define EXT_ERR_BASE_PATH_NULL 13

#define PORT 5521

int IeventQueue = -1;
int IeventDescriptor = -1;

/* Catch shutdown signals so we can
 * cleanup exit properly */
void sig_shutdown_handler(int signal) {
	int closeStatus;

	printf("Exit signal received!\nclosing inotify descriptors...\n");

	// don't need to check if they're valid because
	// the signal handlers are registered after they're created;
	// we can assume they are set up correctly.
	if (IeventDescriptor != -1) {

		closeStatus = inotify_rm_watch(IeventQueue, IeventDescriptor);
		if (closeStatus == -1) {
			fprintf(stderr, "Error removing file from inotify watch!\n");
		}
	}

	close(IeventQueue);
	exit(EXIT_SUCCESS);
}

int main() {

	char inotifyBuffer[4096];
	uint8_t socketBuffer[SO_PKT_MAXIMUM_SIZE];

	char *programTitle = "rolexhound";
	char *watchPath = NULL;
	char *basePath = NULL;
	char *token = NULL;

	int readLength = 0;

	const struct inotify_event* watchEvent;

	const uint32_t watch_flags = IN_CREATE | IN_DELETE |
		IN_ACCESS | IN_CLOSE_WRITE | IN_MODIFY | IN_MOVE_SELF;

	int socketFd, connectionFd = -1;
	int bytesRead = -1;

	struct sockaddr_in serverAddress, clientAddress;
	socklen_t addressSize = sizeof(struct sockaddr_in);

	struct song_msg *readMsg, *sendMsg;
	struct serialize_result *result;

	if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Error creating socket!\n");
		exit(EXT_ERR_SOCKET);
	}

	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(PORT);

	if (bind(socketFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
		fprintf(stderr, "Error binding to socket!\n");
		exit(EXT_ERR_BIND);
	}

	if (listen(socketFd, 1)) {
		fprintf(stderr, "Error listening!\n");
		exit(EXT_ERR_LISTEN);
	}

	printf("rolexhound is listening on %d...\n", PORT);

	if ((connectionFd = accept(socketFd, (struct sockaddr *)&clientAddress,
		(socklen_t *)&addressSize)) == -1) {

		fprintf(stderr, "Error accepting connection!\n");
		exit(EXT_ERR_ACCEPT);
	}

	readMsg = (struct song_msg *)malloc(sizeof(struct song_msg));
	song_msg_factory(readMsg);

	sendMsg = (struct song_msg *)malloc(sizeof(struct song_msg));
	song_msg_factory(sendMsg);

	result = (struct serialize_result *)malloc(sizeof(struct serialize_result));
	serialize_result_factory(result);

	while (true) {
		bytesRead = read(connectionFd, socketBuffer, sizeof(socketBuffer));

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

			write(connectionFd, socketBuffer, sizeof(socketBuffer));
			continue;
		}

		if (readMsg->action != SO_ACT_WATCH) {
			continue;
		}

		break;
	}

	watchPath = (char *)malloc(sizeof(char)*strlen(readMsg->data[0]) + 1);
	strcpy(watchPath, readMsg->data[0]);

	// DIFF TO VID: I did have to malloc this after all cause strtok is
	// far more destructive than anticipated...
	basePath = (char *)malloc(sizeof(char)*strlen(readMsg->data[0]) + 1);
	strcpy(basePath, readMsg->data[0]);
	token = strtok(basePath, "/");
	while (token != NULL) {
		basePath = token;
		token = strtok(NULL, "/");
	}

	if (basePath == NULL) {
		fprintf(stderr, "Error getting base file path!\n");
		exit(EXT_ERR_BASE_PATH_NULL);
	}

	IeventQueue = inotify_init();

	if (IeventQueue == -1) {
		fprintf(stderr, "Error initialising event queue!\n");
		exit(EXT_ERR_INIT_INOTIFY);
	}

	// register signal handlers, note we
	// have already checked all error states
	signal(SIGABRT, sig_shutdown_handler);
	signal(SIGINT, sig_shutdown_handler);
	signal(SIGTERM, sig_shutdown_handler);

	IeventDescriptor = inotify_add_watch(IeventQueue, watchPath, watch_flags);

	if (IeventDescriptor == -1) {
		fprintf(stderr, "Error adding file to inotify watch!\n");
		exit(EXT_ERR_ADD_WATCH);
	}

	printf("%s is now watching %s - %s.\n", programTitle, watchPath, basePath);

	// daemon main loop
	while (true) {

		// block on inotify event read
		printf("Waiting for ievent...\n");
		readLength = read(IeventQueue, inotifyBuffer, sizeof(inotifyBuffer));

		if (readLength == -1) {
			fprintf(stderr, "Error reading from inotify event!\n");
			exit(EXT_ERR_READ_INOTIFY);
		}

		// one read could yield multiple events; loop through all
		// kinda tricky to get at a glance but basically moves the pointer
		// through the buffer according to the length of the inotify_event struct
		// and the length of the previous inotify event data
		for (char *buffPointer = inotifyBuffer; buffPointer < inotifyBuffer + readLength;
			 buffPointer += sizeof(struct inotify_event) + watchEvent->len) {

			song_msg_factory(sendMsg);
			watchEvent = (const struct inotify_event *) buffPointer;

			if (watchEvent->mask & IN_CREATE) {
				sendMsg->option = SO_NOTIFY_CREATE;
			}

			if (watchEvent->mask & IN_DELETE) {
				sendMsg->option = SO_NOTIFY_DELETE;
			}

			if (watchEvent->mask & IN_ACCESS) {
				sendMsg->option = SO_NOTIFY_ACCESS;
			}

			if (watchEvent->mask & IN_CLOSE_WRITE) {
				sendMsg->option = SO_NOTIFY_CLOSE;
			}

			if (watchEvent->mask & IN_MODIFY) {
				sendMsg->option = SO_NOTIFY_MODIFY;
			}

			if (watchEvent->mask & IN_MOVE_SELF) {
				sendMsg->option = SO_NOTIFY_MOVE;
			}

			if (sendMsg->option == SO_UNSET_UNSET) {
				continue;
			}

			sendMsg->action = SO_ACT_NOTIFY;
			sendMsg->dataLen = SO_DLEN_NOTIFY;

			sendMsg->data = (char **)malloc(sizeof(char *)*SO_DLEN_NOTIFY);
			sendMsg->data[0] = basePath;
			sendMsg->data[1] = watchPath;

			bzero(socketBuffer, SO_PKT_MAXIMUM_SIZE);

			serialize(socketBuffer, sendMsg, result);
			if (result->reply != SO_REPLY_VALID) {
				fprintf(stderr, "Error %x constructing watch notification!\n", result->reply);
				exit(EXT_ERR_CONSTRUCT_NOTIF);
			}

			printf("Sent a notification to smartwatch!\n");
			write(connectionFd, socketBuffer, SO_PKT_MAXIMUM_SIZE);

			free(sendMsg->data);
		}
	}
}
