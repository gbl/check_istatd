/*
 * Poor man's implementation of xml. This is really inferior to libxml2, but
 * the fewer dependencies on nonstandard libraries, the better, if this is
 * supposed to run on "non-linux" OSes.
 *
 * We don't care much about memory leaks, since this is not intended to be a
 * general library, and check_istatd will exit in a second anyway.
 */

/* not even a tree structure ... */
struct xmltree {
	char	*keyword;
	char	*value;
	struct xmltree *next;
};

char *xmlheader();
struct xmltree *xmlparsefd(int fd);
char *xmlvalue(struct xmltree *tree, char *treepos);
void xmlfreetree(struct xmltree *tree);

/* and a cheap "iterator" implementation */
#define xmliterator xmltree
#define xmliteratorinit(tree) (tree)
#define xmliteratornext(iter) (iter->next)
#define xmliteratorkey(iter)  (iter->keyword)
#define xmliteratorval(iter)  (iter->value)
