#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libsong.h"

const uint8_t OptionRanges[5] = {
	SO_WATCH_REM,
	SO_QUIT_ERROR,
	SO_NOTIFY_MOVE,
	SO_REPLY_INVALID_DATA,
	SO_STATUS_ERR_READ_INOTIFY
};

const int ActionSizes[5] = {
	SO_DLEN_WATCH,
	SO_DLEN_QUIT,
	SO_DLEN_NOTIFY,
	SO_DLEN_REPLY,
	SO_DLEN_STATUS
};

int LastActionIndex = sizeof(ActionSizes) / sizeof(ActionSizes[0]) - 1;

void print_packet(struct song_msg *msg) {
	printf("--PACKET--\n");

	printf("cmd: %x\ntype: %x\nsize: %x\n",
		msg->action, msg->option, msg->size);

	for (int i = 0; i < msg->dataLen; i++) {
		printf("data %d: %s\n", i, msg->data[i]);
	}

}

void song_msg_factory(struct song_msg *message) {

	message->action = SO_ACT_UNSET;
	message->option = SO_UNSET_UNSET;
	message->size = 0;

	message->data = NULL;
	message->dataLen = 0;
}

void song_msg_reset(struct song_msg *message) {

	if (message->data == NULL) {

		song_msg_factory(message);
		return;
	}

	for (int i = 0; i < message->dataLen; i++) {

		if (message->data[i] != NULL) {
			free(message->data[i]);
		}
	}

	free(message->data);
	song_msg_factory(message);
}

void serialize_result_factory(struct serialize_result *result) {
	result->size = -1;
	result->reply = SO_REPLY_UNSET;
}

void serialize(uint8_t buffer[SO_PKT_MAXIMUM_SIZE], struct song_msg *msg,
		struct serialize_result *result) {

	int validDataLength = -1;
	uint8_t dataSize = 0;
	uint8_t runningDataSize = 0;

	serialize_result_factory(result);

	if (msg->action <= LastActionIndex) {

		buffer[0] = msg->action;
		validDataLength = ActionSizes[msg->action];
	} else {
		result->reply = SO_REPLY_BAD_ACTION;
		return;
	}

	if (validDataLength != msg->dataLen) {
		result->reply = SO_REPLY_BAD_SIZE;
		return;
	}

	if (msg->option > OptionRanges[msg->action]) {
		result->reply = SO_REPLY_BAD_OPTION;
		return;
	}

	buffer[1] = msg->option;

	if (validDataLength != 0 && msg->data == NULL) {
		result->reply = SO_REPLY_INVALID_DATA;
		return;
	}

	for (int i = 0; i < msg->dataLen; i++) {

		if (msg->data[i] == NULL) {
			result->reply = SO_REPLY_INVALID_DATA;
			return;
		}

		for (int j = 0; j < SO_PKT_MAXIMUM_SIZE - SO_PKT_MINIMUM_SIZE - dataSize; j++) {

			runningDataSize += 1;
			buffer[SO_PKT_MINIMUM_SIZE + dataSize + j] = msg->data[i][j];

			if (msg->data[i][j] == '\0') {
				break;
			}
		}

		dataSize += runningDataSize;
		runningDataSize = 0;
	}

	buffer[2] = dataSize;
	result->reply = SO_PKT_MINIMUM_SIZE + dataSize;
	result->reply = SO_REPLY_VALID;
	return;
}

void deserialize(uint8_t buffer[SO_PKT_MAXIMUM_SIZE],
		struct song_msg *msg, struct serialize_result *result) {

	int validDataLength = -1;
	int dataOffset = SO_PKT_MINIMUM_SIZE;
	int *dataSizes = NULL;
	int dataIndex = 0;
	int dataSize = 0;

	serialize_result_factory(result);
	song_msg_reset(msg);

	if (buffer[0] <= LastActionIndex) {

		msg->action = buffer[0];
		validDataLength = ActionSizes[msg->action];
	} else {

		result->reply = SO_REPLY_BAD_ACTION;
		return;
	}

	if (buffer[1] > OptionRanges[msg->action]) {

		result->reply = SO_REPLY_BAD_OPTION;
		return;
	}

	msg->option = buffer[1];

	if (validDataLength == 0) {
		if (buffer[2] != 0) {

			result->reply = SO_REPLY_BAD_SIZE;
		} else {

			result->reply = SO_REPLY_VALID;
		}

		return;
	}

	msg->size = buffer[2];
	dataSizes = (int *)malloc(sizeof(int)*validDataLength);

	for (int i = SO_PKT_MINIMUM_SIZE; i < SO_PKT_MAXIMUM_SIZE; i++) {
		if (i > msg->size + SO_PKT_MINIMUM_SIZE) {

			free(dataSizes);
			result->reply = SO_REPLY_BAD_SIZE;
			return;
		}

		if (dataIndex == validDataLength) {
			if (dataSize == 1) {

				free(dataSizes);
				result->reply = SO_REPLY_BAD_SIZE;
				return;
			}

			break;
		}

		if (buffer[i] == '\0') {

			dataSizes[dataIndex] = ++dataSize;
			dataIndex++;
			dataSize = 0;
			continue;
		}

		// DIFF TO VID: Added space because it is less than ! but still valid :P
		if (buffer[i] != ' ' && (buffer[i] < '!' || buffer[i] > '~')) {

			free(dataSizes);
			result->reply = SO_REPLY_INVALID_DATA;
			return;
		}

		dataSize++;
	}

	msg->data = (char **)malloc(sizeof(char *)*validDataLength);
	msg->dataLen = validDataLength;

	for (int i = 0; i < validDataLength; i++) {

		msg->data[i] = (char *)malloc(sizeof(char)*dataSizes[i]);

		for (int j = 0; j < dataSizes[i]; j++) {
			msg->data[i][j] = buffer[j + dataOffset];
		}

		dataOffset += dataSizes[i];
	}

	result->reply = SO_REPLY_VALID;
	result->size = SO_PKT_MINIMUM_SIZE + msg->size;
	free(dataSizes);
	return;
}