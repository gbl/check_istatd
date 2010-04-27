#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check_istatd.h"

char *progname;	/* our name, for error output */
int verbose;	/* verbosely tell what we're doing, for debug reasons */
int exitcode;
int requestno;

struct checkresult {
	int rc;
	char checktype;
	char *checkoption;
	int value, warn, crit, min, max;
	struct checkresult *next;
} *head;

char *usage=
"Usage: %s -W warnlevel -C critlevel -H hostname -I password [-P port]\n"
"	[-D duuid] [-v]\n"
"	[-cpu {u}{s}{n}]\n"
"	[-disk mountpoint]\n"
"	[-fan fan-name]\n"
"	[-network {d|u}]\n"
"	[-memory {w,a,i,f,t,su,st,pi,po}]\n"
"	[-load {1|5|15}]\n"
"	[-temp temp-name]\n"
"	[-uptime]\n"
"	[-paramlist]\n"
;

char *usage2=
"\n"
"The -H and -I Options must be given, and may appear only once.\n"
"-W and -C may appear in front of each individual test.\n"
"\n"
"-cpu compares the sum of the user, system, and nice times, depending on the\n"
"	letter options you give, to the preceding warn and crit levels, i.e.\n"
"	-W 80 -C 90 -cpu u	warns if user > 80%%, crit if >90%%\n"
"	-W 100 -C 100 -cpu usn	warns (and crits) if user+system+nice =100%%\n"
"\n"
"-disk compares the percentage of disk usage, i.e.\n"
"	-W 90 -C 95 -disk /home	warns if /home reaches 90%% usage, crit if 95%%\n"
"\n"
"-fan tests for fan speed\n"
"	-W -1500 -C -1200 -fan 'CPU Fan'\n"
"\n"
"-network checks the number of BYTES (not bits) sent(u, upload) or\n"
"	received(d, download) PER SECOND.\n"
"	-W 10000000 -C 11000000 -network u are good values to check if a\n"
"	100 Mbit connection is near its upload limits.\n"
"\n"
"-memory adds the values of total(t), free(f), active(a),\n"
"	inactive(i), swapuse(su) and cached(w) memory, i.e.\n"
"	-W -1000 -C -1000 -mem f,c\n"
"				warn if the sum of free+cached memory drops\n"
"				below 1 MB\n"
"\n"
"-load checks the cpu load\n"
"\n"
"-temp tests a system temperature sensor\n"
"	-W 40 -C 50 -temp 'M/B Temp'\n"
"\n"
"The tag names for -disk, -fan and -temp depend on the istatd configuration.\n"
"Use 'check_istatd -H host -I password -p' to see what your istatd supports.\n"
"\n"
"Several checks can be put together, like in\n"
"-W 3 -C 5 -load 1 5 -W 2 -C 3 -load 15 -W 10000000 -C 11000000 -net u d\n"
;



