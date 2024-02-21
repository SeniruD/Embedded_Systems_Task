/*
 * http_ssi.h
 *
 *  Created on: Feb 21, 2024
 *      Author: SENIRU
 */

#ifndef INC_HTTP_SSI_H_
#define INC_HTTP_SSI_H_

#include "stdbool.h"
#include "string.h"
#include "stdio.h"

void http_server_init (void);

typedef struct {
    volatile bool green;
    volatile bool blue;
    volatile bool red;
} LED_State_Struct;

extern LED_State_Struct led_state;

#endif /* INC_HTTP_SSI_H_ */
