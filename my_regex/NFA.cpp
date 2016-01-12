#include "NFA.h"
#include <cassert>
#include <vector>
#include <fstream>
#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>

using namespace my_regex;

// 正常的边，接受一个字符
class normal_edge
{
public:
	explicit normal_edge(my_char _c, p_nfa_node _p_next_node)
		:c(_c), p_next_node(_p_next_node){}
	my_char c;
	p_nfa_node p_next_node;
};

// epson边，不接受输入，直接跳转到下一个节点
class epson_edge
{
public:
	explicit epson_edge(p_nfa_node _p_next_node)
		:p_next_node(_p_next_node){}
	p_nfa_node p_next_node;
};

// 小数点边，接受任意的字符
class point_edge
{
public:
	explicit point_edge(p_nfa_node _p_next_node)
		:p_next_node(_p_next_node){}
	p_nfa_node p_next_node;
};

// NFA节点
class my_regex::nfa_node
{
public:
	std::vector<normal_edge> norm_edgs;
	std::vector<epson_edge> epson_edgs;
	std::vector<point_edge> point_edgs;
	bool accept = false;	//是否可以被接受
};

// 基本元素类
class element
{
public:
	virtual const bool is_open_parentheses(){ return false; }
	virtual const bool is_close_parentheses(){ return false; }
	virtual const bool is_character(){ return false; }
	virtual const bool is_num_symbol(){ return false; }
	virtual const bool is_point_symbol(){ return false; }
	virtual const bool is_prefix_exp(){ return false; }
	virtual const bool is_or_symbol(){ return false; }
};
typedef std::shared_ptr<element> p_element;

// 普通的符号，包括用\引着的特殊符号
class character : public element
{
public:
	virtual const bool is_character(){ return true; }
	explicit character(my_char _c)
	{
		c = _c;
	}
	my_char c;
};

class open_parentheses : public element//开括号
{
public:
	virtual const bool  is_open_parentheses(){ return true; }
};
class close_parentheses : public element//闭括号
{
public:
	virtual const bool is_close_parentheses(){ return true; }
};

// 数目符号，+、？、*
class num_symbol : public element
{
public:
	virtual const bool is_num_symbol(){ return true; }
	explicit num_symbol(my_char _c)
	{
		if (_c == my_char('+') || _c == my_char('?') || _c == my_char('*'))
		{
			c = _c;
		}
		else
		{
			//错误
			throw std::logic_error("num_symbol类只能接受+、?、*三种符号");
		}
	}
	my_char c;
};

// 小数点符号
class point_symbol : public element
{
public:
	virtual const bool is_point_symbol(){ return true; }
};

// 或操作符
class or_symbol : public element
{
public:
	virtual const bool is_or_symbol(){ return true; }
};

// 前缀表达式
class prefix_exp : public element
{
public:
	virtual const bool is_prefix_exp(){ return true; }
	std::vector<p_element> expression;
};
typedef std::shared_ptr<prefix_exp> p_prefix_exp;

