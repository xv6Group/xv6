#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

#define BUF_SIZE 48
#define MAX_LINE_NUMBER 40
#define MAX_LINE_LENGTH 32
#define MAX_VARS_NUMBER 8
#define MAX_FUNCTION_NUMBER 4
#define MAX_NAME_LINGTH 24
#define MAX_STACK_LENGTH 8
#define NULL 0

//支持的变量类型
#define VOID 0
#define INT 1
#define CHAR 2
#define STRING 3

//变量类型
typedef struct
{
	int type;
	char name[MAX_NAME_LINGTH];
	void *value;
}Var;

//If语句块
typedef struct
{
	int start;
	int end;
	char condition[MAX_NAME_LINGTH * 2];
}IfBlock;

//If-Else语句块
typedef struct
{
	int if_start;
	int if_end;
	int else_start;
	int else_end;
	char condition[MAX_NAME_LINGTH * 2];
}IfElseBlock;

//For语句块
typedef struct
{
	int start;
	int end;
	char init[MAX_NAME_LINGTH * 2];
	char condition[MAX_NAME_LINGTH * 2];
	char back[MAX_NAME_LINGTH * 2];
}ForBlock;

//函数类型
typedef struct
{
	//起始行号
	int start;
	//终止行号
	int end;
	//返回类型
	int return_type;
	//函数名
	char name[MAX_NAME_LINGTH];
	//参数类型
	int param_type[MAX_VARS_NUMBER];
	//参数名称
	char param_name[MAX_VARS_NUMBER][MAX_NAME_LINGTH];
}Function;

//栈帧类型
typedef struct
{
	//函数栈帧
	Function function;
	//当前运行位置
	int current;
	//变量列表
	Var vars[MAX_VARS_NUMBER];
	//返回值
	Var return_var;
}Frame;

//全局变量
char *text[MAX_LINE_NUMBER];
Function fun_list[MAX_FUNCTION_NUMBER];
int fun_list_number;
Frame stack_frame[MAX_STACK_LENGTH];
int stack_frame_number;
Var global_vars[MAX_VARS_NUMBER];
int global_vars_number;

//函数声明
void read_script(char *path);
char* strcat_n(char* dest, char* src, int len);
int strcmp_n(const char *str1, const char *str2, int n);
int split(char *src, char pattern, char *result[]);
void free_split(char *array[]);
char* remove_spaces(char *line);
char* remove_semicolon(char *line);
int is_function(char *part);
int is_variable(char *part);
char get_char_value(char *part);
void get_string_value(char *part, char *result);
int find_vars_in_line(char *line, Var *vars, char *scope);
void remove_duplicate_variables(Var *vars);
void find_global_vars();
void find_functions();
int get_fun_finish(int start);
int push_function(char *fun_str);
void execute_text();
Var run_function();
int run_line(int line_number, int *return_flag, Var *return_var);
void convert_inc_dec(char *part);
Var get_var(char *part);
void set_var(char *var_name, Var *var);
Var run_expression(char *expression);
Var calculate(char *expression, char kind);
void itoa(char *s, int n);
void run_command(char* cmd);
void process_text();
IfBlock *get_if_block(int line_number);
IfElseBlock *get_if_else_block(int line_number);
ForBlock *get_for_block(int line_number);
int run_if_block(IfBlock *i_block, int *return_flag, Var *return_var);
int run_if_else_block(IfElseBlock *ie_block, int *return_flag, Var *return_var);
int run_for_block(ForBlock *f_block, int *return_flag, Var *return_var);
int get_condition_result(char *part);
int calculate_condition(Var *left, Var *right, char *operator);

int main(int argc, char *argv[])
{
	//判断参数个数
	if (argc < 2)
	{
		printf(1, "please input the command as [script file]\n");
		exit();
	}
	//读取文件
	read_script(argv[1]);
	//处理
	process_text();
	//退出程序
	exit();
}

//拼接src的前n个字符到dest
char* strcat_n(char* dest, char* src, int len)
{
	if (len <= 0)
		return dest;
	int pos = strlen(dest);
	if (len + pos >= MAX_LINE_LENGTH)
		return dest;
	int i = 0;
	for (; i < len; i++)
		dest[i+pos] = src[i];
	dest[len+pos] = '\0';
	return dest;
}

//比较两个字符串前n个字符的大小
int strcmp_n(const char *str1, const char *str2, int n)
{
	int i = 0;
	for (; i < n; i++)
	{
		if (str1[i] < str2[i])
			return -1;
		if (str1[i] > str2[i])
			return 1;
	}
	return 0;
}

//将一段内容按照某个字符分割，pattern不能是单双引号
int split(char *src, char pattern, char *result[])
{
	//标记是否处于引号中
	int single_quotes = 0;
	int double_quotes = 0;
	
	result[0] = malloc(MAX_NAME_LINGTH);
	memset(result[0], 0, MAX_NAME_LINGTH);
	int number = 0;
	int len = strlen(src);
	int position = 0;
	int i = 0;
	while (i < len)
	{
		for (; i < len && (src[i] != pattern || single_quotes || double_quotes); i++) {
			if (src[i] == '\'')
				single_quotes = single_quotes ^ 1;
			if (src[i] == '\"')
				double_quotes = double_quotes ^ 1;
			result[number][i - position] = src[i];
		}
		if (i >= len)
			break;
		if (i < len && src[i] == pattern)
			position = i + 1;
		if (i < len && src[i] == pattern && i + 1 < len && result[number][0] != '\0')
		{
			number++;
			result[number] = malloc(MAX_NAME_LINGTH);
			memset(result[number], 0, MAX_NAME_LINGTH);
		}
		i = position;
	}
	if (result[number][0] != '\0')
		number++;
	return number;
}

//释放split申请的内存
void free_split(char *array[])
{
	int i = 0;
	for (; array[i] != NULL; i++)
	{
		free(array[i]);
		array[i] = NULL;
	}
}