int main(int argc, char **argv) {

	/* command line parameters */
	int warn=0, crit=0;
	char *hostname=NULL;
	int hostport=5109;
	char *duuid=NULL;
	char *password=NULL;

	int err;
	int conn=-1;
	char checktype='\0';
	struct checkresult *result;

	/* No getopt here. The syntax allows for too many specialties,
	 * and some OSes like IRIX don't have it, especially not the long
	 * version.
	 * Upper case options set generic parameters
	 * (like many nagios plugins), lower case letters specify what should
	 * be checked next.
	 */

	progname=argv[0];
	while (--argc && *++argv!=NULL) {
		if (**argv=='-') {
			switch (*++*argv) {
			/* allow long options by ignoring multiple dashes. This
			 * assumes long option names have their first letter
			 * identical to short ones. */
			case '-':
				while (**argv=='-') *argv++;
				/* and fall through! */
			case 'c': /* cpu */
			case 'd': /* disk */
			case 'f': /* fan */
			case 'l': /* load ('lo' in xml) */
			case 'm': /* memory */
			case 'n': /* network */
			case 't': /* temperature */
				checktype=**argv;
				continue;

			/* Uptime and paramlist need a hack as they don't have
			 * parameters. Adjust argc and argv so the keywords
			 * without the dash look like a parameter next time. */
			case 'p':
			case 'u': /* uptime */
				checktype=**argv;
				++argc; --argv;
				continue;

			case 'W': warn=atoi(*++argv); --argc; continue;
			case 'C': crit=atoi(*++argv); --argc; continue;
			case 'H': hostname=*++argv; --argc; continue;
			case 'P': hostport=atoi(*++argv); --argc; continue;
			case 'D': duuid=*++argv; --argc; continue;
			case 'I': password=*++argv; --argc; continue;
			case 'v': verbose++; continue;

			case 'h':
				fprintf(stderr, usage, progname);
				fprintf(stderr, usage2);
				exit(3);

			default:
				fprintf(stderr, usage, progname);
				exit(3);
			}
		}
		/* We have a non-option argument. At this point, the
		 * host name, warn level, crit level, and password should all
		 * have been set. If we have not been given a duuid, we'll
		 * invent one ourselves later.
		 */
		err=0;
		if (warn==0 && checktype!='p')
			err=fprintf(stderr, "No warning level given.\n");
		if (crit==0 && checktype!='p')
			err=fprintf(stderr, "No crit level given.\n");
		if (hostname==NULL)
			err=fprintf(stderr, "No host name given.\n");
		if (password==NULL)
			err=fprintf(stderr, "No password given.\n");
		if (checktype=='\0') {
			err=fprintf(stderr, "no check option given.\n");
		}
		if (err) {
			fprintf(stderr, usage, progname);
			exit(3);
		}
		if (conn==-1) {
			if ((conn=initconn(hostname, hostport,
						duuid, password))<0) {
				switch(conn) {
				case -2:
					fprintf(stderr,
						"istat server refuses socket "
						"connections\n");
					break;
				case -3:
					fprintf(stderr,
						"istat server doesn't speak "
						"XML\n");
					break;
				case -4:
					fprintf(stderr,
						"istat server rejects our "
						"password\n");
					break;
				default:
					fprintf(stderr,
						"Couldn't connect to istat "
						"server on host %s port %d\n",
						hostname, hostport);
					break;
				}
				exit(3);
			}
		}
		switch(checktype) {
		case 'c':
			check_cpu_stats(conn, warn, crit, *argv);
			break;
		case 'd':
			check_disk_stats(conn, warn, crit, *argv);
			break;
		case 'f':
			check_fan_stats(conn, warn, crit, *argv);
			break;
		case 'l':
			check_load_stats(conn, warn, crit, *argv);
			break;
		case 'm':
			check_memory_stats(conn, warn, crit, *argv);
			break;
		case 'n':
			check_network_stats(conn, warn, crit, *argv);
			break;
		case 't':
			check_temp_stats(conn, warn, crit, *argv);
			break;
		case 'u':
			check_uptime(conn, warn, crit);
			break;
		case 'p':
			dumpparams(conn);
			exit(3);
		}
	}

	if (head==0) {
		printf("%s UNKNOWN - no checks at all\n", progname);
		exit(3);
	}

	switch(exitcode) {
		case 0: printf("%s OK - no problems found.", progname);
			break;
		default:
			printf("%s %s - ", progname,
					(exitcode==1 ? "WARNING" :
					 exitcode==2 ? "CRITICAL" :
					 "UNKNOWN"));
			for (result=head; result; result=result->next)
				if (result->rc!=0)
					printf("%c(%s)=%ld ", result->checktype,
						result->checkoption,
						result->value);
	}
	putchar('|');
	for (result=head; result; result=result->next) {
		printf("'%c(%s)'=%ld;%ld;%ld",
			result->checktype, result->checkoption,
			result->value, result->warn, result->crit);
		if (result->min!=-1 || result->max!=-1)
			putchar(';');
		if (result->min!=-1)
			printf("%ld", result->min);
		if (result->max!=-1) {
			printf(";%ld", result->max);
		}
		putchar(' ');
	}
	putchar('\n');
	exit(exitcode);
}

void addcheckresult(int rc, char type, char *param, int total,
					int warn, int crit, int min, int max) {

	struct checkresult *res=malloc(sizeof(struct checkresult)), *temp;
	res->rc=rc;
	res->checktype=type;
	res->checkoption=strdup(param);
	res->value=total;
	res->warn=warn;
	res->crit=crit;
	res->min=min;
	res->max=max;
	res->next=NULL;

	if (head==NULL)
		head=res;
	else {
		temp=head;
		while (temp->next)
			temp=temp->next;
		temp->next=res;
	}
	if (rc>exitcode)
		exitcode=rc;
}