// 将刚开始的元素转化成前缀表达式
static void element_vec_2_prefix_exp(std::vector<p_element> &element_vec)
{		
	// 由于会改变vector
	// 所以各个处理循环得一个一个来

	// 处理括号
	// 注意这里的处理流程，每次遇见开括号
	// 处理后原来开括号处放的是处理好后的前缀表达式
	// 正好不需要对i进行操作
	for (int i = 0; i < element_vec.size(); i++)
	{
		if (element_vec[i]->is_open_parentheses())
		{
			unsigned int s = 1;
			int j = 0, k = 0;
			for (j = i + 1; ; j++)
			{
				if (element_vec[j]->is_open_parentheses())
				{
					s++;
				}
				if (element_vec[j]->is_close_parentheses())
				{
					s--;
				}
				if (s == 0)
				{
					break;
				}
			}
			// 此时i和j之间的是括号的部分
			std::vector<p_element> parenthese;
			parenthese.reserve(j - i - 1);
			for (k = i + 1; k < j; k++)
			{
				parenthese.push_back(element_vec[k]);
			}

			element_vec_2_prefix_exp(parenthese);
			element_vec[j] = parenthese[0];// 此时parenthese里面就剩一个前缀表达式
			element_vec.erase(element_vec.begin() + i, element_vec.begin() + j);
		}		
	}

	// 处理数量字符
	for (int i = 0; i < element_vec.size(); i++)
	{
		if (element_vec[i]->is_num_symbol())
		{
			p_prefix_exp pre(new prefix_exp);
			pre->expression.push_back(element_vec[i]);
			if (!element_vec[i - 1]->is_prefix_exp())
			{
				// 如果不是前缀表达式，说明是普通字符或者小数点，要变成前缀表达式好处理
				p_prefix_exp t(new prefix_exp);
				t->expression.push_back(element_vec[i - 1]);
				pre->expression.push_back(t);
			}
			else
			{
				pre->expression.push_back(element_vec[i - 1]);
			}
			element_vec[i - 1] = pre;
			element_vec.erase(element_vec.begin() + i);
			i--;
		}
	}

	// 处理或操作符
	for (int i = 0; i < element_vec.size(); i++)
	{
		if (element_vec[i]->is_or_symbol())
		{
			p_prefix_exp pre(new prefix_exp);
			p_prefix_exp first_pre(new prefix_exp);
			p_prefix_exp second_pre(new prefix_exp);
			pre->expression.push_back(element_vec[i]);
			pre->expression.push_back(first_pre);
			pre->expression.push_back(second_pre);
			int begin = 0, end = 0;
			// 由于是从前往后遍历的，所以从头开始
			for (int j = begin; j < i; j++)
			{
				first_pre->expression.push_back(element_vec[j]);
			}
			for (end = i + 1; end < element_vec.size(); end++)
			{
				if (element_vec[end]->is_or_symbol())
				{
					break;
				}
			}
			for (int j = i + 1; j < end; j++)
			{
				second_pre->expression.push_back(element_vec[j]);
			}
			element_vec.erase(element_vec.begin() + 1, element_vec.begin() + end);
			element_vec[0] = pre;
			i = 0;
		}
	}

	if (element_vec.size() > 1)
	{
		p_prefix_exp pre(new prefix_exp);
		pre->expression = element_vec;
		element_vec.clear();
		element_vec.push_back(pre);
	}
	// 此时整个element_vec应该就一个元素，对每一层的递归调用都是
	assert(element_vec.size() == 1);
}

// 将前缀表达式转化为NFA
static void prefix_exp_2_nfa_node(const std::vector<p_element> &element_vec, p_nfa_node begin, p_nfa_node end)
{
	if (element_vec[0]->is_character() || element_vec[0]->is_point_symbol() || element_vec[0]->is_prefix_exp())
	{
		p_nfa_node now = begin, temp;
		// 把大家串起来就好了
		for (int i = 0; i < element_vec.size(); i++)
		{
			if (element_vec[i]->is_character())
			{
				temp = p_nfa_node(new nfa_node);
				now->norm_edgs.push_back(normal_edge(std::dynamic_pointer_cast<character>(element_vec[i])->c, temp));
				now = temp;
			}
			if (element_vec[i]->is_point_symbol())
			{
				temp = p_nfa_node(new nfa_node);
				now->point_edgs.push_back(point_edge(temp));
				now = temp;
			}
			if (element_vec[i]->is_prefix_exp())
			{
				p_nfa_node begin_temp(new nfa_node), end_temp(new nfa_node);
				prefix_exp_2_nfa_node(std::dynamic_pointer_cast<prefix_exp>(element_vec[i])->expression, begin_temp, end_temp);
				now->epson_edgs.push_back(epson_edge(begin_temp));
				now = end_temp;
			}
		}
		now->epson_edgs.push_back(epson_edge(end));
	}

	if (element_vec[0]->is_num_symbol())
	{
		// 数组只有两个元素
		assert(element_vec.size() == 2);
		if (std::dynamic_pointer_cast<num_symbol>(element_vec[0])->c == my_char('?'))
		{
			prefix_exp_2_nfa_node(std::dynamic_pointer_cast<prefix_exp>(element_vec[1])->expression, begin, end);
			begin->epson_edgs.push_back(epson_edge(end));
		}
		if (std::dynamic_pointer_cast<num_symbol>(element_vec[0])->c == my_char('*'))
		{
			prefix_exp_2_nfa_node(std::dynamic_pointer_cast<prefix_exp>(element_vec[1])->expression, begin, end);
			begin->epson_edgs.push_back(epson_edge(end));
			end->epson_edgs.push_back(epson_edge(begin));
		}
		if (std::dynamic_pointer_cast<num_symbol>(element_vec[0])->c == my_char('+'))
		{
			prefix_exp_2_nfa_node(std::dynamic_pointer_cast<prefix_exp>(element_vec[1])->expression, begin, end);
			end->epson_edgs.push_back(epson_edge(begin));
		}
	}

	if (element_vec[0]->is_or_symbol())
	{
		// 数组只有三个元素
		assert(element_vec.size() == 3);
		p_nfa_node begin_temp1(new nfa_node), end_temp1(new nfa_node),\
			begin_temp2(new nfa_node), end_temp2(new nfa_node);
		prefix_exp_2_nfa_node(std::dynamic_pointer_cast<prefix_exp>(element_vec[1])->expression, begin_temp1, end_temp1);
		prefix_exp_2_nfa_node(std::dynamic_pointer_cast<prefix_exp>(element_vec[2])->expression, begin_temp2, end_temp2);
		begin->epson_edgs.push_back(epson_edge(begin_temp1));
		begin->epson_edgs.push_back(epson_edge(begin_temp2));
		end_temp1->epson_edgs.push_back(epson_edge(end));
		end_temp2->epson_edgs.push_back(epson_edge(end));
	}
}