char* remove_spaces(char *line)
{
	//保存结果
	char result[MAX_LINE_LENGTH] = {};
	int pos = 0;
	//标记是否处于引号中
	int single_quotes = 0;
	int double_quotes = 0;
	//初始化
	int len = strlen(line);
	int i = 0;
	//循环查找
	for (; i < len; i++)
	{
		if (line[i] != '\t')
		{
			//处理引号问题
			if (line[i] != ' ' || single_quotes != 0 || double_quotes != 0)
			{
				result[pos] = line[i];
				pos++;
			}
			//处理声明问题
			else if (line[i] != ' ' ||
				(i >= 4 && strcmp_n(&line[i-4], "void", 4) == 0) ||
				(i >= 3 && strcmp_n(&line[i-3], "int", 3) == 0) ||
				(i >= 4 && strcmp_n(&line[i-4], "char", 4) == 0) ||
				(i >= 6 && strcmp_n(&line[i-6], "string", 6) == 0) ||
				(i >= 6 && strcmp_n(&line[i-6], "return", 6) == 0)
				)
			{
				result[pos] = line[i];
				pos++;
			}
		}
		if (line[i] == '\'')
			single_quotes = single_quotes ^ 1;
		if (line[i] == '\"')
			double_quotes = double_quotes ^ 1;
	}
	memset(line, 0, MAX_LINE_LENGTH);
	strcpy(line, result);
	return line;
}

//去掉分号和之后的东西，因此一行中有多个语句将只识别第一个
//可以认为分号之后的内容为注释
char* remove_semicolon(char *line)
{
	if (strcmp_n(line, "for", 3) == 0)
		return line;
	
	//标记是否处于引号中
	int single_quotes = 0;
	int double_quotes = 0;
	
	int i = 0;
	int len = strlen(line);
	for (; i < len && (line[i] != ';' || single_quotes || double_quotes); i++)
	{
		if (line[i] == '\'')
			single_quotes = single_quotes ^ 1;
		if (line[i] == '\"')
			double_quotes = double_quotes ^ 1;
	}
	for (; i < len; i++)
		line[i] = '\0';
	return line;
}

//读文件
void read_script(char *path)
{
	//打开文件
	int fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		printf(1, "can't open the file\n");
		exit();
	}
	//读取内容
	int line_number = 0;
	text[0] = malloc(MAX_LINE_LENGTH);
	memset(text[0], 0, MAX_LINE_LENGTH);
	char buf[BUF_SIZE] = {};
	int len = 0;
	while ((len = read(fd, buf, BUF_SIZE)) > 0)
	{
		int i = 0;
		int next = 0;
		int is_full = 0;
		while (i < len)
		{
			//拷贝"\n"之前的内容
			for (i = next; i < len && buf[i] != '\n'; i++)
				;
			strcat_n(text[line_number], buf+next, i-next);
			//必要时新建一行
			if (i < len && buf[i] == '\n')
			{
				if (line_number >= MAX_LINE_NUMBER - 1)
					is_full = 1;
				else
				{
					line_number++;
					text[line_number] = malloc(MAX_LINE_LENGTH);
					memset(text[line_number], 0, MAX_LINE_LENGTH);
				}
			}
			if (is_full == 1 || i >= len - 1)
				break;
			else
				next = i + 1;
		}
		if (is_full == 1)
			break;
	}
	close(fd);
}

int is_function(char *part)
{
	if (part[0] == '\0' || part[0] == '(' || part[0] == ')')
		return 0;
	int len = strlen(part);
	int i = 0;
	int ok = 0;
	for (i = 1; i < len; i++)
	{
		if (part[i] == '(')
		{
			ok++;
			break;
		}
	}
	for (i = i + 1; i < len; i++)
		if (part[i] == ')' && ok == 1)
			return 1;
	return 0;
}

int is_variable(char *part)
{
	int i = 0;
	if (part[0] == '\0')
		return 0;
	for (; part[i] != '\0'; i++)
	{
		if (part[i] == '=')
			break;
		if (part[i] == '(' || part[i] == ')')
			return 0;
	}
	return 1;
}

char get_char_value(char *part)
{
	int len = strlen(part);
	if (len <= 2)
		return '\0';
	else if (len == 3)
		return part[1];
	else if (len == 4 && part[1] == '\\')
	{
		if (part[2] == 'a')
			return '\a';
		if (part[2] == 'b')
			return '\b';
		if (part[2] == 'f')
			return '\f';
		if (part[2] == 'n')
			return '\n';
		if (part[2] == 'r')
			return '\r';
		if (part[2] == 't')
			return '\t';
		if (part[2] == 'v')
			return '\v';
		if (part[2] == '\\')
			return '\\';
		if (part[2] == '\'')
			return '\'';
		if (part[2] == '\"')
			return '\"';
		if (part[2] == '\0')
			return '\0';
	}
	return '\0';
}

void get_string_value(char *part, char *result)
{
	int pos = 0;
	int len = strlen(part);
	if (len < 2)
		return;
	int i = 1;
	for (; i < len - 1; i++)
	{
		if (part[i] != '\\')
			result[pos] = part[i];
		else if (i < len - 2)
		{
			char one_char[5] = {};
			one_char[0] = '\'';
			one_char[1] = '\\';
			one_char[2] = part[i+1];
			one_char[3] = '\'';
			result[pos] = get_char_value(one_char);
			i++;
		}
		pos++;
	}
}

//获得一行的变量，返回获得的个数
int find_vars_in_line(char *line, Var *vars, char *scope)
{
	int count = 0;
	char *split_vars[MAX_NAME_LINGTH] = {};
	if (strcmp_n(line, "int ", 4) == 0 && is_variable(line+4))
	{
		int n = split(line+4, ',', split_vars);
		int j = 0;
		for (; j < n; j++)
		{
			//判断是int a,b,c还是int a=1,b=2,c=3
			char *split_expression[MAX_NAME_LINGTH] = {};
			int m = split(split_vars[j], '=', split_expression);
			//没有赋值的变量
			if (m == 1)
			{
				vars[count].type = INT;
				strcpy(vars[count].name, split_expression[0]);
				vars[count].value = malloc(sizeof(int));
				*(int *)(vars[count].value) = 0;
				count++;
			}
			if (m >= 2)
			{
				vars[count].type = INT;
				strcpy(vars[count].name, split_expression[0]);
				vars[count].value = malloc(sizeof(int));
				Var result = run_expression(split_expression[1]);
				set_var(vars[count].name, &result);
				count++;
			}
			free_split(split_expression);
		}
		free_split(split_vars);
	}
	if (strcmp_n(line, "char ", 5) == 0 && is_variable(line+5))
	{
		int n = split(line+5, ',', split_vars);
		int j = 0;
		for (; j < n; j++)
		{
			//判断是char a,b,c还是char a='a',b='b',c='c'
			char *split_expression[MAX_NAME_LINGTH] = {};
			int m = split(split_vars[j], '=', split_expression);
			//没有赋值的变量
			if (m == 1)
			{
				vars[count].type = CHAR;
				strcpy(vars[count].name, split_expression[0]);
				vars[count].value = malloc(sizeof(char));
				*(char *)(vars[count].value) = '\0';
				count++;
			}
			if (m >= 2)
			{
				vars[count].type = CHAR;
				strcpy(vars[count].name, split_expression[0]);
				vars[count].value = malloc(sizeof(char));
				Var result = run_expression(split_expression[1]);
				set_var(vars[count].name, &result);
				count++;
			}
			free_split(split_expression);
		}
		free_split(split_vars);
	}
	if (strcmp_n(line, "string ", 7) == 0 && is_variable(line+7))
	{
		int n = split(line+7, ',', split_vars);
		int j = 0;
		for (; j < n; j++)
		{
			//判断是string a,b,c还是string a="a",b="b",c="c"
			char *split_expression[MAX_NAME_LINGTH] = {};
			int m = split(split_vars[j], '=', split_expression);
			//没有赋值的变量
			if (m == 1)
			{
				vars[count].type = STRING;
				strcpy(vars[count].name, split_expression[0]);
				vars[count].value = malloc(MAX_LINE_LENGTH);
				memset(vars[count].value, 0, MAX_LINE_LENGTH);
				count++;
			}
			if (m >= 2)
			{
				vars[count].type = STRING;
				strcpy(vars[count].name, split_expression[0]);
				vars[count].value = malloc(MAX_LINE_LENGTH);
				memset(vars[count].value, 0, MAX_LINE_LENGTH);
				Var result = run_expression(split_expression[1]);
				set_var(vars[count].name, &result);
				count++;
			}
			free_split(split_expression);
		}
		free_split(split_vars);
	}
	return count;
}

