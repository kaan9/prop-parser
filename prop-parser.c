#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct P P;
typedef struct S S;

struct S {
	enum { S_ID, S_P } type;
	union {
		char *id;
		P *P_child;
	} u;
};

struct P {
	enum { P_S, P_neg, P_and, P_or, P_impl } type;
	union {
		S *S_child;	/* for P_S and P_neg */
		struct {
			S *l_child;
			S *r_child;
		} S_pair;	/* for P_and, P_or, P_impl */
	} u;
};

/* current token */
struct {
	enum token_type { ID, LPAREN, RPAREN, LNEG, LAND, LOR, LIMPL, NONE } type;
	char *id;	/* only used when type == ID, must be copied */
} token;

/* fetches next token, puts it into 'token', does not free id */
void next(void)
{
	static int i = 0;
	enum token_type tokens[] = { LPAREN, LPAREN, ID, LIMPL, ID, RPAREN, LIMPL, ID, RPAREN, LOR,
		LPAREN, ID, LAND, LPAREN, ID, LIMPL, ID, RPAREN, RPAREN, NONE };
	char *id = "p";
	token.id = id;
	token.type = tokens[i++];
}

void *kalloc(size_t size)
{
	void *p;

	if (size == 0) {
		return NULL;
	}
	if ((p = malloc(size)) == NULL) {
		perror("prop-parse");
		abort();
	}
	return p;
}

char *kstrdup(const char *s)
{
	char *p;
	if ((p = strdup(s)) == NULL) {
		perror("prop-parse");
		abort();
	}
	return p;
}

P *parse_P();

/* parse a subproposition */
/* assumes a valid token has been loaded */
S *parse_S(void)
{
	S *s = kalloc(sizeof(*s));	
	
	if (token.type == ID) {
		s->type = S_ID;
		s->u.id = kstrdup(token.id);
		next();
	} else if (token.type == LPAREN) {
		s->type = S_P;
		next();
		s->u.P_child = parse_P();
		if (token.type != RPAREN) {
			fprintf(stderr, "Error, expected ')'\n");
			abort();
		}
		next();
	} else {
		fprintf(stderr, "Error, unexpected token when parsing S\n");
		abort();
	}
	return s;
}

/* parse a proposition */
P *parse_P(void)
{
	P *p = kalloc(sizeof(*p));

	if (token.type == LNEG) {
		p->type = P_neg;
		next();
		p->u.S_child = parse_S();
	} else {
		p->u.S_pair.l_child = parse_S();

		switch (token.type) {
		case LAND:
			p->type = P_and;
			break;
		case LOR:
			p->type = P_or;
			break;
		case LIMPL:
			p->type = P_impl;
			break;
		default: /* P -> S */
			p->type = P_S;
			p->u.S_child = p->u.S_pair.l_child;
			return p;
		}
		next();
		p->u.S_pair.r_child = parse_S();
	}
	return p;
}

void free_P(P *p);

void free_S(S *s)
{
	switch (s->type) {
	case S_ID:
		free(s->u.id);
		break;
	case S_P:
		free_P(s->u.P_child);
		break;
	default:
		fprintf(stderr, "Error freeing invalid S node\n");
		abort();
	}
	free(s);
}

void free_P(P *p)
{
	switch (p->type) {
	case P_S:
	case P_neg:
		free(p->u.S_child);
		break;
	case P_and:
	case P_or:
	case P_impl:
		free(p->u.S_pair.l_child);
		free(p->u.S_pair.r_child);
		break;
	default:
		fprintf(stderr, "Error freeing invalid P node\n");
		abort();
	}
	free(p);
}

void print_P(P *p, int level);

void print_S(S *s, int level)
{
	for (int i = 0; i < level; i++)
		putchar('\t');
	switch (s->type) {
	case S_ID:
		printf("S: ID %s\n", s->u.id);
		break;
	case S_P:
		printf("S: ( P )\n");
		print_P(s->u.P_child, level + 1);
		break;
	default:
		fprintf(stderr, "Error printing invalid S node\n");
		abort();
	}
}


void print_P(P *p, int level)
{
	for (int i = 0; i < level; i++)
		putchar('\t');
	switch (p->type) {
	case P_S:
		printf("P: S\n");
		print_S(p->u.S_child, level + 1);
		break;
	case P_neg:
		printf("P: ¬S\n");
		print_S(p->u.S_child, level + 1);
		break;
	case P_and:
		printf("P: S_left ∧ S_right\n");
		print_S(p->u.S_pair.l_child, level + 1);
		print_S(p->u.S_pair.r_child, level + 1);
		break;
	case P_or:
		printf("P: S_left ∨ S_right\n");
		print_S(p->u.S_pair.l_child, level + 1);
		print_S(p->u.S_pair.r_child, level + 1);
		break;
	case P_impl:
		printf("P: S_left → S_right\n");
		print_S(p->u.S_pair.l_child, level + 1);
		print_S(p->u.S_pair.r_child, level + 1);
		break;
	default:
		fprintf(stderr, "Error printing invalid P node\n");
		abort();
	}
}

int main(void)
{
	P *prop;

	next();
	prop = parse_P();
	print_P(prop, 0);
	free(prop);
	printf("done\n");
	return 0;
}

