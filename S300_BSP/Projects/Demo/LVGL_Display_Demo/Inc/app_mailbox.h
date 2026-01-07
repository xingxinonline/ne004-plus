#ifndef _APP_MAILBOX_H_
#define _APP_MAILBOX_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_mailbox_init(void);
void app_mailbox_set_time_callback(uint32_t (*get_ms)(void));
void app_mailbox_poll(void);
bool app_mailbox_is_face_present(void);

#ifdef __cplusplus
}
#endif

#endif