//删除重复变量
void remove_duplicate_variables(Var *vars)
{
	int count = 0;
	for (; count < MAX_VARS_NUMBER; count++)
		if (vars[count].name[0] == '\0')
			break;
	int i = 0, j = 0;
	int invalid[MAX_VARS_NUMBER] = {};
	for (i = 0; i < count; i++)
		for (j = 0; j < i; j++)
			if (strcmp(vars[i].name, vars[j].name) == 0)
				invalid[j] = 1;
	Var tmp[MAX_VARS_NUMBER];
	memset(tmp, 0, sizeof(Var) * MAX_VARS_NUMBER);
	int pos = 0;
	for (i = 0; i < count; i++)
	{
		if (invalid[i] == 0)
		{
			tmp[pos].type = vars[i].type;
			strcpy(tmp[pos].name, vars[i].name);
			tmp[pos].value = vars[i].value;
			pos++;
		}
	}
	memset(vars, 0, sizeof(Var) * MAX_VARS_NUMBER);
	for (i = 0; i < pos; i++)
	{
		vars[i].type = tmp[i].type;
		strcpy(vars[i].name, tmp[i].name);
		vars[i].value = tmp[i].value;
	}
}

void find_global_vars()
{
	int i = 0, j = 0;
	char scope[1] = {};
	//判断那些行不属于函数部分
	int in_fun[MAX_LINE_NUMBER] = {};
	for (i = 0; i < fun_list_number; i++)
		for (j = fun_list[i].start; j <= fun_list[i].end; j++)
			in_fun[j] = 1;
	//加入全局变量
	for (i = 0; text[i] != NULL; i++)
	{
		if (in_fun[i] == 1)
			continue;
		global_vars_number += find_vars_in_line(text[i], global_vars + global_vars_number, scope);
	}
	//去重
	remove_duplicate_variables(global_vars);
}

void find_functions()
{
	int i = 0;
	for (; text[i] != NULL; i++)
	{
		int pos1 = -1;
		int pos2 = -1;
		if (strcmp_n(text[i], "int ", 4) == 0 && is_function(text[i]+4))
		{
			int j = 4;
			for (; text[i][j] != '(' && text[i][j] != '\0'; j++)
				;
			pos1 = j;
			for (j = pos1+1; text[i][j] != ')' && text[i][j] != '\0'; j++)
				;
			pos2 = j;
			//设定函数名字
			strcat_n(fun_list[fun_list_number].name, text[i]+4, pos1-4);
			//设定函数返回类型
			fun_list[fun_list_number].return_type = INT;
		}
		//函数类型
		if (strcmp_n(text[i], "char ", 5) == 0 && is_function(text[i]+5))
		{
			int j = 5;
			for (; text[i][j] != '(' && text[i][j] != '\0'; j++)
				;
			pos1 = j;
			for (j = pos1+1; text[i][j] != ')' && text[i][j] != '\0'; j++)
				;
			pos2 = j;
			//设定函数名字
			strcat_n(fun_list[fun_list_number].name, text[i]+5, pos1-5);
			//设定函数返回类型
			fun_list[fun_list_number].return_type = CHAR;
		}
		if (strcmp_n(text[i], "string ", 7) == 0 && is_function(text[i]+7))
		{
			int j = 7;
			for (; text[i][j] != '(' && text[i][j] != '\0'; j++)
				;
			pos1 = j;
			for (j = pos1+1; text[i][j] != ')' && text[i][j] != '\0'; j++)
				;
			pos2 = j;
			//设定函数名字
			strcat_n(fun_list[fun_list_number].name, text[i]+7, pos1-7);
			//设定函数返回类型
			fun_list[fun_list_number].return_type = STRING;
		}
		if (strcmp_n(text[i], "void ", 5) == 0 && is_function(text[i]+5))
		{
			int j = 5;
			for (; text[i][j] != '(' && text[i][j] != '\0'; j++)
				;
			pos1 = j;
			for (j = pos1+1; text[i][j] != ')' && text[i][j] != '\0'; j++)
				;
			pos2 = j;
			//设定函数名字
			strcat_n(fun_list[fun_list_number].name, text[i]+5, pos1-5);
			//设定函数返回类型
			fun_list[fun_list_number].return_type = VOID;
		}
		if (pos1 != -1 && pos2 != -1)
		{
			int j = 0;
			//寻找参数类型
			char to_split[MAX_LINE_NUMBER] = {};
			strcat_n(to_split, text[i]+pos1+1, pos2-pos1-1);
			char *split_result[MAX_LINE_NUMBER] = {};
			int number = split(to_split, ',', split_result);
			Var params[MAX_VARS_NUMBER];
			int params_number = 0;
			memset(params, 0, sizeof(Var) * MAX_VARS_NUMBER);
			for (j = 0; j < number; j++)
				params_number += find_vars_in_line(split_result[j], params+params_number, fun_list[fun_list_number].name);
			for (j = 0; j < params_number; j++)
			{
				fun_list[fun_list_number].param_type[j] = params[j].type;
				strcpy(fun_list[fun_list_number].param_name[j], params[j].name);
			}
			free_split(split_result);
			//设定函数起点
			fun_list[fun_list_number].start = i;
			//寻找函数终点
			fun_list[fun_list_number].end = get_fun_finish(i);
			//增加函数计数
			fun_list_number++;
		}
	}
}

