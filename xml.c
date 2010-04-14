#include "xml.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/select.h"

#define ERROR(x) { fputs(x, stderr); return NULL; }
#define ERROR2(x) { fputs(x, stderr); fprintf(stderr, "buf=%s pos=%d\n", buf, pos); return NULL; }

extern int verbose;

char *xmlheader() {
	return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
}


/* This makes a lot of assumptions that work while talking to istatd, but
 * make the code completely unusable for any other purpose. We assume that
 * a single tag will never be longer than 1024 bytes, that tags don't nest
 * more than 20 levels, and we don't free memory correctly if anything goes
 * wrong. Also, we assume there's only tags with values, no CDATA at all.
 */

struct xmltree *xmlparsefd(int fd) {
	struct xmltree *tree=NULL, *node, *prev;
	char buf[1024];
	int pos, end, parsed=0, len=0;
	int level=0;
	char *stack[20];		/* assume no more than 20 levels;
					   for istatd, this is sufficient */
	char nodename[80];
	int i, nnpos, nnpos2;
	fd_set rfds, wfds, efds;
	struct timeval timeout;

	for (;;) {
read_more:
		/* At this point, assume len to contain the length of valid
		 * data within buf. parsed contains the number of bytes
		 * that have already been added to the tree.
		 * If we have already parsed something, move the stuff behind
		 * it to the beginning of the buffer. Then, try to read more.
		 * Wait at most 5 seconds for data to become available. */
		if (parsed>0) {
			memmove(buf, buf+parsed, len-parsed);
			len-=parsed;
			parsed=0;
		}
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);
		FD_SET(fd, &rfds);
		timeout.tv_sec=5;
		timeout.tv_usec=0;
		if (select(fd+1, &rfds, &wfds, &efds, &timeout)<0
		||  !FD_ISSET(fd, &rfds))
			ERROR("select timeout\n");
		pos=len;
		if ((len=read(fd, buf+pos, sizeof buf - pos))<=0)
			ERROR("read returned <=0\n");
		/* Set len so it points to the end of the current valid buffer,
		 * and pos to the point we're parsing from. parsed marks the
		 * end of what we've parsed successfully. */
		len+=pos;
		pos=0;
		parsed=0;
		/*
		 * Now, our buffer starts with the XML header (<? ... ?>), a
		 * tag, or an end tag. (It could also start with cdata, but not
		 * while talking to istatd). Ignore the header; collect the
		 * tag and put it to the stack, or check the end tag and pop
		 * the stack.
		 */
		while (pos<len) {
			if (buf[pos]!='<') {
				ERROR2("< expected\n");
			}
			if (buf[pos+1] == '?') {
				/* read to the end of the xml header tag */
				pos+=2;
				while (pos<len-1
					&& (buf[pos]!='?' || buf[pos+1] != '>'))
					pos++;
				/* If we can't find the tag end, read more. */
				if (pos>=len-1) {
					goto read_more;
				}
				if (verbose>1)
					printf("parsed %-*.*s\n", pos+2-parsed, pos+2-parsed, buf+parsed);
				parsed=pos=pos+2;
			}
			else if (buf[pos+1] == '/' && level>0) {
				/* parse tag end */
				end=pos+1;
				while (end<len && buf[end]!='>')
					end++;
				if (end>=len)
					goto read_more;
				/* does closing tag match opening? */
				if (memcmp(stack[level-1], buf+pos+2, end-pos-2)
				||  stack[level-1][end-pos-2]!='/') 
					ERROR2("wrong closing tag\n");
				if (verbose>1)
					printf("parsed %-*.*s\n", end+1-parsed, end+1-parsed, buf+parsed);
				parsed=pos=end+1;
				--level;
				/* No more data and everything closed? finish!*/
				if (parsed==len && level==0) {
					return tree;
				}
			} else if (buf[pos+1] == '/') {
				ERROR2("end tag where none was expected\n");
			} else {
				/* parse a tag. First find out if we have
				 * everything up to the closing tag. */
				pos++;
				end=pos;
				/* Another bug if we're not with istatd - 
				 * what about tag end markers in quotes? */
				while (end<len && buf[end]!='>')
					end++;
				if (end==len)
					goto read_more;
				/* Look for the end of the current tag name.
				 * This can't reach len as '>' isn't alnum. */
				end=pos;
				while (isalnum(buf[end]))
					end++;

				/* Build the current node name from the node
				 * name stack. */
				nnpos=0;
				for (i=0; i<level; i++) {
					strcpy(nodename+nnpos, stack[i]);
					nnpos+=strlen(nodename+nnpos);
				}
				memcpy(nodename+nnpos, buf+pos, end-pos);
				nnpos2=nnpos;
				nnpos+=end-pos;
				nodename[nnpos++]='/';
				nodename[nnpos]='\0';
				/* Now we have the complete node name in nnpos,
				 * and nnpos2 tells us where the current tag
				 * name begins.*/
				if ((node=malloc(sizeof(struct xmltree)))==NULL)
					ERROR("malloc returns 0");
				node->next=NULL;
				if (tree==NULL)
					tree=node;
				else
					prev->next=node;
				prev=node;
				if ((node->keyword=strdup(nodename))==NULL)
					ERROR("strdup returns 0");
				/* By setting the stack to within node->keyword,
				 * we make sure it doesn't get overwritten
				 * until we are finished. */
				stack[level++]=node->keyword+nnpos2;
				node->value=NULL;
				/* Find key="value" pairs, or tag end. */
				pos=end;
				while (pos<len && buf[pos]!='>') {
					if (isspace(buf[pos]))
						pos++;
					else if (buf[pos]=='/'
					     && buf[pos+1]=='>') {
					     	level--;
						pos++;
					     	break; /* next tag */
					} else {
						/* find end of tag name */
						end=pos;
						while (end<len
						    && isalnum(buf[end]))
							end++;
						/* must continue with =" */
						if (end>=len
						||  buf[end]!='='
						||  buf[++end]!='"')
							ERROR2("tag value doesn't end in =\"\n");
						if ((node=malloc(sizeof(struct xmltree)))==NULL)
							ERROR("malloc returns NULL\n");
						prev->next=node;
						prev=node;
						node->next=NULL;
						if ((node->keyword=malloc(nnpos+end-pos+1))==NULL)
							ERROR("malloc returns NULL\n");
						memcpy(node->keyword, nodename, nnpos);
						memcpy(node->keyword+nnpos, buf+pos, end-pos-1);
						node->keyword[nnpos+end-pos-1]='\0';
						++end; pos=end;
						while (end<len
						    && buf[end]!='"')
						    	end++;
						if (end>=len)
							ERROR2("no \" found at end of value\n");
						if ((node->value=malloc(end-pos+1))==NULL)
							ERROR("malloc returns NULL\n");
						memcpy(node->value, buf+pos, end-pos);
						node->value[end-pos]='\0';
						pos=end+1;
					}
				}
				if (verbose>1)
					printf("parsed %-*.*s\n", pos-parsed+1, pos-parsed+1, buf+parsed);
				parsed=++pos;
			} /* end parse tag */
		} /* end buffer contains another tag */
	} /* end still data to read */
}

char *xmlvalue(struct xmltree *tree, char *key) {
	while (tree) {
		if (!strcmp(tree->keyword, key))
			return tree->value;
		tree=tree->next;
	}
	return NULL;
}

void xmlfreetree(struct xmltree *tree) {
	struct xmltree *next;

	while (tree) {
		next=tree->next;
		free(tree->keyword);
		free(tree->value);
		tree=next;
	}
}
