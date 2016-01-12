#ifndef _NFA_H_
#define _NFA_H_

#include <string>
#include <memory>

namespace my_regex{

// 如果VS版本是2015
// 则字符用utf32表示，省的麻烦
#if _MSC_VER < 1900
typedef char my_char;
typedef std::string my_string;
#else
typedef char32_t my_char;
typedef std::u32string my_string;
#endif

class nfa_node;
typedef std::shared_ptr<nfa_node> p_nfa_node;
typedef std::shared_ptr<const nfa_node> cp_nfa_node;

class NFA
{
public:
	NFA() = delete;
	explicit NFA(const my_string &reg)
		:reg(reg)
	{
		create_nfa_node(reg);
	}

public:	
	// 生成一个dot文件
	// 可以用graphviz工具来将dot文件转化为pdf等格式
	// 如 dot -Tpdf -o file.pdf file.dot
	void create_dot_file(const my_string &filename);

	// 利用生成的NFA来尝试匹配一个字符串str
	// 返回匹配到第几个字符
	size_t try_match(const my_string &str);

	void set_reg(const my_string &_reg)
	{
		reg = _reg;
		create_nfa_node(reg);
	}

private:
	// 从一个正则表达式reg生成NFA
	void create_nfa_node(const my_string &reg);
	cp_nfa_node start;
	my_string reg;
};

}
#endif // _NFA_H