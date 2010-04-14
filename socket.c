#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "xml.h"

extern int verbose;

/*
 * Initialize a connection and send our passcode if neccesary.
 * Returns a socket, or a (negative) error code.
 *
 * -1 generic error
 * -2 if the server refuses our connection
 * -3 if the server doesn't speak XML
 * -4 if the server rejects our password
 */

int initconn(char *hostname, int port, char *duuid, char *password) {
	int sock;
	struct sockaddr_in server;
	struct hostent *host;
	char myhostname[80];
	char buf[512];
	struct xmltree *response;
	char *ath;

	if ((host=gethostbyname(hostname))==NULL) {
		if (verbose)
			herror("gethostbyname");
		return -1;
	}
	if ((sock=socket(AF_INET, SOCK_STREAM, 0))<0) {
		if (verbose)
			perror("socket");
		return -1;
	}
	server.sin_family=AF_INET;
	memcpy(&server.sin_addr, host->h_addr_list[0], sizeof server.sin_addr);
	server.sin_port=htons(port);
	if (connect(sock, (struct sockaddr *)&server, sizeof server)<0) {
		if (verbose)
			perror("connect");
		return -2;
	}

	/* TODO: make it a bit more unique - my ip, server ip, server port */
	if (duuid==NULL) {
		duuid="nagios_check_istatd_duuid";
	}
	gethostname(myhostname, sizeof myhostname);
	myhostname[sizeof myhostname - 1]='\0';
	sprintf(buf, "%s<isr><h>%s</h><duuid>%s</duuid></isr>",
		xmlheader(), myhostname, duuid);
	write(sock, buf, strlen(buf));
	if ((response=xmlparsefd(sock))==NULL) {
		close(sock);
		return -3;
	}
	if ((ath=xmlvalue(response, "isr/ath"))==NULL) {
		xmlfreetree(response);
		close(sock);
		return -1;
	} else if (atoi(ath)==0) {
		xmlfreetree(response);
		return sock;
	} else {
		xmlfreetree(response);
		write(sock, password, strlen(password));
		if ((response=xmlparsefd(sock))==NULL) {
			close(sock);
			return -1;
		}
		if ((ath=xmlvalue(response, "isr/athrej"))!=NULL) {
			close(sock);
			return -4;
		}
		if ((ath=xmlvalue(response, "isr/ready"))==NULL
		||   atoi(ath)!=1) {
			close(sock);
			return -1;
		}
		xmlfreetree(response);
		return sock;
	}
}
