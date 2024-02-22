#include "tcpClientRAW.h"
#include "lwip/tcp.h"
#include "string.h"
#include "http_ssi.h"

/*  protocol states */
enum tcp_client_states {
	ES_NONE = 0, ES_CONNECTED, ES_RECEIVING, ES_CLOSING
};

/* structure for maintaining connection infos to be passed as argument
 to LwIP callbacks*/
struct tcp_client_struct {
	u8_t state; /* current connection state */
	u8_t retries;
	struct tcp_pcb *pcb; /* pointer on the current tcp_pcb */
	struct pbuf *p; /* pointer on the received/to be transmitted pbuf */
};

/* This callback will be called, when the client is connected to the server */
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);

/* This callback will be called, when the client receive data from the server */
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
		err_t err);

/* This callback will be called, when the server Polls for the Client */
static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb);

/* This callback will be called, when the server acknowledges the data sent by the client */
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);

/* A Function to send the data to the server */
static void tcp_client_send(struct tcp_pcb *tpcb, struct tcp_client_struct *es);

/* Function to close the connection */
static void tcp_client_connection_close(struct tcp_pcb *tpcb,
		struct tcp_client_struct *es);

/* This is the part where we are going to handle the incoming data from the server */
static void tcp_client_handle(struct tcp_pcb *tpcb,
		struct tcp_client_struct *es);


/* create a struct to store data */
struct tcp_client_struct *esTx = 0;

struct tcp_pcb *pcbTx = 0;

const char *host = "192.168.1.111:80";
const char *path = "/webpage_request";
char json_data[1000];

extern RTC_HandleTypeDef hrtc;

extern RTC_DateTypeDef sDate;
extern RTC_TimeTypeDef sTime;

extern LED_State_Struct led_state;

char datetime_str[50];
char led_state_str[30];
char buf[1500];
int data_len = 0;
int len = 0;

char* led_state_to_string(bool value) {
	if (value == true) {
		return "ON";
	} else {
		return "OFF";
	}
}

void send_web_request(void) {

	/* Get the current time and date */

	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	sprintf(datetime_str,
			"Date : %02d/%02d/%02d, Time: %02d:%02d:%02d",
			sDate.Date, sDate.Month, sDate.Year, sTime.Hours, sTime.Minutes,
			sTime.Seconds);

	/* Get the current LED state */

	sprintf(led_state_str, "G_LED:%s B_LED:%s R_LED:%s",
			led_state_to_string(led_state.green),
			led_state_to_string(led_state.blue),
			led_state_to_string(led_state.red));

	data_len = sprintf(json_data,
			"{\"LAST CHANGE\": \"%s\",\"LED_STATES\" : \"%s\"}", datetime_str, led_state_str);
	len = sprintf(buf, "POST %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %d\r\n\r\n"
			"%s", path, host, data_len, json_data);
	/* allocate pbuf */
	esTx->p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_POOL);

	/* copy data to pbuf */
	pbuf_take(esTx->p, (char*) buf, len);

	tcp_client_send(pcbTx, esTx);

	pbuf_free(esTx->p);

}

void send_web_request_to_server(void) {
	send_web_request();
}

/* IMPLEMENTATION FOR TCP CLIENT

 1. Create TCP block.
 2. connect to the server
 3. start communicating
 */

void tcp_client_init(void) {
  
	/* 1. create new tcp pcb */
	struct tcp_pcb *tpcb;
	tpcb = tcp_new();

	/* 2. Connect to the server */
	ip_addr_t destIPADDR;
	IP_ADDR4(&destIPADDR, 192, 168, 1, 113); // IP address of the server
	tcp_connect(tpcb, &destIPADDR, 5000, tcp_client_connected);
}

/** This callback is called, when the client is connected to the server
 * Here we will initialise few other callbacks
 * and in the end, call the client handle function
 */
static err_t tcp_client_connected(void *arg, struct tcp_pcb *newpcb, err_t err) {
	err_t ret_err;
	struct tcp_client_struct *es;

	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	/* allocate structure es to maintain tcp connection information */
	es = (struct tcp_client_struct*) mem_malloc(
			sizeof(struct tcp_client_struct));
	if (es != NULL) {
		es->state = ES_CONNECTED;
		es->pcb = newpcb;
		es->retries = 0;
		es->p = NULL;

		/* pass newly allocated es structure as argument to newpcb */
		tcp_arg(newpcb, es);

		/* initialize lwip tcp_recv callback function for newpcb  */
		tcp_recv(newpcb, tcp_client_recv);

		/* initialize lwip tcp_poll callback function for newpcb */
		tcp_poll(newpcb, tcp_client_poll, 0);

		/* initialize LwIP tcp_sent callback function */
		tcp_sent(newpcb, tcp_client_sent);

		/* handle the TCP data */
		tcp_client_handle(newpcb, es);

		ret_err = ERR_OK;
	} else {
		/*  close tcp connection */
		tcp_client_connection_close(newpcb, es);
		/* return memory error */
		ret_err = ERR_MEM;
	}
	return ret_err;
}


