#include "txt2html.h"

bool rule_match(const char *str, NodeType type)
{
	if ((type & CLOSE) && strlen(str) >= 2)
		return (str[0] == '\n' && str[1] == '\n');

	bool match;
	switch (type) {
		case OPEN+OL+LI:
			if (strlen(str) >= rule_len(OPEN+OL+LI))
				match = (isalnum(str[0]) && str[1] == '.' && str[2] == ' ');
			break;
		case OPEN+UL+LI:
			if (strlen(str) >= rule_len(OPEN+UL+LI))
				match = ((str[0] == '-' || str[0] == '*') && str[1] == ' ');
			break;
		case OPEN+PRE:
			// +1 to peek and make sure next char is print
			if (strlen(str) >= rule_len(OPEN+PRE)+1)
				match = (str[0] == '\t' && isprint(str[1]));
			break;
		default:
			match = false;
	}
	return match;
}

size_t rule_len(NodeType type)
{
	if (type & CLOSE) return 2;

	int len = 0;
	switch (type) {
		case OPEN+OL+LI:
			return 3;
		case OPEN+UL+LI:
			return 2;
		case OPEN+PRE:
			return 1;
	}
	return len;
}
