/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/
/**
 * @file at_cmd_common.c
 * @author Michael Liao
 * @date 14 Dec 2017
 * @brief File contains the AT Command common api.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include "opl1000.h"
#include "os.h"
#include "at_cmd.h"
#include "at_cmd_sys.h"
#include "at_cmd_wifi.h"
#include "at_cmd_ble.h"
#include "at_cmd_tcpip.h"
#include "at_cmd_rf.h"
#include "at_cmd_pip.h"
#include "at_cmd_others.h"
#include "le_ctrl.h"
#include "hal_uart.h"
#include "at_cmd_common.h"
#include "hal_dbg_uart.h"
#include "hal_uart.h"
#if defined(__AT_CMD_TASK__)
#include "at_cmd_task.h"
#endif
#include "at_cmd_data_process.h"
#include "wpa_supplicant_i.h"
#include "le_cmd_app_cmd.h"

/*
 * @brief Global variable msg_print_uart1 retention attribute segment
 *
 */
RET_DATA msg_print_uart1_fp_t msg_print_uart1;

/*
 * @brief Global variable uart1_mode_get retention attribute segment
 *
 */
RET_DATA uart1_mode_get_fp_t uart1_mode_get;

/*
 * @brief Global variable uart1_mode_set retention attribute segment
 *
 */
RET_DATA uart1_mode_set_fp_t uart1_mode_set;

/*
 * @brief Global variable uart1_mode_set_at retention attribute segment
 *
 */
RET_DATA uart1_mode_set_at_fp_t uart1_mode_set_at;

/*
 * @brief Global variable uart1_mode_set_ble_hci retention attribute segment
 *
 */
RET_DATA uart1_mode_set_ble_hci_fp_t uart1_mode_set_ble_hci;

/*
 * @brief Global variable uart1_mode_set_ble_host retention attribute segment
 *
 */
RET_DATA uart1_mode_set_ble_host_fp_t uart1_mode_set_ble_host;

/*
 * @brief Global variable uart1_mode_set_others retention attribute segment
 *
 */
RET_DATA uart1_mode_set_others_fp_t uart1_mode_set_others;

/*
 * @brief Global variable uart1_mode_set_default retention attribute segment
 *
 */
RET_DATA uart1_mode_set_default_fp_t uart1_mode_set_default;

/*
 * @brief Global variable uart1_rx_int_do_at retention attribute segment
 *
 */
RET_DATA uart1_rx_int_do_at_fp_t uart1_rx_int_do_at;

/*
 * @brief Global variable uart1_rx_int_do_ble_hci retention attribute segment
 *
 */
RET_DATA uart1_rx_int_do_ble_hci_fp_t uart1_rx_int_do_ble_hci;

/*
 * @brief Global variable uart1_rx_int_do_ble_host retention attribute segment
 *
 */
RET_DATA uart1_rx_int_do_ble_host_fp_t uart1_rx_int_do_ble_host;

/*
 * @brief Global variable uart1_rx_int_handler_default retention attribute segment
 *
 */
RET_DATA uart1_rx_int_handler_default_fp_t uart1_rx_int_handler_default;

/*
 * @brief Global variable uart1_rx_int_do_others retention attribute segment
 *
 */
RET_DATA uart1_rx_int_do_others_fp_t uart1_rx_int_do_others;

/*
 * @brief Global variable at_clear_uart_buffer retention attribute segment
 *
 */
RET_DATA at_clear_uart_buffer_fp_t at_clear_uart_buffer;

/*
 * @brief Global variable g_uart1_mode retention attribute segment
 *
 */
RET_DATA unsigned int g_uart1_mode;

/*
 * @brief Global variable at_rx_buf retention attribute segment
 *
 */
RET_DATA at_uart_buffer_t at_rx_buf;

/*
 * @brief Global variable _uart1_rx_int_at_data_receive_ble retention attribute segment
 *
 */
RET_DATA _uart1_rx_int_at_data_receive_ble_fp_t _uart1_rx_int_at_data_receive_ble;

/*
 * @brief Global variable _uart1_rx_int_at_data_receive_tcpip retention attribute segment
 *
 */