/** This callback is called, when the client receives some data from the server
 * if the data received is valid, we will handle the data in the client handle function
  */
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct tcp_client_struct *es;
  err_t ret_err;

  LWIP_ASSERT("arg != NULL",arg != NULL);

  es = (struct tcp_client_struct *)arg;

  /* if we receive an empty tcp frame from server => close connection */
  if (p == NULL)
  {
    /* remote host closed connection */
    es->state = ES_CLOSING;
    if(es->p == NULL)
    {
       /* we're done sending, close connection */
       tcp_client_connection_close(tpcb, es);
    }
    else
    {
      /* we're not done yet */
//      /* acknowledge received packet */
//      tcp_sent(tpcb, tcp_client_sent);

      /* send remaining data*/
//      tcp_client_send(tpcb, es);
    }
    ret_err = ERR_OK;
  }
  /* else : a non empty frame was received from server but for some reason err != ERR_OK */
  else if(err != ERR_OK)
  {
    /* free received pbuf*/
    if (p != NULL)
    {
      es->p = NULL;
      pbuf_free(p);
    }
    ret_err = err;
  }
  else if(es->state == ES_CONNECTED)
  {
   /* store reference to incoming pbuf (chain) */
    es->p = p;

    // tcp_sent has already been initialized in the beginning.
//    /* initialize LwIP tcp_sent callback function */
//    tcp_sent(tpcb, tcp_client_sent);

    /* Acknowledge the received data */
    tcp_recved(tpcb, p->tot_len);

    /* handle the received data */
    tcp_client_handle(tpcb, es);

    pbuf_free(p);

    ret_err = ERR_OK;
  }
  else if(es->state == ES_CLOSING)
  {
    /* odd case, remote side closing twice, trash data */
    tcp_recved(tpcb, p->tot_len);
    es->p = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  else
  {
    /* unknown es->state, trash data  */
    tcp_recved(tpcb, p->tot_len);
    es->p = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}


static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct tcp_client_struct *es;

  es = (struct tcp_client_struct *)arg;
  if (es != NULL)
  {
    if (es->p != NULL)
    {
        // tcp_sent has already been initialized in the beginning.
//      tcp_sent(tpcb, tcp_client_sent);
      /* there is a remaining pbuf (chain) , try to send data */
//      tcp_client_send(tpcb, es);
    }
    else
    {
      /* no remaining pbuf (chain)  */
      if(es->state == ES_CLOSING)
      {
        /*  close tcp connection */
        tcp_client_connection_close(tpcb, es);
      }
    }
    ret_err = ERR_OK;
  }
  else
  {
    /* nothing to be done */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

/** This callback is called, when the server acknowledges the data sent by the client
 * If there is no more data left to sent, we will simply close the connection
  */
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct tcp_client_struct *es;

  LWIP_UNUSED_ARG(len);

  es = (struct tcp_client_struct *)arg;
  es->retries = 0;

  if(es->p != NULL)
  {
	// tcp_sent has already been initialized in the beginning.
    /* still got pbufs to send */
//    tcp_sent(tpcb, tcp_client_sent);


//    tcp_client_send(tpcb, es);
  }
  else
  {
    /* if no more data to send and client closed connection*/
    if(es->state == ES_CLOSING)
      tcp_client_connection_close(tpcb, es);
  }
  return ERR_OK;
}


/** A function to send the data to the server
  */
static void tcp_client_send(struct tcp_pcb *tpcb, struct tcp_client_struct *es)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;

  while ((wr_err == ERR_OK) &&
         (es->p != NULL) &&
         (es->p->len <= tcp_sndbuf(tpcb)))
  {

    /* get pointer on pbuf from es structure */
    ptr = es->p;

    /* enqueue data for transmission */
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

    if (wr_err == ERR_OK)
    {
//      u16_t plen;
      u8_t freed;

//      plen = ptr->len;

      /* continue with next pbuf in chain (if any) */
      es->p = ptr->next;

      if(es->p != NULL)
      {
        /* increment reference count for es->p */
        pbuf_ref(es->p);
      }

     /* chop first pbuf from chain */
      do
      {
        /* try hard to free pbuf */
        freed = pbuf_free(ptr);
      }
      while(freed == 0);
     /* we can read more data now */
//     tcp_recved(tpcb, plen);
   }
   else if(wr_err == ERR_MEM)
   {
      /* we are low on memory, try later / harder, defer to poll */
     es->p = ptr;
   }
   else
   {
     /* other problem ?? */
   }
  }
}


static void tcp_client_connection_close(struct tcp_pcb *tpcb, struct tcp_client_struct *es)
{

  /* remove all callbacks */
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);

  /* delete es structure */
  if (es != NULL)
  {
    mem_free(es);
  }

  /* close tcp connection */
  tcp_close(tpcb);
}

/* Handle the incoming TCP Data */

static void tcp_client_handle (struct tcp_pcb *tpcb, struct tcp_client_struct *es)
{
	/* get the Remote IP */
	// ip4_addr_t inIP = tpcb->remote_ip;
//	uint16_t inPort = tpcb->remote_port;

	/* Extract the IP */
	// char *remIP = ipaddr_ntoa(&inIP);

//	esTx->state = es->state;
//	esTx->pcb = es->pcb;
//	esTx->p = es->p;

	esTx = es;
	pcbTx = tpcb;
}
