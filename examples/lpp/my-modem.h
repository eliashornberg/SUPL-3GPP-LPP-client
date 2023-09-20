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

#include <lpp/cell_id.h>
#include "actia/actia.h"

struct CellID get_cell_data();
unsigned long get_imsi();

#ifdef __cplusplus
}
#endif

#endif /* MY_MODEM_H */
