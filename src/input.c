#include "c.h"

static char rcsid[] = "$Id$";

static void pragma(void);
static void resynch(void);
static char *pragma_string(void);

static int bsize;
static unsigned char buffer[MAXLINE+1 + BUFSIZE+1];
unsigned char *cp;	/* current input character */
char *file;		/* current input file name */
char *firstfile;	/* first input file */
unsigned char *limit;	/* points to last character + 1 */
char *line;		/* current line */
int lineno;		/* line number of current line */

void nextline(void) {
	do {
		if (cp >= limit) {
			fillbuf();
			if (cp >= limit)
				cp = limit;
			if (cp == limit)
				return;
		} else {
			lineno++;
			for (line = (char *)cp; *cp==' ' || *cp=='\t'; cp++)
				;
			if (*cp == '#') {
				resynch();
				nextline();
			}
		}
	} while (*cp == '\n' && cp == limit);
}
void fillbuf(void) {
	if (bsize == 0)
		return;
	if (cp >= limit)
		cp = &buffer[MAXLINE+1];
	else
		{
			int n = limit - cp;
			unsigned char *s = &buffer[MAXLINE+1] - n;
			assert(s >= buffer);
			line = (char *)s - ((char *)cp - line);
			while (cp < limit)
				*s++ = *cp++;
			cp = &buffer[MAXLINE+1] - n;
		}
	if (feof(stdin))
		bsize = 0;
	else
		bsize = fread(&buffer[MAXLINE+1], 1, BUFSIZE, stdin);
	if (bsize < 0) {
		error("read error\n");
		exit(EXIT_FAILURE);
	}
	limit = &buffer[MAXLINE+1+bsize];
	*limit = '\n';
}
void input_init(int argc, char *argv[]) {
	static int inited;

	if (inited)
		return;
	inited = 1;
	main_init(argc, argv);
	limit = cp = &buffer[MAXLINE+1];
	bsize = -1;
	lineno = 0;
	file = NULL;
	fillbuf();
	if (cp >= limit)
		cp = limit;
	nextline();
}

/* ident - handle #ident "string" */
static void ident(void) {
	while (*cp != '\n' && *cp != '\0')
		cp++;
}

/* A pragma's strings are metadata, not C expressions.  Read them directly so
 * the C lexer does not join the two adjacent string literals as it normally
 * must for an expression. */
static char *pragma_string(void) {
	char *start;

	while (*cp == ' ' || *cp == '\t')
		cp++;
	if (*cp != '"') {
		error("expected string in `#pragma vig import'\n");
		return NULL;
	}
	start = (char *)++cp;
	while (*cp != '"' && *cp != '\n' && *cp != '\0')
		cp++;
	if (*cp != '"') {
		error("missing closing quote in `#pragma vig import'\n");
		return NULL;
	}
	return stringn(start, (char *)cp++ - start);
}

/* pragma - handle #pragma ref id... and target-specific directives. */
static void pragma(void) {
	if ((t = gettok()) == ID && strcmp(token, "ref") == 0) {
		for (;;) {
			while (*cp == ' ' || *cp == '\t')
				cp++;
			if (*cp == '\n' || *cp == 0)
				break;
			if ((t = gettok()) == ID && tsym) {
				tsym->ref++;
				use(tsym, src);
			}
		}
	}
	else if (t == ID && strcmp(token, "vig") == 0) {
		char *name, *library, *symbol, *signature = NULL;

		if ((t = gettok()) != ID || strcmp(token, "import") != 0) {
			error("expected `import' after `#pragma vig'\n");
			return;
		}
		if ((t = gettok()) != ID) {
			error("expected C identifier in `#pragma vig import'\n");
			return;
		}
		name = string(token);
		if ((t = gettok()) != ',') {
			error("expected comma after C identifier in `#pragma vig import'\n");
			return;
		}
		library = pragma_string();
		if (library == NULL)
			return;
		while (*cp == ' ' || *cp == '\t')
			cp++;
		if (*cp != ',') {
			error("expected comma after library string in `#pragma vig import'\n");
			return;
		}
		cp++;
		symbol = pragma_string();
		if (symbol == NULL)
			return;
		while (*cp == ' ' || *cp == '\t')
			cp++;
		/* An optional fourth string is the VIG64 signature. Its first word is
		 * the result type and the remaining words are argument types. It lets a
		 * C declaration distinguish a guest buffer pointer from an opaque host
		 * handle, which the C type system cannot express by itself. */
		if (*cp == ',') {
			cp++;
			signature = pragma_string();
			if (signature == NULL)
				return;
		}
		if (IR->foreign_import == NULL)
			error("`#pragma vig import' is only supported by the VIG target\n");
		else
			(*IR->foreign_import)(name, library, symbol, signature);
	}
}

/* resynch - set line number/file name in # n [ "file" ], #pragma, etc. */
static void resynch(void) {
	for (cp++; *cp == ' ' || *cp == '\t'; )
		cp++;
	if (limit - cp < MAXLINE)
		fillbuf();
	if (strncmp((char *)cp, "pragma", 6) == 0) {
		cp += 6;
		pragma();
	} else if (strncmp((char *)cp, "ident", 5) == 0) {
		cp += 5;
		ident();
	} else if (*cp >= '0' && *cp <= '9') {
	line:	for (lineno = 0; *cp >= '0' && *cp <= '9'; )
			lineno = 10*lineno + *cp++ - '0';
		lineno--;
		while (*cp == ' ' || *cp == '\t')
			cp++;
		if (*cp == '"') {
			file = (char *)++cp;
			while (*cp && *cp != '"' && *cp != '\n')
				cp++;
			file = stringn(file, (char *)cp - file);
			if (*cp == '\n')
				warning("missing \" in preprocessor line\n");
			if (firstfile == 0)
				firstfile = file;
		}
	} else if (strncmp((char *)cp, "line", 4) == 0) {
		for (cp += 4; *cp == ' ' || *cp == '\t'; )
			cp++;
		if (*cp >= '0' && *cp <= '9')
			goto line;
		if (Aflag >= 2)
			warning("unrecognized control line\n");
	} else if (Aflag >= 2 && *cp != '\n')
		warning("unrecognized control line\n");
	while (*cp)
		if (*cp++ == '\n')
			if (cp == limit + 1) {
				nextline();
				if (cp == limit)
					break;
			} else
				break;
}

