// Copyright (C) 2020 Actia Nordic AB. All rights reserved.

#define _GNU_SOURCE
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

#define IPC_ID (A_IPC_GUEST_MIN_ID + 5)
#define MAXIMAL_EXECUTION_TIME_S 60
#define USER_MAX_LENGTH 15

#define IPC_OUTBOX_SIZE 10
#define IPC_INBOX_SIZE 15
A_IPC_STATIC_BUFFERS(ipc_buffers, IPC_INBOX_SIZE, IPC_OUTBOX_SIZE);


void usage(char *app_name) {
    printf("Syntax: %s <request | release | subscribe | unsubscribe | "
           "subscribe_wait_for_status_unsubscribe | ext_subscribe | ext_unsubscribe | "
           "ext_subscribe_wait_for_status_unsubscribe | status | status_poll | info | "
           "temperature | imsi | cell>\n",
           app_name);
}

static bool wait_for_msg(a_ipc_handle *handle,
                         a_ipc_msg *msg,
                         int timeout_ms,
                         int msg_type)
{
    int epfd = epoll_create(1);
    // check return
    struct epoll_event events[1];

    struct epoll_event event = { 0 };
    event.events = EPOLLIN;
    event.data.fd = a_ipc_get_fd(handle);
    epoll_ctl(epfd, EPOLL_CTL_ADD, a_ipc_get_fd(handle), &event);
    int rc = A_IPC_RET_ERROR;
    while (true) {
        int event_count = epoll_wait(epfd, events, a_membersof(events), timeout_ms);
        if (event_count > 0) {
            rc = a_ipc_recv(handle, NULL, msg);
            if (rc != A_IPC_RET_NO_MSG && msg->info.type == msg_type) {
                return true;
            } else {
                printf("a_ipc_recv returned %s\n", a_ipc_result_str(rc));
            }
        } else if (event_count == 0) {
            printf("Timeout waiting for cellular IPC message.\n");
            return false;
        } else {
            printf("Some serious error waiting for cellular IPC message: %s.\n", strerror(errno));
            return false;
        }
    }
    return true;
}


static A_IPC_RESULT send_ipc(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf)
{
    A_IPC_RESULT rc = a_ipc_send(a_ipc, A_SERVICE_ID_CELLULAR, msg_buf);
    if (rc != A_IPC_RET_OK) {
        printf("Failed to send request to cellular service: %s\n",
               a_ipc_result_str(rc));
    }
    return rc;
}

static A_IPC_RESULT request(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, char *user)
{
    A_IPC_RESULT rc = a_ipc_init_cellular_data_request_req(msg_buf,
                                                           USER_MAX_LENGTH);

    if (rc != A_IPC_RET_OK) {
        printf("Initialize cellular data request message failed: %s\n",
               a_ipc_result_str(rc));
        return rc;
    }
    msg_buf->cellular_data_request_req.user.value = user;

    rc = a_ipc_send(a_ipc,
                    A_SERVICE_ID_CELLULAR,
                    msg_buf);
    if (rc != A_IPC_RET_OK) {
        printf("IPC sending cellular data request message failed: %s\n",
            a_ipc_result_str(rc));
        return rc;
    }

    if (wait_for_msg(a_ipc,
                      msg_buf,
                     5000,
                     A_IPC_MSG_CELLULAR_DATA_REQUEST_RSP)) {
        printf("Cellular data request response result: %d\n",
               msg_buf->cellular_data_request_rsp.result);
        return A_IPC_RET_OK;
    } else {
        printf("Unsuccessful wait for response on cellular data request message.\n");
        return A_IPC_RET_ERROR;
    }
}

