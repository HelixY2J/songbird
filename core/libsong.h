#include <stdint.h>

typedef uint8_t SO_act;
typedef uint8_t SO_option;

#define SO_ACT_WATCH					0x00
#define SO_ACT_QUIT					0x01
#define SO_ACT_NOTIFY					0x02
#define SO_ACT_REPLY					0x03
#define SO_ACT_STATUS					0x04
#define SO_ACT_UNSET					0xFF

#define SO_WATCH_ADD					0x00
#define SO_WATCH_REM					0x01

#define SO_QUIT_USER					0x00
#define SO_QUIT_ERROR					0x01

#define SO_NOTIFY_CREATE				0x00
#define SO_NOTIFY_DELETE				0x01
#define SO_NOTIFY_ACCESS				0x02
#define SO_NOTIFY_CLOSE				0x03
#define SO_NOTIFY_MODIFY				0x04
#define SO_NOTIFY_MOVE					0x05

#define SO_REPLY_VALID					0x00
#define SO_REPLY_BAD_SIZE	 			0x01
#define SO_REPLY_BAD_ACTION			0x02
#define SO_REPLY_BAD_OPTION			0x03
#define SO_REPLY_BAD_PATH 				0x04
#define SO_REPLY_INVALID_DATA			0x05
#define SO_REPLY_UNSET					0xFF

#define SO_STATUS_SUCCESS				0x00
#define SO_STATUS_ERR_INIT_INOTIFY 	0x01
#define SO_STATUS_ERR_ADD_WATCH 		0x02
#define SO_STATUS_ERR_READ_INOTIFY 	0x03

#define SO_UNSET_UNSET					0xFF

#define SO_PKT_MINIMUM_SIZE			3
#define SO_PKT_MAXIMUM_SIZE			255

#define SO_DLEN_WATCH					1
#define SO_DLEN_QUIT					0
#define SO_DLEN_NOTIFY					2
#define SO_DLEN_REPLY					0
#define SO_DLEN_STATUS					0
#define SO_DLEN_UNSET					0

struct song_msg {
	SO_act action;
	SO_option option;
	uint8_t size;

	char **data;
	int dataLen;
};

struct serialize_result {
	int size;
	SO_option reply;
};

void print_packet(struct song_msg *msg);

void song_msg_factory(struct song_msg *message);
void song_msg_reset(struct song_msg *message);
void serialize_result_factory(struct serialize_result *result);

void deserialize(uint8_t buffer[SO_PKT_MAXIMUM_SIZE],
	struct song_msg *msg, struct serialize_result *result);
void serialize(uint8_t buffer[SO_PKT_MAXIMUM_SIZE],
	struct song_msg *msg, struct serialize_result *result);