RET_DATA _uart1_rx_int_at_data_receive_tcpip_fp_t _uart1_rx_int_at_data_receive_tcpip;

/*
 * @brief Global variable set_echo_on retention attribute segment
 *
 */
RET_DATA set_echo_on_fp_t set_echo_on;

/*
 * @brief Global variable _uart1_rx_int_at_data_receive_tcpip retention attribute segment
 *
 */
RET_DATA int at_echo_on;

extern struct wpa_supplicant *wpa_s;

uint8_t at_state = AT_STATE_IDLE;
uint8_t *pDataLine;
uint8_t  at_data_line[AT_DATA_LEN_MAX];
extern uint16_t at_send_len;


//at global variables
static char *at_result_string[AT_RESULT_CODE_MAX] = {
    "\r\nOK\r\n",         //AT_RESULT_CODE_OK         = 0x00,
    "\r\nERROR\r\n",      //AT_RESULT_CODE_ERROR      = 0x01,
    "\r\nERROR\r\n",      //AT_RESULT_CODE_FAIL       = 0x02,
    "\r\nSEND OK\r\n",    //AT_RESULT_CODE_SEND_OK    = 0x03,
    "\r\nSEND FAIL\r\n",  //AT_RESULT_CODE_SEND_FAIL  = 0x04,
    NULL,
    NULL,
};

/*
 * @brief Print Message to UART1
 *
 * @param [in] fmt message
 */
void msg_print_uart1_impl(char *fmt,...)
{
	va_list ap;
    char string[256];
    char *pt;

	{
		va_start(ap,fmt);
    	vsprintf(string,fmt,ap);

	    pt = &string[0];
        while(*pt != '\0')
        {
            if(*pt == '\n') {
                Hal_Uart_DataSend(UART_IDX_1, '\r');
            }
            Hal_Uart_DataSend(UART_IDX_1, *pt++);
        }

    	va_end(ap);
	}
}

/*
 * @brief Get UART1 mode
 *
 * @return int Current UART1 mode
 */
int uart1_mode_get_impl(void)
{
    return g_uart1_mode;
}

/*
 * @brief UART1 Mode Setting's Handler
 *
 * @param [in] argc count of parameters
 *
 * @param [in] argv parameters array
 *
 * @return 0 fail 1 success
 *
 */
int uart1_mode_set_impl(int argc, char *argv[])
{
    int mode = UART1_MODE_DEFAULT;

    if(argc > 1) {
        mode = atoi(argv[1]);
        if ((mode >= UART1_MODE_NUM) || (mode < UART1_MODE_AT)) return false;
    }
    g_uart1_mode = mode;

    switch (mode) {
    	case UART1_MODE_AT:
            /** Do something when set AT mode */
            uart1_mode_set_at();
    		break;
        case UART1_MODE_BLE_HCI:
            /** Do something when set BLE HCI mode */
            uart1_mode_set_default();
            uart1_mode_set_ble_hci();
    		break;
        case UART1_MODE_OTHERS:
            /** Do something when set Others mode */
            uart1_mode_set_others();
            break;
    	default:
    		break;
	}

    return true;
}

/*
 * @brief UART1 AT Mode's Configuration
 *
 */
void uart1_mode_set_at_impl(void)
{
    /** Nothing to do */
}

/*
 * @brief UART1 BLE HCI Mode's Configuration
 *
 */
void uart1_mode_set_ble_hci_impl(void)
{
    le_ctrl_data.is_uart_hci_mode = (g_uart1_mode == UART1_MODE_BLE_HCI);
}

/*
 * @brief UART1 BLE HOST Mode's Configuration ()
 *
 */
void uart1_mode_set_ble_host_impl(void)
{
    /** Reserved */
}

/*
 * @brief UART1 BLE HCI Mode's Configuration
 *
 */
void uart1_mode_set_others_impl(void)
{
    /** Reserved */
}

/*
 * @brief Configure the UART1 to default UART AT Mode
 *
 */
void uart1_mode_set_default_impl(void)
{
    Hal_Uart_RxCallBackFuncSet(UART_IDX_1, uart1_rx_int_handler_default);
    Hal_Uart_RxIntEn(UART_IDX_1, 1);
}

