#ifndef _NFA_H_
#define _NFA_H_

#include <string>
#include <memory>

namespace my_regex{

// ���VS�汾��2015
// ���ַ���utf32��ʾ��ʡ���鷳
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
	// ����һ��dot�ļ�
	// ������graphviz��������dot�ļ�ת��Ϊpdf�ȸ�ʽ
	// �� dot -Tpdf -o file.pdf file.dot
	void create_dot_file(const my_string &filename);

	// �������ɵ�NFA������ƥ��һ���ַ���str
	// ����ƥ�䵽�ڼ����ַ�
	size_t try_match(const my_string &str);

	void set_reg(const my_string &_reg)
	{
		reg = _reg;
		create_nfa_node(reg);
	}

private:
	// ��һ��������ʽreg����NFA
	void create_nfa_node(const my_string &reg);
	cp_nfa_node start;
	my_string reg;
};

}
#endif // _NFA_H