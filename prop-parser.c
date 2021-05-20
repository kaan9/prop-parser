#include <ctype.h>
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
struct token {
	enum token_type { ID, LPAREN, RPAREN, LNEG, LAND, LOR, LIMPL, NONE } type;
	char *id;	/* only used when type == ID, must be copied */
};

/* does not own the program source, only points to it */
struct parser {
	struct token curr_token;
	struct token (*next_token)(char **str);
	char *prog_pos;
};

void *kalloc(size_t size);

/* returns first token in the string pointed by *s and increments *s to the position
 * immediately after the token if it exists, also allocs and sets sets id if the token is an ID
 * otherwise id == NULL and free(id) can be called safely
 */
struct token lex_token(char **str)
{
	char *s = *str, *p = *str;
	struct token tok = {NONE, NULL}; /* invalid input if no character matched */

	while (isspace(*s))
		s++;
	switch (*s) {
	case '(':
		*str = s + 1;
		tok.type = LPAREN;
		break;
	case ')':
		*str = s + 1;
		tok.type = RPAREN;
		break;
	case '!': /* C doesn't have unicode support :( */
		*str = s + 1;
		tok.type = LNEG;
		break;
	case '&':
		*str = s + 1;
		tok.type = LAND;
		break;
	case '|':
		*str = s + 1;
		tok.type = LOR;
		break;
	case '>':
		*str = s + 1;
		tok.type = LIMPL;
		break;
	default:
		if (isalnum(*s)) {
			while (isalnum(*p))
				p++;
			tok.id = kalloc(sizeof(*(tok.id)) * (p - s + 1));
			strncpy(tok.id, s, p - s);
			tok.id[p-s] = '\0';
			*str = p;
			tok.type = ID;
		}
	}
	printf("Got token [type: %d, id: %s]\n", tok.type, tok.id);
	return tok;
}

/* makes parse fetch next token */
void next(struct parser *p)
{
	free(p->curr_token.id);
	p->curr_token = (*(p->next_token))(&p->prog_pos);
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

P *parse_P(struct parser *parser);

/* parse a subproposition */
/* assumes a valid token has been loaded */
S *parse_S(struct parser *parser)
{
	S *s = kalloc(sizeof(*s));	
	
	if (parser->curr_token.type == ID) {
		s->type = S_ID;
		s->u.id = kstrdup(parser->curr_token.id);
		next(parser);
	} else if (parser->curr_token.type == LPAREN) {
		s->type = S_P;
		next(parser);
		s->u.P_child = parse_P(parser);
		if (parser->curr_token.type != RPAREN) {
			fprintf(stderr, "Error, expected ')'\n");
			abort();
		}
		next(parser);
	} else {
		fprintf(stderr, "Error, unexpected token when parsing S\n");
		abort();
	}
	return s;
}

/* parse a proposition */
P *parse_P(struct parser *parser)
{
	P *p = kalloc(sizeof(*p));

	if (parser->curr_token.type == LNEG) {
		p->type = P_neg;
		next(parser);
		p->u.S_child = parse_S(parser);
	} else {
		p->u.S_pair.l_child = parse_S(parser);

		switch (parser->curr_token.type) {
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
		next(parser);
		p->u.S_pair.r_child = parse_S(parser);
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
	char *prog = "((!(a&b))|(c>(!d)))";
	struct parser p = {{NONE, NULL}, &lex_token, prog};
	P *prop;

	next(&p);
	prop = parse_P(&p);
	print_P(prop, 0);
	free(prop);
	printf("done\n");
	return 0;
}

