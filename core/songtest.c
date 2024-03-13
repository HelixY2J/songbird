#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "libsong.h"

int main() {
	uint8_t buff[SO_PKT_MAXIMUM_SIZE] = {
		SO_ACT_NOTIFY,
		SO_WATCH_REM,
		0x0C,
		'h',
		'e',
		'l',
		'l',
		'o',
		'\0',
		'h',
		'e',
		'l',
		'l',
		'o',
		'\0',
	};

	char *lol = "heeeelooo, I am elMo";
	char *jej = "how u doin";

	char *testData[2] = {
		lol,
		jej
	};

	uint8_t cereal[1024] = {0};

	struct serialize_result* result = (struct serialize_result*)malloc(sizeof(struct serialize_result));
	serialize_result_factory(result);

	struct song_msg *message = (struct song_msg*)malloc(sizeof(struct song_msg));
	song_msg_factory(message);

	SO_option replyType = SO_REPLY_UNSET;

	deserialize(buff, message, result);

	if (result->reply != SO_REPLY_VALID) {
		fprintf(stderr, "GOT %d\n", result->reply);
		
	}

	print_packet(message);
	serialize(cereal, message, result);

	for (int i = 0; i < 3 + cereal[3]; i++) {
		printf("%x ", cereal[i]);
	}
	printf("\n");

	message->data = testData;
	message->dataLen = 2;

	serialize(cereal, message, result);
	if (result->reply != SO_REPLY_VALID) {
		fprintf(stderr, "GOT %d\n", result->reply);
		return 1;
	}

	for (int i = 0; i < 3 + cereal[3]; i++) {
		printf("%x ", cereal[i]);
	}
	printf("\n");

	return 0;
}