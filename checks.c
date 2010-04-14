#include "xml.h"
#include "check_istatd.h"
#include <string.h>
#include <stdio.h>

extern int requestno;
static void sendreq(int conn, char *req);

void check_cpu_stats(int conn, int warn, int crit, char *param) {
	char *p, *v;
	int rc=0;
	struct xmltree *response;
	int total=0;
	char buf[]="isr/CPU/c/.";

	sendreq(conn, "<c>-1</c>");
	response=xmlparsefd(conn);

	total=0;
	for (p=param; *p; p++) {
		buf[sizeof buf - 2]=*p;
		if ((v=xmlvalue(response, buf))==NULL) {
			addcheckresult(3, 'c', p, 0, warn, crit, 0, 100);
			return;
		}
		total+=atoi(v);
	}
	if	(crit>0 && total>crit) { rc=2; }
	else if	(crit<0 && total<-crit) { rc=2; }
	else if	(warn>0 && total>warn) { rc=1; }
	else if	(warn<0 && total<-warn) { rc=1; }
	addcheckresult(rc, 'c', param, total, warn, crit, 0, 100);
	xmlfreetree(response);
}

void check_disk_stats(int conn, int warn, int crit, char *diskname) {
	char *p;
	int rc=0, value=-1;
	struct xmltree *response;
	struct xmliterator *iter;
	int founddisk=0;

	sendreq(conn, "<d>-1</d>");
	response=xmlparsefd(conn);

	for (iter=xmliteratorinit(response); iter; iter=xmliteratornext(iter)) {
		if (!strcmp(iter->keyword, "isr/DISKS/d/n"))
			founddisk=!strcmp(iter->value, diskname);

		if (!strcmp(iter->keyword, "isr/DISKS/d/p") && founddisk) {
			value=atoi(iter->value);
			break;
		}
	}

	if (value==-1) { rc=3; }
	else if	(crit>0 && value>crit) { rc=2; }
	else if	(crit<0 && value<-crit) { rc=2; }
	else if	(warn>0 && value>warn) { rc=1; }
	else if	(warn<0 && value<-warn) { rc=1; }
	addcheckresult(rc, 'd', diskname, value, warn, crit, 0, 100);
	xmlfreetree(response);
}

void check_fan_stats(int conn, int warn, int crit, char *fanname) {
	char *p;
	int rc=0, value=-1;
	struct xmltree *response;
	struct xmliterator *iter;
	int foundfan=0;

	sendreq(conn, "<f>-1</f>");
	response=xmlparsefd(conn);

	for (iter=xmliteratorinit(response); iter; iter=xmliteratornext(iter)) {
		if (!strcmp(iter->keyword, "isr/FANS/f/n"))
			foundfan=!strcmp(iter->value, fanname);

		if (!strcmp(iter->keyword, "isr/FANS/f/s") && foundfan) {
			value=atoi(iter->value);
			break;
		}
	}

	if (value==-1) { rc=3; }
	else if	(crit>0 && value>crit) { rc=2; }
	else if	(crit<0 && value<-crit) { rc=2; }
	else if	(warn>0 && value>warn) { rc=1; }
	else if	(warn<0 && value<-warn) { rc=1; }
	addcheckresult(rc, 'f', fanname, value, warn, crit, 0, -1);
	xmlfreetree(response);
}

void check_load_stats(int conn, int warn, int crit, char *param) {
	char *p, *v;
	int rc=0;
	struct xmltree *response;
	int value=-1;

	sendreq(conn, "<lo>-1</lo>");
	response=xmlparsefd(conn);
	
	if (!strcmp(param, "1"))
		v=xmlvalue(response, "isr/LOAD/one");
	else if (!strcmp(param, "5"))
		v=xmlvalue(response, "isr/LOAD/fv");
	else if (!strcmp(param, "15"))
		v=xmlvalue(response, "isr/LOAD/ff");
	if (v!=NULL)
		value=atoi(v);
	
	if (value==-1) { rc=3; }
	else if	(crit>0 && value>crit) { rc=2; }
	else if	(crit<0 && value<-crit) { rc=2; }
	else if	(warn>0 && value>warn) { rc=1; }
	else if	(warn<0 && value<-warn) { rc=1; }
	addcheckresult(rc, 'l', param, value, warn, crit, 0, -1);
	xmlfreetree(response);
}

