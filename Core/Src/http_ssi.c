/*
 * http_ssi.c
 *
 *  Created on: 11-Oct-2021
 *      Author: controllerstech
 */

#include"http_ssi.h"

#include "lwip/tcp.h"
#include "lwip/apps/httpd.h"

#include "stm32f4xx_hal.h"
#include "tcpClientRAW.h"

#define numSSItags 3
LED_State_Struct led_state;

// ---------- SSI handler ---------- //
char const *theSSItags[numSSItags] = { "tag1", "tag2", "tag3" };

uint16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen) {

	if (iIndex == 0) {

		if (led_state.green == false) {

			char myStr1[] = "<option>ON<option selected>OFF";
			strcpy(pcInsert, myStr1);
			return strlen(myStr1);
		}

		else if (led_state.green == true) {

			char myStr1[] = "<option selected>ON<option>OFF";
			strcpy(pcInsert, myStr1);
			return strlen(myStr1);

		}

	}

	else if (iIndex == 1)

	{
		if (led_state.blue == false) {
			char myStr2[] = "<option>ON<option selected>OFF";
			strcpy(pcInsert, myStr2);
			return strlen(myStr2);
		}

		else if (led_state.blue == true) {

			char myStr2[] = "<option selected>ON<option>OFF";
			strcpy(pcInsert, myStr2);
			return strlen(myStr2);

		}

	} else if (iIndex == 2)

	{
		if (led_state.red == false) {
			char myStr2[] = "<option>ON<option selected>OFF";
			strcpy(pcInsert, myStr2);
			return strlen(myStr2);
		}

		else if (led_state.red == true) {

			char myStr2[] = "<option selected>ON<option>OFF";
			strcpy(pcInsert, myStr2);
			return strlen(myStr2);

		}

	}

	return 0;

}

// ---------- CGI handler ---------- //
const char* LedControlCgiHandler(int index, int numParams, char *pcParam[],
		char *pcValue[]);

const tCGI LED_CGI = { "/LEDControl.cgi", LedControlCgiHandler };

const char* LedControlCgiHandler(int index, int numParams, char *pcParam[],
		char *pcValue[]) {
	if (index == 0) {
		send_web_request();
		for (int i = 0; i < numParams; i++) {
			if (strcmp(pcParam[i], "green") == 0) {
				if (strcmp(pcValue[i], "ON") == 0) {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // Turn On Greed Led
					led_state.green = true;
				} else {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); // Turn Off Greed Led
					led_state.green = false;
				}
			} else if (strcmp(pcParam[i], "blue") == 0) {
				if (strcmp(pcValue[i], "ON") == 0) {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); // Turn On Blue Led
					led_state.blue = true;
				} else {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); // Turn Off Blue Led
					led_state.blue = false;
				}
			} else if (strcmp(pcParam[i], "red") == 0) {
				if (strcmp(pcValue[i], "ON") == 0) {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // Turn On Red Led
					led_state.red = true;
				} else {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // Turn Off Red Led
					led_state.red = false;
				}
			}
		}
	}

	return "/index.shtml";
}

void http_server_init(void) {
	httpd_init();
	http_set_ssi_handler(mySSIHandler, (char const**) theSSItags, numSSItags);
	http_set_cgi_handlers(&LED_CGI, 1);
}
