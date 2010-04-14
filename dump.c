#include <stdio.h>
#include <string.h>
#include "xml.h"

#define	PRINTHELP(x, y) { printf("%-20s: %s\n", x, y); }
#define	HELP(k, x) { if (!strcmp(key, k)) PRINTHELP(x, val); }

void dumpparams(int conn) {
	char buf[512];
	struct xmltree *response;
	struct xmliterator *iterator;
	char *key, *val;
	char *name;

	sprintf(buf, "%s<isr><rid>1</rid><c>-1</c><n>-1</n><m></m><lo></lo><t>1</t><f>1</f><u></u><d>1</d></isr>", xmlheader());
	write(conn, buf, strlen(buf));
	response=xmlparsefd(conn);

	iterator=xmliteratorinit(response);
	while (iterator) {
		key=xmliteratorkey(iterator);
		val=xmliteratorval(iterator);

		HELP("isr/CPU/c/u",	"-cpu u (user)");
		HELP("isr/CPU/c/s",	"-cpu s (sys)");
		HELP("isr/CPU/c/n",	"-cpu n (nice)");
		HELP("isr/NET/n/d",	"-net d (download)");
		HELP("isr/NET/n/u",	"-net u (upload)");
		HELP("isr/MEM/t",	"-mem t (total)");
		HELP("isr/MEM/w",	"-mem w (cached)");
		HELP("isr/MEM/a",	"-mem a (available)");
		HELP("isr/MEM/i",	"-mem i (inactive)");
		HELP("isr/MEM/f",	"-mem f (free)");
		HELP("isr/MEM/su",	"-mem su (swapuse)");
		HELP("isr/MEM/st",	"-mem st (swaptotal)");
		HELP("isr/MEM/pi",	"-mem pi (page in)");
		HELP("isr/MEM/po",	"-mem po (page out)");
		HELP("isr/LOAD/one",	"-load 1");
		HELP("isr/LOAD/fv",	"-load 5");
		HELP("isr/LOAD/ff",	"-load 15");
		if (!strcmp(key, "isr/TEMPS/t/n")
		||  !strcmp(key, "isr/FANS/f/n")
		||  !strcmp(key, "isr/DISKS/d/n"))
			name=val;
		if (!strcmp(key, "isr/TEMPS/t/t")) {
			sprintf(buf, "-temp '%s'", name);
			PRINTHELP(buf, val);
		}
		if (!strcmp(key, "isr/FANS/f/s")) {
			sprintf(buf, "-fan '%s'", name);
			PRINTHELP(buf, val);
		}
		if (!strcmp(key, "isr/DISKS/d/p")) {
			sprintf(buf, "-disk '%s'", name);
			PRINTHELP(buf, val);
		}

		iterator=xmliteratornext(iterator);
	}

}
