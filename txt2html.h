#ifndef _TXT2HTML
#define _TXT2HTML

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h> // replace with utf8 support
#include <assert.h>

typedef uint8_t NodeType;

// node tags
enum NodeTypes {
	OPEN  = 0x10,
	CLOSE = 0x20,
	H1    = 0x01,
	H2    = 0x02,
	P     = 0x03,
	PRE   = 0x04,
	LI    = 0x05,
	BR    = 0x06,
	OL    = 0x07,
	UL    = 0x08
};

/* rule.c */
// check if `str` matches the rule for `NodeType t`
bool rule_match(const char *str, NodeType t);

// get the length of a rule for `NodeType t`
size_t  rule_len(NodeType t);

#endif