#if 0
/*
 * @brief AT mode handler
 *
 * @param [in] u32Data 4bytes data from UART1 interrupt
 */
void uart1_rx_int_do_at_impl(uint32_t u32Data)
{
    at_uart_buffer_t *p;
    xATMessage txMsg;

    p = &at_rx_buf;
    if ((p->in & ~(AT_RBUF_SIZE-1)) == 0)
    {
        if((u32Data & 0xFF) == 0x0D)
        {
            p->buf [p->in & (AT_RBUF_SIZE-1)] = 0x00;

            /** send message */
            txMsg.pcMessage = (char *)p;
            txMsg.length = sizeof(at_uart_buffer_t);
            txMsg.event = AT_UART1_EVENT;

            /** Post the byte. */
            at_task_send( txMsg );
        }
        else if((u32Data & 0xFF) == 0x08)
        {
            if (p->in > 0)
            {
                p->in--;
                p->buf[p->in & (AT_RBUF_SIZE-1)] = 0x00;
                Hal_Uart_DataSend(UART_IDX_1, 0x08);
                Hal_Uart_DataSend(UART_IDX_1, 0x20);
                Hal_Uart_DataSend(UART_IDX_1, 0x08);
            }
        }
        else
        {
            p->buf [p->in & (AT_RBUF_SIZE-1)] = (u32Data & 0xFF);
            Hal_Uart_DataSend(UART_IDX_1, p->buf [p->in]);
            p->in++;
        }
    }
    else
    {
        p->in = 0;
        p->buf [p->in & (AT_RBUF_SIZE-1)] = (u32Data & 0xFF);
        Hal_Uart_DataSend(UART_IDX_1, p->buf [p->in]);
    }

}
#endif

/*
 * @brief UART1 RX Data Handler of BLE (Reserved)
 *
 * @param [in] u32Data 4bytes data from UART1 interrupt
 */
void _uart1_rx_int_at_data_receive_ble_impl(uint32_t u32Data)
{
	LeCmdAppProcessSendData((char *)&u32Data, 1);
}

/*
 * @brief UART1 RX Data Handler of TCP/IP (Reserved)
 *
 * @param [in] u32Data 4bytes data from UART1 interrupt
 */
void _uart1_rx_int_at_data_receive_tcpip_impl(uint32_t u32Data)
{
   int send_len = 0;

    send_len = data_process_data_len_get();

    *pDataLine = (u32Data & 0xFF);

    //ToDo:
    //Enter transparent transmission, with a 20-ms
    //interval between each packet, and a maximum of
    //2048 bytes per packet.
    //When a single packet containing +++ is received,
    //returns to normal command mode.

    if (at_state == AT_STATE_SENDING_RECV)
    {
        //if not transparent transmission mode, display back
        //if(( (u32Data & 0xFF) != '\n') && (at_echo_on))
        {
            Hal_Uart_DataSend(UART_IDX_1, (u32Data & 0xFF));
        }

        if ((pDataLine >= &at_data_line[send_len - 1]) || (pDataLine >= &at_data_line[AT_DATA_LEN_MAX - 1]))
        {
            at_event_msg_t msg = {0};

            msg.event = AT_DATA_TX_EVENT;
            msg.length = send_len;
            msg.param = at_data_line;
            at_data_task_send(&msg);
            at_state = AT_STATE_SENDING_DATA;
            pDataLine = at_data_line;
        }
        else
        {
            pDataLine++;
        }
    }
    else //AT_STATE_SENDING_DATA
    {
        at_uart1_printf((char *)"\r\nbusy p...\r\n");
    }
}