static A_IPC_RESULT open_session(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf)
{
    A_IPC_RESULT rc = a_ipc_init_ipc_open_session_req(msg_buf);

    if (rc != A_IPC_RET_OK) {
        printf("Initialize open session request message failed: %s\n",
               a_ipc_result_str(rc));
        return rc;
    }
    rc = a_ipc_send(a_ipc,
                    A_SERVICE_ID_CELLULAR,
                    msg_buf);
    if (rc != A_IPC_RET_OK) {
        printf("IPC sending cellular data request message failed: %s\n",
            a_ipc_result_str(rc));
        return rc;
    }

    if (wait_for_msg(a_ipc,
                      msg_buf,
                     5000,
                     A_IPC_MSG_IPC_OPEN_SESSION_RSP)) {
        printf("Open session response result: %d\n",
               msg_buf->cellular_data_request_rsp.result);
        return A_IPC_RET_OK;
    } else {
        printf("Unsuccessful wait for response on open session request message.\n");
        return A_IPC_RET_ERROR;
    }
}

static A_IPC_RESULT release(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, char *user)
{
    A_IPC_RESULT rc = a_ipc_init_cellular_data_release_req_norsp(msg_buf, USER_MAX_LENGTH);

    if (rc != A_IPC_RET_OK) {
        printf("Initialize cellular data release message failed: %s\n",
            a_ipc_result_str(rc));
        return rc;
    }

    msg_buf->cellular_data_release_req_norsp.user.value = user;
    return send_ipc(a_ipc, msg_buf);
}

static A_IPC_RESULT subscribe(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, bool extended)
{
    A_IPC_RESULT rc;
    int wait_type;

    if (extended) {
        rc = a_ipc_init_cellular_extended_status_subscribe_req(msg_buf);
        wait_type = A_IPC_MSG_CELLULAR_EXTENDED_STATUS_SUBSCRIBE_RSP;
    } else {
        rc = a_ipc_init_cellular_status_subscribe_req(msg_buf);
        wait_type = A_IPC_MSG_CELLULAR_STATUS_SUBSCRIBE_RSP;
    }

    if (rc != A_IPC_RET_OK) {
        printf("Initialize cellular data subscribe message failed: %s\n",
            a_ipc_result_str(rc));
        return rc;
    }
    rc = a_ipc_send(a_ipc, A_SERVICE_ID_CELLULAR, msg_buf);
    if (rc != A_IPC_RET_OK) {
        printf("IPC sending cellular data subscribe message failed: %s\n",
            a_ipc_result_str(rc));
        return rc;
    }

    if (wait_for_msg(a_ipc,
                     msg_buf,
                     5000,
                     wait_type)) {
        printf("Cellular data subscribe response result: %d\n",
               msg_buf->cellular_status_subscribe_rsp.result);
        return A_IPC_RET_OK;
    } else {
        printf("Unsuccessful wait for response on cellular data subscribe message");
        return A_IPC_RET_ERROR;
    }
}

static A_IPC_RESULT unsubscribe(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, bool extended)
{
    A_IPC_RESULT rc;

    if (extended) {
        rc = a_ipc_init_cellular_extended_status_unsubscribe_req_norsp(msg_buf);
    } else {
        rc = a_ipc_init_cellular_status_unsubscribe_req_norsp(msg_buf);
    }

    if (rc != A_IPC_RET_OK) {
        printf("Initialize cellular data unsubscribe message failed: %s\n",
            a_ipc_result_str(rc));
        return rc;
    }

    return send_ipc(a_ipc, msg_buf);
}


static A_IPC_RESULT cell_wait_for_status(a_ipc_handle *a_ipc, uint32_t *cellid, uint16_t *tac, char *mcc, char *mnc)
{
    A_IPC_MSG_ON_STACK(msg_buf, 256);
    if (wait_for_msg(a_ipc,
                    msg_buf,
                    MAXIMAL_EXECUTION_TIME_S * 1000,
                    A_IPC_MSG_CELLULAR_EXTENDED_STATUS_PUBLISH_IND)) {

        // a_ipc_enum_cellular_connection_status data_status;
        switch (msg_buf->cellular_extended_status_publish_ind.data_status) {
        case A_IPC_ENUM_CELLULAR_CONNECTION_STATUS_NOT_CONNECTED:
            printf("status:  not connected\n");
            break;
        case A_IPC_ENUM_CELLULAR_CONNECTION_STATUS_SEARCHING:
            printf("status:  searching\n");
            break;
        case A_IPC_ENUM_CELLULAR_CONNECTION_STATUS_CONNECTED:
            printf("status:  connected\n");
            break;
        default:
            printf("status:  unknown\n");
            break;
        }

        // changes the value of variables to correct value
        *cellid = msg_buf->cellular_extended_status_publish_ind.cid;
        *tac = msg_buf->cellular_extended_status_publish_ind.lac_tac;
        strcpy(mcc, msg_buf->cellular_extended_status_publish_ind.mcc);
        strcpy(mnc, msg_buf->cellular_extended_status_publish_ind.mnc);

        return A_IPC_RET_OK;
    } else {
        printf("Did not get expected cellular extended status message.\n");
    }

    return A_IPC_RET_ERROR;
}