void my_regex::NFA::create_nfa_node(const my_string &reg)
{
	// 第一遍遍历的时候主要处理\开头引出的特殊符号
	std::vector<p_element> element_vec;
	element_vec.reserve(reg.length());
	p_element elem;
	for (int i = 0; i < reg.length(); i++)
	{
		switch (reg[i])
		{
		case my_char('+'):
		case my_char('?'):
		case my_char('*'):
			elem = p_element(new num_symbol(reg[i]));
			element_vec.push_back(elem);
			break;
		case my_char('\\'):
			elem = p_element(new num_symbol(reg[++i]));// 注意，这里现在是把所有跟在反斜杠后面的字符都当作普通字符来处理
			element_vec.push_back(elem);
			break;
		case my_char('.'):
			elem = p_element(new point_symbol);
			element_vec.push_back(elem);
			break;
		case my_char('('):
			elem = p_element(new open_parentheses);
			element_vec.push_back(elem);
			break;
		case my_char(')'):
			elem = p_element(new close_parentheses);
			element_vec.push_back(elem);
			break;
		case my_char('|'):
			elem = p_element(new or_symbol);
			element_vec.push_back(elem);
			break;
		default:
			elem = p_element(new character(reg[i]));
			element_vec.push_back(elem);
			break;
		}
	}

	// 将基础元素转化为前缀表达式
	element_vec_2_prefix_exp(element_vec);
	//assert(element_vec.size() == 1);//最后只剩下一个大的前缀表达式//已经在函数里判断过了
	p_nfa_node begin(new nfa_node), end(new nfa_node);
	prefix_exp_2_nfa_node(element_vec, begin, end);
	end->accept = true;

	// todo:加入清理内存的操作
	start = begin;
}

// 获取这些状态的闭包
static void get_epson_closure(std::vector<cp_nfa_node> &nodes)
{
	auto node_exist = [&nodes](cp_nfa_node node)->bool
	{
		// 查找一个元素是否已经在vector里
		return ( std::find(nodes.begin(), nodes.end(), node) != nodes.end() );
	};
	std::function<void(cp_nfa_node)> get_epson_closure_of_node;//为了递归
	get_epson_closure_of_node = [&nodes, &node_exist, &get_epson_closure_of_node](cp_nfa_node node)->void
	{
		for (auto &epson_chhild : node->epson_edgs)
		{
			if (!node_exist(epson_chhild.p_next_node))
			{
				nodes.push_back(epson_chhild.p_next_node);
				get_epson_closure_of_node(epson_chhild.p_next_node);
			}
		}
	};

	auto nodes_size = nodes.size();
	for (int i = 0; i < nodes_size; i++)
	{
		get_epson_closure_of_node(nodes[i]);
	}
}