void _uart1_rx_int_do_at_impl(uint32_t u32Data)
{
    at_uart_buffer_t *p;
    xATMessage txMsg;
    int lock = data_process_lock_get();

    p = &at_rx_buf;

    //command input for all modules
    if (lock == LOCK_NONE)
    {
        if ((p->in & ~(AT_RBUF_SIZE-1)) == 0)
        {
            if((u32Data & 0xFF) == 0x0D)//Enter
            {//detect the ending character, send command buffer to at cmd task
                p->buf [p->in & (AT_RBUF_SIZE-1)] = 0x00;

                /** send message */
                txMsg.pcMessage = (char *)p;
                txMsg.length = sizeof(at_uart_buffer_t);
                txMsg.event = AT_UART1_EVENT;

                /** Post the byte. */
                at_task_send( txMsg );
            }
            else if((u32Data & 0xFF) == 0x08)
            {//backspace
                if (p->in > 0)
                {
                    p->in--;
                    p->buf[p->in & (AT_RBUF_SIZE-1)] = 0x00;
                    Hal_Uart_DataSend(UART_IDX_1, 0x08);
                    Hal_Uart_DataSend(UART_IDX_1, 0x20);
                    Hal_Uart_DataSend(UART_IDX_1, 0x08);
                }
            }
            else
            {//each character input
                p->buf [p->in & (AT_RBUF_SIZE-1)] = (u32Data & 0xFF);
                if(at_echo_on) Hal_Uart_DataSend(UART_IDX_1, p->buf [p->in]);
                p->in++;
            }
        }
        else
        { //error case: overwrite
            p->in = 0;
            p->buf [p->in & (AT_RBUF_SIZE-1)] = (u32Data & 0xFF);
            Hal_Uart_DataSend(UART_IDX_1, p->buf [p->in]);
        }
    }

    //data input for BLE or TCP/IP module
    else
    {
        //Set handler of BLE or TCP/IP module
        switch(lock)
        {
            case LOCK_BLE:
                _uart1_rx_int_at_data_receive_ble_impl(u32Data);
                break;

            case LOCK_TCPIP:
                _uart1_rx_int_at_data_receive_tcpip_impl(u32Data);
                break;

            default:
                break;
        }
    }
}

/*
 * @brief BLE HCI mode handler
 *
 * @param [in] u32Data 4bytes data from UART1 interrupt
 */
void uart1_rx_int_do_ble_hci_impl(uint32_t u32Data)
{
    le_ctrl_hci_uart_input(u32Data);
}

/*
 * @brief BLE HOST mode handler (Reserved)
 *
 * @param [in] u32Data 4bytes data from UART1 interrupt
 */
void uart1_rx_int_do_ble_host_impl(uint32_t u32Data)
{
    /** Reserved */
}

/*
 * @brief Others mode handler (Reserved)
 *
 * @param [in] u32Data 4bytes data from UART1 interrupt
 */
void uart1_rx_int_do_others_impl(uint32_t u32Data)
{
    /** Reserved */
}

/*
 * @brief UART1 RX Interrupt default handler
 *
 * @param [in] u32Data 4bytes data from UART1 interrupt
 */
void uart1_rx_int_handler_default_impl(uint32_t u32Data)
{
    int mode;

    mode = uart1_mode_get();
    switch (mode) {
    	case UART1_MODE_AT:
            /** Do AT CMD */
            uart1_rx_int_do_at(u32Data);
    		break;
        case UART1_MODE_BLE_HCI:
            /** Do BLE HCI */
            uart1_rx_int_do_ble_hci(u32Data);
    		break;
        case UART1_MODE_BLE_HOST:
            /** Do BLE HOST (Reserved) */
            //uart1_rx_int_do_ble_host(u32Data);
    		break;
        case UART1_MODE_OTHERS:
            /** Do Others (Reserved) */
            uart1_rx_int_do_others(u32Data);
            break;
    	default:
            /** Do AT CMD */
            uart1_rx_int_do_at(u32Data);
    		break;
	}
}

/*
 * @brief buffer flag clear
 *
 */
void at_clear_uart_buffer_impl(void)
{
    at_rx_buf.in = 0;
}

