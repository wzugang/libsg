#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kcp_server.h"

enum
{
    PT_LS,
    PT_GET,
    PT_PUT,
    PT_CHK,
    PT_BYE,
    PT_HELP,
};

typedef struct
{
    int payload;
    int len;
    char data[1024];
}ftp_t;

static void s_kcp_on_open(sg_kcp_client_t *client)
{
    char * addr = NULL;
    /*printf("s_kcp_on_open\n");*/

    addr = sg_kcp_server_get_client_addr(client);

    printf("conn from %s\n", addr);

    free(addr);
}

static void s_kcp_on_message(sg_kcp_client_t *client, char *data, size_t size)
{
    ftp_t * input = (ftp_t *)data;
    ftp_t output;
    FILE * fp = NULL;

    switch (input->payload)
    {
        case PT_GET:
            printf("GET %s\n", input->data);
            fp = fopen(input->data, "rb");
            if (NULL == fp)
            {
                printf("open %s failed\n", input->data);
                return;
            }

            output.payload = PT_CHK;
            while ((output.len = fread(output.data, 1, 1024, fp)) > 0)
            {
                sg_kcp_server_send_data(client, &output, output.len + 2*sizeof(int));
            }
            output.payload = PT_BYE;
            output.len = 0;
            sg_kcp_server_send_data(client, &output, output.len + 2*sizeof(int));

            sg_kcp_server_close_client(client);

            fclose(fp);
            break;
        case PT_BYE:
            sg_kcp_server_close_client(client);
            break;
    }    
}

static void s_kcp_on_close(sg_kcp_client_t *client, int code, const char *reason)
{
    char * addr = NULL;

    addr = sg_kcp_server_get_client_addr(client);

    printf("close conn from %s\n", addr);

    free(addr);
}


int main(int argc, char * argv[])
{
    char sport[16]  = {0};
    int  port;
	
	if (argc < 2)
	{
		printf("%s port\n", argv[0]);
		return 0;
	}

    if (argc > 1) strncpy(sport, argv[1], sizeof(sport));
    sscanf(sport, "%d", &port);
	
	printf("listen @ %d\n", port);
    
    sg_kcp_server_t * server = sg_kcp_server_open("0.0.0.0", port, 1, 100, s_kcp_on_open, s_kcp_on_message, s_kcp_on_close);

    sg_kcp_server_run(server, 10);

    sg_kcp_server_close(server);

    return 0;
}