int get_fun_finish(int start)
{
	int count = 0;
	int flag = 0;
	int i = start + 1;
	for (; i < MAX_LINE_NUMBER && text[i] != NULL && (flag == 0 || count > 0); i++)
	{
		if (text[i][0] == '{')
		{
			count++;
			flag = 1;
		}
		if (text[i][0] == '}')
		{
			count--;
			flag = 1;
		}
	}
	i--;
	return i;
}

int push_function(char *fun_str)
{
	//找到左括号和右括号的位置
	int i = 0;
	for(; fun_str[i] != '(' && i < MAX_LINE_LENGTH; i++)
		;
	int pos1 = i;
	if (pos1 < 1)
	{
		printf(1, "error, it's not function\n");
		exit();
	}
	for(; fun_str[i] != ')' && i < MAX_LINE_LENGTH; i++)
		;
	int pos2 = i;
	//寻找函数
	char fun_name[MAX_NAME_LINGTH] = {};
	strcat_n(fun_name, fun_str, pos1);
	for (i = 0; i < fun_list_number; i++)
		if (strcmp(fun_name, fun_list[i].name) == 0)
			break;
	if (i == fun_list_number)
	{
		printf(1, "error, can't find function %s\n", fun_name);
		exit();
	}
	stack_frame[stack_frame_number].function = fun_list[i];
	stack_frame[stack_frame_number].current = fun_list[i].start + 2;
	//处理参数
	char param_name[MAX_LINE_LENGTH] = {};
	strcat_n(param_name, fun_str+pos1+1, pos2-pos1-1);
	char *split_param[MAX_VARS_NUMBER] = {};
	int param_number = split(param_name, ',', split_param);
	for (i = 0; i < param_number; i++)
	{
		int len = strlen(split_param[i]);
		//整数
		if (atoi(split_param[i]) != 0 || split_param[i][0] == '0')
		{
			if (stack_frame[stack_frame_number].function.param_type[i] != INT)
			{
				printf(1, "parameter type error\n");
				exit();
			}
			stack_frame[stack_frame_number].vars[i].type = INT;
			strcpy(stack_frame[stack_frame_number].vars[i].name, stack_frame[stack_frame_number].function.param_name[i]);
			stack_frame[stack_frame_number].vars[i].value = malloc(sizeof(int));
			*(int *)stack_frame[stack_frame_number].vars[i].value = atoi(split_param[i]);
		}
		//字符
		else if (split_param[i][0] == '\'' && split_param[i][len-1] == '\'')
		{
			if (stack_frame[stack_frame_number].function.param_type[i] != CHAR)
			{
				printf(1, "parameter type error\n");
				exit();
			}
			stack_frame[stack_frame_number].vars[i].type = CHAR;
			strcpy(stack_frame[stack_frame_number].vars[i].name, stack_frame[stack_frame_number].function.param_name[i]);
			stack_frame[stack_frame_number].vars[i].value = malloc(sizeof(char));
			*(char *)stack_frame[stack_frame_number].vars[i].value = get_char_value(split_param[i]);
		}
		//字符串
		else if (split_param[i][0] == '\"' && split_param[i][len-1] == '\"')
		{
			if (stack_frame[stack_frame_number].function.param_type[i] != STRING)
			{
				printf(1, "parameter type error\n");
				exit();
			}
			stack_frame[stack_frame_number].vars[i].type = STRING;
			strcpy(stack_frame[stack_frame_number].vars[i].name, stack_frame[stack_frame_number].function.param_name[i]);
			stack_frame[stack_frame_number].vars[i].value = malloc(sizeof(char) * MAX_LINE_LENGTH);
			memset(stack_frame[stack_frame_number].vars[i].value, 0, sizeof(char) * MAX_LINE_LENGTH);
			get_string_value(split_param[i], stack_frame[stack_frame_number].vars[i].value);
		}
		//变量
		else
		{
			Var tmp_var = run_expression(split_param[i]);
			//判断文件是否存在
			if (tmp_var.type == 0)
			{
				printf(1, "can't find variable %s", split_param[i]);
				exit();
			}
			//判断类型是否匹配
			if (stack_frame[stack_frame_number].function.param_type[i] != tmp_var.type)
			{
				printf(1, "parameter type error\n");
				exit();
			}
			//赋值
			stack_frame[stack_frame_number].vars[i].type = tmp_var.type;
			strcpy(stack_frame[stack_frame_number].vars[i].name, stack_frame[stack_frame_number].function.param_name[i]);
			if (tmp_var.type == INT)
			{
				stack_frame[stack_frame_number].vars[i].value = malloc(sizeof(int));
				*(int *)(stack_frame[stack_frame_number].vars[i].value) = *(int *)(tmp_var.value);
			}
			if (tmp_var.type == CHAR)
			{
				stack_frame[stack_frame_number].vars[i].value = malloc(sizeof(char));
				*(char *)(stack_frame[stack_frame_number].vars[i].value) = *(char *)(tmp_var.value);
			}
			if (tmp_var.type == STRING)
			{
				stack_frame[stack_frame_number].vars[i].value = malloc(sizeof(char) * MAX_LINE_LENGTH);
				memset(stack_frame[stack_frame_number].vars[i].value, 0, sizeof(char) * MAX_LINE_LENGTH);
				strcpy((char *)stack_frame[stack_frame_number].vars[i].value, (char *)tmp_var.value);
			}
		}
	}
	stack_frame_number++;
	free_split(split_param);
	return stack_frame[stack_frame_number - 1].current;
}

void execute_text()
{
	//初始化操作
	push_function("main()");
	run_function();
}