char *at_cmd_param_trim(char *sParam)
{
    char *sRet = NULL;
    int iLen = strlen(sParam);

    if(iLen == 0) goto done;

    if(sParam[0] == '"')
    {
        if(iLen < 2)
        {
            printf("[%s %d] invalid param[%s]\n", __func__, __LINE__, sParam);
            goto done;
        }

        if(sParam[iLen - 1] != '"')
        {
            printf("[%s %d] invalid param[%s]\n", __func__, __LINE__, sParam);
            goto done;
        }

        sParam[0] = 0;
        sParam[iLen - 1] = 0;
        sRet = &(sParam[1]);
    }
    else
    {
        if(sParam[iLen - 1] == '"')
        {
            printf("[%s %d] invalid param[%s]\n", __func__, __LINE__, sParam);
            goto done;
        }

        sRet = sParam;
    }

done:
    return sRet;
}

int _at_cmd_buf_to_argc_argv(char *pbuf, int *argc, char *argv[], int iArgvNum)
{
    int count = 0;
    char *p = NULL;
    char *pTrim = NULL;

    if((!pbuf) || (!argc) || (!argv) || (!iArgvNum))
    {
        return 0;
    }

    /** Get the first word */
    p = strtok(pbuf, "=");
    argv[count] = p;
    //msg_print_uart1("\r\n _at_cmd_buf_to_argc_argv, argv[%d]:%s ", count, argv[count]);
    count++;

	while ((p = strtok(NULL, ",")) != NULL)
    {
        if(count >= iArgvNum)
        {
            break;
        }

	    //argv[count] = p;
        //msg_print_uart1("\r\n _at_cmd_buf_to_argc_argv, argv[%d]:%s ", count, argv[count]);
        pTrim = at_cmd_param_trim(p);
        if(pTrim == NULL)
        {
            argv[count] = NULL;
            return false;
        }
        argv[count] = pTrim;
        count++;
    }
    *argc = count;

	return true;
}

void at_uart1_write_buffer(char *buf, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        if(buf[i] == '\n') {
            Hal_Uart_DataSend(UART_IDX_1, '\r');
        }
        Hal_Uart_DataSend(UART_IDX_1, buf[i]);
    }
}

void at_uart1_printf(char *str)
{
    if (str == NULL) {
        return;
    }
    at_uart1_write_buffer(str, strlen(str));
}

void at_response_result(uint8_t result_code)
{
    if (result_code < AT_RESULT_CODE_MAX) {
        at_uart1_printf(at_result_string[result_code]);
    }
}

void set_echo_on_impl(int on)
{
    int enable = false;
    if(on) enable = true;
    at_echo_on = enable;
}

int wpas_get_state(void)
{
    if (wpa_s == NULL) return 0;
	return ((int)wpa_s->wpa_state);
}

int wpas_get_assoc_freq(void)
{
    if (wpa_s == NULL) return 0;
    return ((int)wpa_s->assoc_freq);
}

/*
 * @brief AT Common Interface Initialization (AT Common)
 *
 */
void at_cmd_common_func_init(void)
{
    /** Functions: AT Common */
    msg_print_uart1 = msg_print_uart1_impl;
    uart1_mode_get = uart1_mode_get_impl;
    uart1_mode_set = uart1_mode_set_impl;
    uart1_mode_set_at = uart1_mode_set_at_impl;
    uart1_mode_set_ble_hci = uart1_mode_set_ble_hci_impl;
    uart1_mode_set_ble_host = uart1_mode_set_ble_host_impl;
    uart1_mode_set_others = uart1_mode_set_others_impl;
    uart1_mode_set_default = uart1_mode_set_default_impl;
    //uart1_rx_int_do_at = uart1_rx_int_do_at_impl;
    uart1_rx_int_do_ble_hci = uart1_rx_int_do_ble_hci_impl;
    uart1_rx_int_do_ble_host = uart1_rx_int_do_ble_host_impl;
    uart1_rx_int_do_others = uart1_rx_int_do_others_impl;
    uart1_rx_int_handler_default = uart1_rx_int_handler_default_impl;
    at_clear_uart_buffer = at_clear_uart_buffer_impl;

    uart1_rx_int_do_at = _uart1_rx_int_do_at_impl;
    _uart1_rx_int_at_data_receive_ble = _uart1_rx_int_at_data_receive_ble_impl;
    _uart1_rx_int_at_data_receive_tcpip = _uart1_rx_int_at_data_receive_tcpip_impl;

    at_echo_on = true;
    set_echo_on = set_echo_on_impl;
}