size_t my_regex::NFA::try_match(const my_string &str)
{
	size_t farthest = 0;//记录匹配到的最远距离
	std::vector<cp_nfa_node> current_pnodes, next_pnodes;
	current_pnodes.push_back(start);
	get_epson_closure(current_pnodes);
	bool matched = start->accept;//当前字符匹配后是否满足正则表达式
	for (auto &cur_node : current_pnodes)
	{
		matched |= cur_node->accept;
	}
	for (size_t i = 0; i < str.length(); i++)
	{
		matched = false;
		next_pnodes.clear();
		for (auto &cur_node : current_pnodes)
		{
			// 先沿着正常边走
			for (auto &n_edg : cur_node->norm_edgs)
			{
				if (n_edg.c == str[i])
				{
					next_pnodes.push_back(n_edg.p_next_node);
				}
			}

			// 小数点通配符
			for (auto &p_node : cur_node->point_edgs)
			{
				next_pnodes.push_back(p_node.p_next_node);
			}
		}
		if (next_pnodes.empty())
		{
			// 匹配不下去了，停止
			return farthest;
		}
		else
		{
			current_pnodes.swap(next_pnodes);
			get_epson_closure(current_pnodes);
			for (auto &cur_node : current_pnodes)
			{
				matched |= cur_node->accept;
			}	
			if (matched)
			{
				farthest = i+1;
			}
		}
	}
	return farthest;
}

void my_regex::NFA::create_dot_file(const my_string &filename)
{
	std::ofstream outstream(filename, std::ios_base::out | std::ios_base::trunc);

	// 刚开始的一些声明和属性等
	outstream << R"(digraph nfa 
{
	fontname = "Microsoft YaHei";
	fontsize = 10;

	node [shape = circle, fontname = "Microsoft YaHei", fontsize = 10];
	edge [fontname = "Microsoft YaHei", fontsize = 10];

	0 [style = filled, color = lightgray];
)";
	std::vector<cp_nfa_node> already_write;
	already_write.push_back(start);
	std::function<void(const cp_nfa_node &cp_node)> output_one_node;//为了递归
	output_one_node = [&output_one_node, &outstream, &already_write](const cp_nfa_node &cp_node)->void{
		size_t this_node_index = std::find(already_write.begin(), already_write.end(), cp_node) - already_write.begin();
		for (auto one_epson_edge : cp_node->epson_edgs)
		{
			size_t next_node_index = std::find(already_write.begin(), already_write.end(), one_epson_edge.p_next_node) - already_write.begin();
			if (next_node_index == already_write.size())
			{
				already_write.push_back(one_epson_edge.p_next_node);
				outstream << "\t" <<next_node_index;
				if (one_epson_edge.p_next_node->accept)
				{
					outstream << R"( [shape = doublecircle])";
				}
				outstream << std::endl;
				output_one_node(one_epson_edge.p_next_node);
			}
			outstream << "\t" << this_node_index << "->" << next_node_index << R"( [label = "epson edge"];)" << std::endl;
		}
		for (auto one_point_edge : cp_node->point_edgs)
		{
			size_t next_node_index = std::find(already_write.begin(), already_write.end(), one_point_edge.p_next_node) - already_write.begin();
			if (next_node_index == already_write.size())
			{
				already_write.push_back(one_point_edge.p_next_node);
				outstream << "\t" << next_node_index;
				if (one_point_edge.p_next_node->accept)
				{
					outstream << R"( [shape = doublecircle])";
				}
				outstream << std::endl;
				output_one_node(one_point_edge.p_next_node);
			}
			outstream << "\t" << this_node_index << "->" << next_node_index << R"( [label = "point edge"];)" << std::endl;
		}
		for (auto one_norm_edge : cp_node->norm_edgs)
		{
			size_t next_node_index = std::find(already_write.begin(), already_write.end(), one_norm_edge.p_next_node) - already_write.begin();
			if (next_node_index == already_write.size())
			{
				already_write.push_back(one_norm_edge.p_next_node);
				outstream << "\t" << next_node_index;
				if (one_norm_edge.p_next_node->accept)
				{
					outstream << R"( [shape = doublecircle])";
				}
				outstream << std::endl;
				output_one_node(one_norm_edge.p_next_node);
			}
			outstream << "\t" << this_node_index << "->" << next_node_index << R"( [label = ")" << one_norm_edge.c << R"("];)" << std::endl;
		}
	};
	output_one_node(start);
	outstream << std::endl << "\t" << R"("NFA: )" << reg << R"(" [shape = plaintext];)" << std::endl << "}";
	outstream.close();
}