Var run_function()
{
	int start = stack_frame[stack_frame_number - 1].current;
	int end = stack_frame[stack_frame_number - 1].function.end - 1;
	int i = start;
	int return_flag = 0;
	Var return_var;
	for (; i <= end; i++)
	{
		//run_line()一次处理了n行
		int n = run_line(i, &return_flag, &return_var);
		i += (n-1);
		if (return_flag == 1)
		{
			if (return_var.type != stack_frame[stack_frame_number - 1].function.return_type)
			{
				printf(1, "return type error\n");
				exit();
			}
			//清除变量
			int j = 0;
			for (; stack_frame[stack_frame_number - 1].vars[j].value != NULL; j++)
				free(stack_frame[stack_frame_number - 1].vars[j].value);
			memset(&stack_frame[stack_frame_number - 1], 0, sizeof(Frame));
			stack_frame_number--;
			return return_var;
		}
	}
	//清除变量
	if (return_var.type != stack_frame[stack_frame_number - 1].function.return_type)
	{
		printf(1, "return type error\n");
		exit();
	}
	for (i = 0; stack_frame[stack_frame_number - 1].vars[i].value != NULL; i++)
		free(stack_frame[stack_frame_number - 1].vars[i].value);
	memset(&stack_frame[stack_frame_number - 1], 0, sizeof(Frame));
	stack_frame_number--;
	return return_var;
}

int run_line(int line_number, int *return_flag, Var *return_var)
{
	//空语句
	if (text[line_number][0] == '\0')
		return 1;
	//return语句
	if (strcmp_n(text[line_number], "return", 6) == 0)
	{
		*return_flag = 1;
		*(return_var) = run_expression(text[line_number] + 7);
		return 1;
	}
	//系统命令
	if (strcmp_n(text[line_number], "system(", 7) == 0)
	{
		char com[MAX_LINE_LENGTH] = {};
		char param[MAX_LINE_LENGTH] = {};
		strcat_n(com, text[line_number] + 7, strlen(text[line_number])-8);
		
		Var tmp = run_expression(com);
		if (tmp.type != STRING)
		{
			printf(1, "parameter type error\n");
			exit();
		}
		
		strcpy(param, (char *)tmp.value);
		run_command(param);
		return 1;
	}
	//定义性语句
	int local_vars_number = 0;
	for (; stack_frame[stack_frame_number - 1].vars[local_vars_number].type != VOID; local_vars_number++)
		;
	int number = find_vars_in_line(text[line_number], stack_frame[stack_frame_number - 1].vars + local_vars_number, stack_frame[stack_frame_number - 1].function.name);
	local_vars_number += number;
	remove_duplicate_variables(stack_frame[stack_frame_number - 1].vars);
	if (number > 0)
		return 1;
	//if逻辑块和if-else逻辑块
	if (strcmp_n(text[line_number], "if", 2) == 0)
	{
		IfElseBlock *ie_block = get_if_else_block(line_number);
		if (ie_block != NULL)
		{
			int n = run_if_else_block(ie_block, return_flag, return_var);
			free(ie_block);
			return n;
		}
		else
		{
			IfBlock *i_block = get_if_block(line_number);
			if (i_block != NULL)
			{
				int n = run_if_block(i_block, return_flag, return_var);
				free(i_block);
				return n;
			}
			else
			{
				printf(1, "unknown line: %s\n", text[line_number]);
				exit();
			}
		}
	}
	//for逻辑块
	if (strcmp_n(text[line_number], "for", 3) == 0)
	{
		ForBlock *f_block = get_for_block(line_number);
		if (f_block != NULL)
		{
			int n = run_for_block(f_block, return_flag, return_var);
			free(f_block);
			return n;
		}
		else
		{
			printf(1, "unknown line: %s\n", text[line_number]);
			exit();
		}
	}
	//++语句和--语句
	convert_inc_dec(text[line_number]);
	//赋值语句
	char *split_equal[MAX_VARS_NUMBER] = {};
	number = split(text[line_number], '=', split_equal);
	if (number != 2)
	{
		//不接收返回值的函数调用逻辑块
		if(is_function(text[line_number]))
		{
			push_function(text[line_number]);
			run_function();
			return 1;
		}
		else
		{
			printf(1, "unknown line: %s\n", text[line_number]);
			exit();
		}
	}
	Var right_val = run_expression(split_equal[1]);
	set_var(split_equal[0], &right_val);
	free_split(split_equal);
	return 1;
}

Var calculate(char *expression, char kind)
{
	Var result;
	memset(&result, 0, sizeof(Var));
	char *split_exp[MAX_VARS_NUMBER] = {};
	split(expression, kind, split_exp);
	Var left = get_var(split_exp[0]);
	Var right = get_var(split_exp[1]);
	if (left.type == INT && right.type == INT && kind == '+')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)left.value + *(int *)right.value;
	}
	else if (left.type == INT && right.type == CHAR && kind == '+')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)left.value + *(char *)right.value;
	}
	else if (left.type == INT && right.type == STRING && kind == '+')
	{
		result.type = STRING;
		char tmp[MAX_LINE_LENGTH] = {};
		itoa(tmp, *(int *)left.value);
		strcat_n(tmp, (char *)right.value, strlen((char *)right.value));
		result.value = malloc(sizeof(char) * MAX_LINE_LENGTH);
		memset(result.value, 0, sizeof(char) * MAX_LINE_LENGTH);
		strcpy((char *)result.value, tmp);
	}
	else if (left.type == CHAR && right.type == INT && kind == '+')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(char *)left.value + *(int *)right.value;
	}
	else if (left.type == CHAR && right.type == CHAR && kind == '+')
	{
		result.type = CHAR;
		result.value = malloc(sizeof(char));
		*(char *)result.value = *(char *)left.value + *(char *)right.value;
	}
	else if (left.type == CHAR && right.type == STRING && kind == '+')
	{
		result.type = STRING;
		char tmp[MAX_LINE_LENGTH] = {};
		tmp[0] = *(char *)left.value;
		strcat_n(tmp, (char *)right.value, strlen((char *)right.value));
		result.value = malloc(sizeof(char) * MAX_LINE_LENGTH);
		memset(result.value, 0, sizeof(char) * MAX_LINE_LENGTH);
		strcpy((char *)result.value, tmp);
	}
	else if (left.type == STRING && right.type == INT && kind == '+')
	{
		result.type = STRING;
		result.value = malloc(sizeof(char) * MAX_LINE_LENGTH);
		memset(result.value, 0, sizeof(char) * MAX_LINE_LENGTH);
		strcpy((char *)result.value, (char *)left.value);
		char tmp[MAX_LINE_LENGTH] = {};
		itoa(tmp, *(int *)right.value);
		strcat_n((char *)result.value, tmp, strlen(tmp));
	}
	else if (left.type == STRING && right.type == CHAR && kind == '+')
	{
		result.type = STRING;
		result.value = malloc(sizeof(char) * MAX_LINE_LENGTH);
		memset(result.value, 0, sizeof(char) * MAX_LINE_LENGTH);
		strcpy((char *)result.value, (char *)left.value);
		char tmp[2] = {};
		tmp[0] = *(char *)right.value;
		strcat_n((char *)result.value, tmp, strlen(tmp));
	}
	else if (left.type == STRING && right.type == STRING && kind == '+')
	{
		result.type = STRING;
		result.value = malloc(sizeof(char) * MAX_LINE_LENGTH);
		memset(result.value, 0, sizeof(char) * MAX_LINE_LENGTH);
		strcpy((char *)result.value, (char *)left.value);
		strcat_n((char *)result.value, (char *)right.value, strlen((char *)right.value));
	}
	else if (left.type == INT && right.type == INT && kind == '-')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)left.value - *(int *)right.value;
	}
	else if (left.type == INT && right.type == CHAR && kind == '-')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)left.value - *(char *)right.value;
	}
	else if (left.type == CHAR && right.type == INT && kind == '-')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(char *)left.value - *(int *)right.value;
	}
	else if (left.type == CHAR && right.type == CHAR && kind == '-')
	{
		result.type = CHAR;
		result.value = malloc(sizeof(char));
		*(char *)result.value = *(char *)left.value - *(char *)right.value;
	}
	else if (left.type == INT && right.type == INT && kind == '*')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)left.value * *(int *)right.value;
	}
	else if (left.type == INT && right.type == CHAR && kind == '*')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)left.value * *(char *)right.value;
	}
	else if (left.type == CHAR && right.type == INT && kind == '*')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(char *)left.value * *(int *)right.value;
	}
	else if (left.type == CHAR && right.type == CHAR && kind == '*')
	{
		result.type = CHAR;
		result.value = malloc(sizeof(int));
		*(char *)result.value = *(char *)left.value * *(char *)right.value;
	}
	else if (left.type == INT && right.type == INT && kind == '/')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)left.value / *(int *)right.value;
	}
	else if (left.type == INT && right.type == CHAR && kind == '/')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)left.value / *(char *)right.value;
	}
	else if (left.type == CHAR && right.type == INT && kind == '/')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(char *)left.value / *(int *)right.value;
	}
	else if (left.type == CHAR && right.type == CHAR && kind == '/')
	{
		result.type = CHAR;
		result.value = malloc(sizeof(int));
		*(char *)result.value = *(char *)left.value / *(char *)right.value;
	}
	free_split(split_exp);
	return result;
}