static A_IPC_RESULT cpy_cell_info(
    a_ipc_handle *a_ipc,
    a_ipc_msg *msg_buf, 
    uint32_t *cellid, uint16_t *tac, char *mcc, char *mnc)
{
    A_IPC_RESULT rc = subscribe(a_ipc, msg_buf, true);

    if (rc != A_IPC_RET_OK) {
        return rc;
    }

    rc = cell_wait_for_status(a_ipc, cellid, tac, mcc, mnc);

    if (rc != A_IPC_RET_OK) {
        return rc;
    }

    return unsubscribe(a_ipc, msg_buf, true);
}

/*  Changes the value of cellid, tac, mcc and mnc variable from connection, creates a msg buffer and more. Call from supl-client*/
void get_cell_data(uint32_t *cellid, uint16_t *tac, char mcc[4], char mnc[4]) {
    a_log_set_logger(a_log_stdout_logger);

    a_ipc_handle *a_ipc = a_ipc_init(IPC_ID, &ipc_buffers);
    if (a_ipc == NULL) {
        printf("AIPC init failed\n");
        return;
    }

    A_IPC_MSG_ON_STACK(msg_buf, 256);

    A_IPC_RESULT rc = A_IPC_RET_ERROR;
    rc = open_session(a_ipc, msg_buf);
    if (rc != A_IPC_RET_OK) {
        a_ipc_destroy(a_ipc);
        return;
    }

    rc = cpy_cell_info(a_ipc, msg_buf, cellid, tac, mcc, mnc);

    a_ipc_destroy(a_ipc);
    return;
}

/* changes the value if in variable imsi to the correct value */
static A_IPC_RESULT cpy_imsi(a_ipc_handle *a_ipc, a_ipc_msg *msg_buf, char *imsi)
{
    A_IPC_RESULT rc = a_ipc_init_cellular_info_req(msg_buf);

    if (rc != A_IPC_RET_OK) {
        printf("Initialize cellular info message failed: %s\n",
            a_ipc_result_str(rc));
        return rc;
    }
    rc = a_ipc_send(a_ipc, A_SERVICE_ID_CELLULAR, msg_buf);
    if (rc != A_IPC_RET_OK) {
        printf("IPC sending cellular info message failed: %s\n",
            a_ipc_result_str(rc));
        return rc;
    }

    if (wait_for_msg(a_ipc,
                     msg_buf,
                     5000,
                     A_IPC_MSG_CELLULAR_INFO_RSP)) {
        // changes imsi to correct value
        //printf("imsi: %s\n", msg_buf->cellular_info_rsp.imsi.value);
        strcpy(imsi, msg_buf->cellular_info_rsp.imsi.value);

        return A_IPC_RET_OK;
    } else {
        printf("Unsuccessful wait for response on cellular info message");
        return A_IPC_RET_ERROR;
    }
}

