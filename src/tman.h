/*
 ******************************************************************************
 * This file is part of Tman.
 *
 * Tman is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tman.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2014 Red Hat, Inc., Jakub Prokeš <jprokes@redhat.com>
 * Author: Jakub Prokeš <jprokes@redhat.com>
 *
 ******************************************************************************
 */

#ifndef _TMAN_H
#define _TMAN_H

#include <time.h>

typedef union {
    struct _Member {
        struct timespec delay;
        unsigned char type;
        struct timespec delta;
    } member;
    char data[sizeof(struct _Member)];
} tmanMSG_t;

#define T_SET 1
#define T_SUB 2
#define T_ADD 4
#define T_MUL 8

#define MQ_MSGSIZE sizeof(tmanMSG_t)
#define MQ_MAXMSG 5
#define MQ_PREFIX "/tman"
#define MQ_MAXNAMELENGTH 64


#endif