Var run_expression(char *expression)
{
	if (is_function(expression))
	{
		push_function(expression);
		return run_function();
	}
	int i = 0;
	//加法
	for (i = 0; expression[i] != '+' && expression[i] != '\0'; i++)
		;
	if (expression[i] == '+')
	{
		return calculate(expression, '+');
	}
	//减法
	for (i = 0; expression[i] != '-' && expression[i] != '\0'; i++)
		;
	if (expression[i] == '-')
		return calculate(expression, '-');
	//乘法
	for (i = 0; expression[i] != '*' && expression[i] != '\0'; i++)
		;
	if (expression[i] == '*')
		return calculate(expression, '*');
	//除法
	for (i = 0; expression[i] != '/' && expression[i] != '\0'; i++)
		;
	if (expression[i] == '/')
		return calculate(expression, '/');
	//其它情况
	return get_var(expression);
}

//处理++语句和--语句
void convert_inc_dec(char *part)
{
	int i = 0;
	//++
	for (; part[i] != '\0' && part[i] != '+'; i++)
		;
	if (part[i+1] == '+')
	{
		char var[MAX_LINE_NUMBER] = {};
		strcat_n(var, part, i);
		part[i] = '=';
		part[i+1] = '\0';
		strcat_n(part, var, strlen(var));
		strcat_n(part, "+1", 2); 
		return;
	}
	//--
	for (i = 0; part[i] != '\0' && part[i] != '-'; i++)
		;
	if (part[i+1] == '-')
	{
		char var[MAX_LINE_NUMBER] = {};
		strcat_n(var, part, i);
		part[i] = '=';
		part[i+1] = '\0';
		strcat_n(part, var, strlen(var));
		strcat_n(part, "-1", 2); 
		return;
	}
}

//获取变量
Var get_var(char *part)
{
	Var result;
	//返回空
	if (part[0] == '\0')
		return result;
	//返回数字
	if (atoi(part) != 0 || part[0] == '0')
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)(result.value) = atoi(part);
		return result;
	}
	//返回字符
	if (part[0] == '\'')
	{
		result.type = CHAR;
		result.value = malloc(sizeof(char));
		*(char *)(result.value) = get_char_value(part);
		return result;
	}
	//返回字符串
	if (part[0] == '\"')
	{
		result.type = STRING;
		result.value = malloc(sizeof(char) * MAX_LINE_LENGTH);
		memset(result.value, 0, sizeof(char) * MAX_LINE_LENGTH);
		get_string_value(part, (char *)result.value);
		return result;
	}
	//返回变量
	int i = 0;
	int exists = 0;
	Var local;
	memset(&local, 0, sizeof(Var));
	//局部变量中是否存在
	for (; stack_frame[stack_frame_number - 1].vars[i].type != VOID; i++)
	{
		Var current = stack_frame[stack_frame_number - 1].vars[i];
		if (strcmp(current.name, part) == 0)
		{
			exists = 1;
			local = current;
			break;
		}
	}
	//全局变量中是否存在
	if (exists == 0)
	{
		for (i = 0; i < global_vars_number; i++)
		{
			if (strcmp(global_vars[i].name, part) == 0)
			{
				local = global_vars[i];
				exists = 1;
				break;
			}
		}
	}
	//不存在该变量
	if (exists == 0)
	{
		printf(1, "can't find variable %s", part);
		exit();
	}
	if (local.type == INT)
	{
		result.type = INT;
		result.value = malloc(sizeof(int));
		*(int *)result.value = *(int *)local.value;
	}
	if (local.type == CHAR)
	{
		result.type = CHAR;
		result.value = malloc(sizeof(char));
		*(char *)result.value = *(char *)local.value;
	}
	if (local.type == STRING)
	{
		result.type = STRING;
		result.value = malloc(sizeof(char) * MAX_LINE_LENGTH);
		memset(result.value, 0, sizeof(char) * MAX_LINE_LENGTH);
		strcpy((char *)result.value, (char *)local.value);
	}
	return result;
}

