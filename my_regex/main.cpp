#include "NFA.h"
#include <cassert>
#include <iostream>

using namespace std;
using namespace my_regex;

int main(int argc, char **argv)
{
	my_string reg = "a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)+", str = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	NFA nfa(reg);
	size_t match_len = nfa.try_match(str);
	nfa.create_dot_file("fuck.dot");

	assert(match_len == str.length());

	return 0;
}