void check_memory_stats(int conn, int warn, int crit, char *param) {
	int rc=0;
	struct xmltree *response;
	int total=0, temp;
	char buf[80], parmbuf[80], *p, *r;

	sendreq(conn, "<m>-1</m>");
	response=xmlparsefd(conn);

	strcpy(parmbuf, param);
	p=strtok(parmbuf, ",");
	while (p) {
		strcpy(buf, "isr/MEM/");
		strcat(buf, p);
		if ((r=xmlvalue(response, buf))==NULL) {
			addcheckresult(3, 'm', p, 0, warn, crit, 0, -1);
			return;
		}
		temp=atoi(r);
		total+=temp;
		p=strtok(NULL, ",");
	}

	if	(crit>0 && total>crit) { rc=2; }
	else if	(crit<0 && total<-crit) { rc=2; }
	else if	(warn>0 && total>warn) { rc=1; }
	else if	(warn<0 && total<-warn) { rc=1; }
	addcheckresult(rc, 'm', param, total, warn, crit, 0, 100);
	xmlfreetree(response);
}

void check_network_stats(int conn, int warn, int crit, char *param) {
	int rc=0;
	struct xmltree *response;
	long long thisd=0, thisu=0, firstd=0, firstu=0, this, first;
	int thist=0, firstt=0;
	int value;
	struct xmliterator *iter;

	sendreq(conn, "<n>1</n>");
	response=xmlparsefd(conn);

	for (iter=xmliteratorinit(response); iter; iter=xmliteratornext(iter)) {
		if (!strcmp(iter->keyword, "isr/NET/n/d")) {
			if (firstd==0)	firstd=atoll(iter->value);
					thisd =atoll(iter->value);
		}
		if (!strcmp(iter->keyword, "isr/NET/n/u")) {
			if (firstu==0)	firstu=atoll(iter->value);
					thisu =atoll(iter->value);
		}
		if (!strcmp(iter->keyword, "isr/NET/n/t")) {
			if (firstt==0)	firstt=atoll(iter->value);
					thist =atoll(iter->value);
		}
	}

	if (*param=='d')	{ this=thisd; first=firstd; }
	else if (*param=='u')	{ this=thisu; first=firstu; }
	else			{ thist=0;  /* just to force error */ }
	if (thist==0 || firstt==0 || thist==firstt ) {
		addcheckresult(rc, 'n', param, -1, warn, crit, 0, 100);
	}
	value=(this-first)/(thist-firstt);

	if	(crit>0 && value>crit) { rc=2; }
	else if	(crit<0 && value<-crit) { rc=2; }
	else if	(warn>0 && value>warn) { rc=1; }
	else if	(warn<0 && value<-warn) { rc=1; }
	addcheckresult(rc, 'n', param, value, warn, crit, 0, -1);
	xmlfreetree(response);
}

void check_temp_stats(int conn, int warn, int crit, char *tempname) {
	char *p;
	int rc=0, value=-1;
	struct xmltree *response;
	struct xmliterator *iter;
	int foundtemp=0;

	sendreq(conn, "<t>1</t>");
	response=xmlparsefd(conn);

	for (iter=xmliteratorinit(response); iter; iter=xmliteratornext(iter)) {
		if (!strcmp(iter->keyword, "isr/TEMPS/t/n"))
			foundtemp=!strcmp(iter->value, tempname);

		if (!strcmp(iter->keyword, "isr/TEMPS/t/t") && foundtemp) {
			value=atoi(iter->value);
			break;
		}
	}

	if (value==-1) { rc=3; }
	else if	(crit>0 && value>crit) { rc=2; }
	else if	(crit<0 && value<-crit) { rc=2; }
	else if	(warn>0 && value>warn) { rc=1; }
	else if	(warn<0 && value<-warn) { rc=1; }
	addcheckresult(rc, 't', tempname, value, warn, crit, 0, -1);
	xmlfreetree(response);
}

void check_uptime(int conn, int warn, int crit) {
	char *p, *v;
	int rc=0;
	struct xmltree *response;
	int value=-1;

	sendreq(conn, "<u></u>");
	response=xmlparsefd(conn);
	v=xmlvalue(response, "isr/UPT/u");
	if (v!=NULL)
		value=atoi(v);
	
	if (value==-1) { rc=3; }
	else if	(crit>0 && value>crit) { rc=2; }
	else if	(crit<0 && value<-crit) { rc=2; }
	else if	(warn>0 && value>warn) { rc=1; }
	else if	(warn<0 && value<-warn) { rc=1; }
	addcheckresult(rc, 'u', "", value, warn, crit, 0, -1);
	xmlfreetree(response);
}

static void sendreq(int conn, char *req) {
	char buf[512];
	sprintf(buf, "%s<isr><rid>%d</rid>%s</isr>", xmlheader(),
		++requestno, req);
	if (verbose>1) {
		printf("sending %s\n", buf);
	}
	write(conn, buf, strlen(buf));
}