void set_var(char *var_name, Var *var)
{
	int i = 0;
	int exists = 0;
	Var *local = NULL;
	//局部变量中是否存在
	for (; stack_frame[stack_frame_number - 1].vars[i].type != VOID; i++)
	{
		Var *current = &(stack_frame[stack_frame_number - 1].vars[i]);
		if (strcmp(current->name, var_name) == 0)
		{
			exists = 1;
			local = current;
		}
	}
	//全局变量中是否存在
	if (exists == 0)
	{
		for (i = 0; i < global_vars_number; i++)
		{
			if (strcmp(global_vars[i].name, var_name) == 0)
			{
				local = &(global_vars[i]);
				exists = 1;
			}
		}
	}
	//不存在该变量
	if (exists == 0)
	{
		printf(1, "can't find variable %s", var_name);
		exit();
	}
	//不同的赋值情况
	if (local->type == INT && var->type == INT)
		*(int *)(local->value) = *(int *)(var->value);
	else if (local->type == INT && var->type == CHAR)
		*(int *)(local->value) = *(char *)(var->value);
	else if (local->type == CHAR && var->type == INT)
		*(char *)(local->value) = *(char *)(var->value);
	else if (local->type == CHAR && var->type == CHAR)
		*(char *)(local->value) = *(char *)(var->value);
	else if (local->type == STRING && var->type == INT)
	{
		memset(local->value, 0, sizeof(char) * MAX_LINE_LENGTH);
		itoa((char *)local->value, *(int *)var->value);
	}
	else if (local->type == STRING && var->type == CHAR)
	{
		memset(local->value, 0, sizeof(char) * MAX_LINE_LENGTH);
		*(char *)local->value = *(char *)var->value;
	}
	else if (local->type == STRING && var->type == STRING)
	{
		memset(local->value, 0, sizeof(char) * MAX_LINE_LENGTH);
		strcpy((char *)local->value, (char *)var->value);
	}
	else
	{
		printf(1, "parameter type error\n");
		exit();
	}
}

//整数到字符串
void itoa(char *s, int n)
{
	int i = 0;
	int len = 0;
	while(n != 0)
	{
		s[len] = n % 10 + '0';
		n = n / 10; 
		len++;
	}
	for(i = 0; i < len/2; i++)
	{
		char tmp = s[i];
		s[i] = s[len - 1 - i];
		s[len - 1 - i] = tmp;
	}
}

//运行一条命令
void run_command(char* cmd)
{
	char *res[MAX_VARS_NUMBER] = {};
	split(cmd, ' ', res);
	if (fork() == 0)
	{
		exec(res[0], res);
	}
	wait();
}

void process_text()
{
	//删除无关的空格和tab
	int i = 0;
	for (; text[i] != NULL; i++)
	{
		remove_spaces(text[i]);
		remove_semicolon(text[i]);
		if (text[i][0] == '/' && text[i][1] == '/')
			memset(text[i], 0, MAX_LINE_LENGTH);
	}
	//找到所有的函数
	find_functions();
	//找到所有的全局变量
	find_global_vars();
	//执行
	execute_text();
}

IfBlock *get_if_block(int line_number)
{
	int count = 0;
	int i = line_number + 1;
	do
	{
		if (text[i][0] == '{')
			count++;
		if (text[i][0] == '}')
			count--;
		if (count > 0)
			i++;
	}
	while(count > 0 && i < MAX_LINE_NUMBER);
	
	if (i == MAX_LINE_NUMBER)
		return NULL;
	IfBlock *i_block = (IfBlock *)malloc(sizeof(IfBlock));
	memset(i_block, 0, sizeof(IfBlock));
	i_block->start = line_number + 2;
	i_block->end = i - 1;
	
	int pos1 = 0;
	for (i = 0; i < MAX_LINE_LENGTH && text[line_number][i] != '('; i++)
		;
	if (i < MAX_LINE_LENGTH)
		pos1 = i;
	for (; i < MAX_LINE_LENGTH && text[line_number][i] != ')'; i++)
		;
	int pos2 = 0;
	if (i < MAX_LINE_LENGTH)
		pos2 = i;
	strcat_n(i_block->condition, text[line_number]+pos1+1, pos2-pos1-1);
	return i_block;
}

IfElseBlock *get_if_else_block(int line_number)
{
	//找到if区域
	int count = 0;
	int i = line_number + 1;
	do
	{
		if (text[i][0] == '{')
			count++;
		if (text[i][0] == '}')
			count--;
		if (count > 0)
			i++;
	}
	while(count > 0 && i < MAX_LINE_NUMBER);
	if (i >= MAX_LINE_NUMBER - 3 || strcmp(text[i+1], "else") != 0)
		return NULL;
	IfElseBlock *ie_block = (IfElseBlock *)malloc(sizeof(IfElseBlock));
	memset(ie_block, 0, sizeof(IfElseBlock));
	ie_block->if_start = line_number + 2;
	ie_block->if_end = i - 1;
	
	count = 0;
	i = i + 2;
	do
	{
		if (text[i][0] == '{')
			count++;
		if (text[i][0] == '}')
			count--;
		if (count > 0)
			i++;
	}
	while(count > 0 && i < MAX_LINE_NUMBER);
	if (i >= MAX_LINE_NUMBER)
	{
		free(ie_block);
		return NULL;
	}
	
	ie_block->else_start = ie_block->if_end + 4;
	ie_block->else_end = i - 1;
	
	//找到条件
	int pos1 = 0;
	for (i = 0; i < MAX_LINE_LENGTH && text[line_number][i] != '('; i++)
		;
	if (i < MAX_LINE_LENGTH)
		pos1 = i;
	for (; i < MAX_LINE_LENGTH && text[line_number][i] != ')'; i++)
		;
	int pos2 = 0;
	if (i < MAX_LINE_LENGTH)
		pos2 = i;
	strcat_n(ie_block->condition, text[line_number]+pos1+1, pos2-pos1-1);
	
	return ie_block;
}

