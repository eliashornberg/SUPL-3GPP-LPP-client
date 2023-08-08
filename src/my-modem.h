#ifndef MY_MODEM_H
#define MY_MODEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/epoll.h>

#include "actia/actia.h"

void usage(char *app_name);
static bool wait_for_msg(a_ipc_handle *handle, a_ipc_msg *msg, int timeout_ms, int msg_type);
static A_IPC_RESULT send_ipc(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf);
static A_IPC_RESULT request(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, char *user);
static A_IPC_RESULT open_session(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf);
static A_IPC_RESULT release(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, char *user);
static A_IPC_RESULT subscribe(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, bool extended);
static A_IPC_RESULT unsubscribe(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, bool extended);
static A_IPC_RESULT cell_wait_for_status(a_ipc_handle *a_ipc, uint32_t *cellid, uint16_t *tac, char *mcc, char *mnc);
static A_IPC_RESULT cpy_cell_info(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, uint32_t *cellid, uint16_t *tac, char *mcc, char *mnc);
void get_cell_data(uint32_t *cellid, uint16_t *tac, char mcc[4], char mnc[4]);
static A_IPC_RESULT cpy_imsi(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, char *imsi);
void get_imsi(char imsi[16]);
unsigned long get_long_imsi();
int main(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* MY_MODEM_H */
