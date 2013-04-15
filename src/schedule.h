typedef struct __attribute__((packed)){
	uint16_t flags;
	uint16_t start_time;
	uint16_t end_time;
	char name[32];
}event;

typedef enum{
	BEFORE,
	DURING,
	AFTER
}time_type;