/* OLD DONT USE; Changes the value of imsi in-variable, sets it to imsi from box, creates a msg buffer and more. Call from supl-client*/
void get_imsi(char imsi[16]) {
    a_log_set_logger(a_log_stdout_logger);

    a_ipc_handle *a_ipc = a_ipc_init(IPC_ID, &ipc_buffers);
    if (a_ipc == NULL) {
        printf("AIPC init failed\n");
        return; //1;
    }

    A_IPC_MSG_ON_STACK(msg_buf, 256);

    A_IPC_RESULT rc = A_IPC_RET_ERROR;
    rc = open_session(a_ipc, msg_buf);
    if (rc != A_IPC_RET_OK) {
        a_ipc_destroy(a_ipc);
        return;// rc;
    }

    rc = cpy_imsi(a_ipc, msg_buf, imsi);

    a_ipc_destroy(a_ipc);
    return; //rc;
}

/* Returns the value of imsi, creates a msg buffer and more. Call from supl-client*/
unsigned long get_long_imsi(){
    a_log_set_logger(a_log_stdout_logger);

    a_ipc_handle *a_ipc = a_ipc_init(IPC_ID, &ipc_buffers);
    if (a_ipc == NULL) {
        printf("AIPC init failed\n");
        return 0; //1;
    }

    A_IPC_MSG_ON_STACK(msg_buf, 256);

    A_IPC_RESULT rc = A_IPC_RET_ERROR;
    rc = open_session(a_ipc, msg_buf);
    if (rc != A_IPC_RET_OK) {
        a_ipc_destroy(a_ipc);
        return 0; // rc;
    }

    char imsi[16];
    rc = cpy_imsi(a_ipc, msg_buf, imsi);
    unsigned long imsi_ul = strtoul(imsi, NULL, 10);

    a_ipc_destroy(a_ipc);
    return imsi_ul; //rc;
}


int main(int argc, char **argv)
{
    unsigned long imsi_long = get_long_imsi();
    printf("imsi long: %lu\n", imsi_long);
    // printf("before \n");
    // uint32_t cellid2 = 0;
    // uint16_t tac2 = 0;
    // char mcc2[3+1] = "";
    // char mnc2[3+1] = "";
    char imsi2[16] = "";
    get_imsi(imsi2);
    // //return get_imsi(imsi2);
    // get_cell_data(&cellid2, &tac2, mcc2, mnc2);
    printf("imsi after func:   %s\n", imsi2);
    // printf("cid2:     %08x\n", cellid2);
    // printf("tac2:     %04x\n", tac2);
    // printf("mcc2:     %s\n", mcc2);
    // printf("mnc2:     %s\n", mnc2);
    // return 1;
    a_log_set_logger(a_log_stdout_logger);

    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    a_ipc_handle *a_ipc = a_ipc_init(IPC_ID, &ipc_buffers);
    if (a_ipc == NULL) {
        printf("AIPC init failed\n");
        return 1;
    }

    A_IPC_MSG_ON_STACK(msg_buf, 256);

    char *user = "cellulardata1";

    char *command = argv[1];

    A_IPC_RESULT rc = A_IPC_RET_ERROR;
    rc = open_session(a_ipc, msg_buf);
    if (rc != A_IPC_RET_OK) {
        a_ipc_destroy(a_ipc);
        return rc;
    }

    // uint32_t cellid = 0;
    // uint16_t tac = 0;
    // char mcc[3+1] = "";
    // char mnc[3+1] = "";
    // char imsi[16] = "";

    if (strcmp(command, "request") == 0) {
        rc = request(a_ipc, msg_buf, user);
    } else if (strcmp(command, "release") == 0) {
        rc = release(a_ipc, msg_buf, user);
    }
    // }  else if (strcmp(command, "cell") == 0) {
    //     rc = cpy_cell_info(a_ipc, msg_buf, &cellid, &tac, mcc, mnc);
    // } else if (strcmp(command, "imsi") == 0) {
    //     rc = cpy_imsi(a_ipc, msg_buf, imsi);
    // } 
    else {
        a_ipc_destroy(a_ipc);
        usage(argv[0]);
        return 1;
    }
    // printf("cid ref:     %08x\n", cellid);
    // printf("tac ref:     %04x\n", tac);
    // printf("new mcc:     %s\n", mcc);
    // printf("new mnc:     %s\n", mnc);
    // printf("new imsi:   %s\n", imsi);

    a_ipc_destroy(a_ipc);
    return rc;
}