ForBlock *get_for_block(int line_number)
{
	//定位
	int pos1 = 0;
	for (; pos1 < MAX_LINE_LENGTH && text[line_number][pos1] != '('; pos1++)
		;
	int pos2 = pos1 + 1;
	for (; pos2 < MAX_LINE_LENGTH && text[line_number][pos2] != ';'; pos2++)
		;
	int pos3 = pos2 + 1;
	for (; pos3 < MAX_LINE_LENGTH && text[line_number][pos3] != ';'; pos3++)
		;
	int pos4 = pos3 + 1;
	for (; pos4 < MAX_LINE_LENGTH && text[line_number][pos4] != ')'; pos4++)
		;
	if (pos1 >= MAX_LINE_LENGTH || pos2 >= MAX_LINE_LENGTH || pos3 >= MAX_LINE_LENGTH || pos4 >= MAX_LINE_LENGTH)
		return NULL;
	ForBlock *f_block = (ForBlock *)malloc(sizeof(ForBlock));
	memset(f_block, 0, sizeof(ForBlock));
	strcat_n(f_block->init, text[line_number]+pos1+1, pos2-pos1-1);
	strcat_n(f_block->condition, text[line_number]+pos2+1, pos3-pos2-1);
	strcat_n(f_block->back, text[line_number]+pos3+1, pos4-pos3-1);
	convert_inc_dec(f_block->init);
	convert_inc_dec(f_block->back);
	//找到区域
	int count = 0;
	int i = line_number + 1;
	do
	{
		if (text[i][0] == '{')
			count++;
		if (text[i][0] == '}')
			count--;
		if (count > 0)
			i++;
	}
	while(count > 0 && i < MAX_LINE_NUMBER);
	if (i == MAX_LINE_NUMBER)
	{
		free(f_block);
		return NULL;
	}
	f_block->start = line_number + 2;
	f_block->end = i - 1;
	return f_block;
}

int run_if_block(IfBlock *i_block, int *return_flag, Var *return_var)
{
	if (get_condition_result(i_block->condition))
	{
		int i = i_block->start;
		for (; i <= i_block->end; i++)
		{
			int n = run_line(i, return_flag, return_var);
			i += (n-1);
			if (*return_flag == 1)
				break;
		}
	}
	return i_block->end - i_block->start + 4;
}

int run_if_else_block(IfElseBlock *ie_block, int *return_flag, Var *return_var)
{
	int result = get_condition_result(ie_block->condition);
	if (result)
	{
		int i = ie_block->if_start;
		for (; i <= ie_block->if_end; i++)
		{
			int n = run_line(i, return_flag, return_var);
			i += (n-1);
			if (*return_flag == 1)
				break;
		}
	}
	else
	{
		int i = ie_block->else_start;
		for (; i <= ie_block->else_end; i++)
		{
			int n = run_line(i, return_flag, return_var);
			i += (n-1);
			if (*return_flag == 1)
				break;
		}
	}
	return ie_block->else_end - ie_block->if_start + 4;
}

int run_for_block(ForBlock *f_block, int *return_flag, Var *return_var)
{
	//执行初始条件
	char *split_equal[MAX_VARS_NUMBER] = {};
	int number = split(f_block->init, '=', split_equal);
	if (number == 2)
	{
		Var right_val = run_expression(split_equal[1]);
		set_var(split_equal[0], &right_val);
	}
	free_split(split_equal);
	
	//循环
	while(get_condition_result(f_block->condition))
	{
		int i = f_block->start;
		for (; i <= f_block->end; i++)
		{
			int n = run_line(i, return_flag, return_var);
			i += (n-1);
			if (*return_flag == 1)
				break;
		}
		if (*return_flag == 1)
			break;
		//执行后语句
		memset(split_equal, 0, MAX_VARS_NUMBER * sizeof(char *));
		number = split(f_block->back, '=', split_equal);
		if (number == 2)
		{
			Var right_val = run_expression(split_equal[1]);
			set_var(split_equal[0], &right_val);
		}
		free_split(split_equal);
	}
	return f_block->end - f_block->start + 4;
}

int get_condition_result(char *part)
{
	int i = 0;
	int len = strlen(part);
	char operator[3] = {};
	for (i = 0; i < len && part[i] != '>' && part[i] != '<' && part[i] != '=' && part[i] != '!'; i++)
		;
	if (i < len)
	{
		operator[0] = part[i];
		if (part[i+1] == '=')
			operator[1] = part[i+1];
		char str_left[MAX_NAME_LINGTH] = {};
		strcat_n(str_left, part, i);
		char str_right[MAX_NAME_LINGTH] = {};
		strcpy(str_right, part + i + (part[i+1] == '=' ? 2 : 1));
		Var left = get_var(str_left);
		Var right = get_var(str_right);
		return calculate_condition(&left, &right, operator);
	}
	else
	{
		Var var = get_var(part);
		if (var.type == INT)
			return *(int *)(var.value);
		if (var.type == CHAR)
			return *(char *)(var.value);
		if (var.type == STRING)
		{
			char c = *(char *)(var.value);
			return (c == '\0' ? 0 : 1);
		}
	}
	return 0;
}
int calculate_condition(Var *left, Var *right, char *operator)
{
	if ((left->type == INT || left->type == CHAR) && (right->type == INT || right->type == CHAR))
	{
		int number1 = 0;
		int number2 = 0;
		if (left->type == INT)
			number1 = *(int *)(left->value);
		else
			number1 = *(char *)(left->value);
		if (right->type == CHAR)
			number2 = *(int *)(right->value);
		else
			number2 = *(char *)(right->value);
		if (strcmp(operator, ">") == 0)
			return (number1 > number2);
		if (strcmp(operator, "<") == 0)
			return (number1 < number2);
		if (strcmp(operator, ">=") == 0)
			return (number1 >= number2);
		if (strcmp(operator, "<=") == 0)
			return (number1 <= number2);
		if (strcmp(operator, "==") == 0)
			return (number1 == number2);
		if (strcmp(operator, "!=") == 0)
			return (number1 != number2);
	}
	else if(left->type == STRING && right->type == STRING)
	{
		if (strcmp(operator, ">") == 0)
			return strcmp((char *)left->value, (char *)right->value) > 0 ? 1 : 0;
		if (strcmp(operator, "<") == 0)
			return strcmp((char *)left->value, (char *)right->value) < 0 ? 1 : 0;
		if (strcmp(operator, ">=") == 0)
			return strcmp((char *)left->value, (char *)right->value) >= 0 ? 1 : 0;
		if (strcmp(operator, "<=") == 0)
			return strcmp((char *)left->value, (char *)right->value) <= 0 ? 1 : 0;
		if (strcmp(operator, "==") == 0)
			return strcmp((char *)left->value, (char *)right->value) == 0 ? 1 : 0;
		if (strcmp(operator, "!=") == 0)
			return strcmp((char *)left->value, (char *)right->value) != 0 ? 1 : 0;
	}
	else
	{
		printf(1, "can't compare variables\n");
		exit();
	}
	return 0;
}
