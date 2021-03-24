#include <iostream>
#include <queue>
#include <stack>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>
using namespace std;


struct Node{
	unsigned char c; // 此节点是哪个字节，仅当是叶子节点有意义
	int weight; // 此节点的权值
	Node *lchild;
	Node *rchild;

	Node(unsigned char c_, int weight_, Node *lchild_ = NULL, Node *rchild_ = NULL){
		c = c_;
		weight = weight_;
		lchild = lchild_;
		rchild = rchild_;
	}
};


class Compare_Node_Pointer{
  public:
    bool operator () (Node* &a, Node* &b) const{
        return a->weight > b->weight;
    }
};

Node *create_hfmTree_by_byte_cnt(int const byte_cnt[]){
	/**
	 *根据byte_cnt生成哈夫曼树，然后返回该树的根节点
	 * @byte_cnt: 长度为256的数组，保存某字节在文件中出现的次数
	 * 			  例如：byte_cnt[65] = 101 表示字节65在文件中出现了101次 
	 */
	priority_queue<Node*, vector<Node*>, Compare_Node_Pointer> q;
	for ( int i=0; i<256; i++ ){
		if ( byte_cnt[i] ) {
			q.push( new Node(i, byte_cnt[i]) );
		}
	}
	// cout << "q.size() = " << q.size() << endl; //log
	while ( q.size() > 1 ) {
		Node *a = q.top(); q.pop();
		Node *b = q.top(); q.pop();
		q.push( new Node('x', a->weight + b->weight, a, b) );
	}
	return q.top();
}

void print_hfmTree(Node *root, int deep = 1, string code=".") {
	/**
	 *打印这颗二叉树 
	 * @root 树的根节点
	 * @deep 此节点的深度
	 * @code 从根节点遍历到此处的路径码，向左用0表示，向右用1表示
	 */
	if (!root) {
		return;
	}
	// cout << "__LINE__ = " << __LINE__ << endl; //log
	// cout << "deep = " << deep << endl; //log
	print_hfmTree(root->rchild, deep+1, code+"1");
	for (int i = 0; i < deep; ++i){
		printf(i==deep-1?"+---": (code[i]==code[i+1]?"     ":"|    "));
	}
	if (root->lchild){
		printf("(_)\n");
	}else{
		printf("(%d)\n", root->c);
	}
	print_hfmTree(root->lchild, deep+1, code+"0");
}

bool get_by_bit(unsigned char const arr[], int idx) {
	/**
	 * 按比特获取字符数组中下标为idx的那一位
	 * @arr: 字符数组 
	 * @idx: 下标
	 */
	return arr[idx/8] & (1 << (7 - idx%8));
}

void set_by_bit(unsigned char arr[], int idx, bool value){
	/**
	 * 按比特设置字符数组中下标为idx的那一位
	 * @arr: 字符数组 
	 * @idx: 下标
	 */
	value ?
	arr[idx/8] |= (1 << (7 - idx%8)) :
	arr[idx/8] &= (~(1 << (7 - idx%8)));
}

void encode_hfmTree(Node const *root, vector<bool> &tree_struct_code, vector<unsigned char> &byte_sequence){
	/**
	 * 将哈夫曼树用01编码，将树的结构用01序列表示，叶子节点用字节序列表示
	 * 使用栈先序遍历哈夫曼树，0表示入栈，1表示出栈
	 * @root: 哈夫曼树的根节点
	 * @tree_struct_code: 输出，用01序列表示的压缩过的树的结构
	 * @byte_sequence: 输出，先序遍历哈夫曼树时访问叶子节点的序列
	 */
	stack<const Node *> s;
	s.push(root);
	tree_struct_code.push_back(0);
	map<const Node *, bool> vis;
	while(s.size()){
		const Node *curr = s.top();
		vis[curr] = true;
		if (curr->lchild){
			if (!vis[curr->lchild]) {
				s.push(curr->lchild);
				tree_struct_code.push_back(0);
			} else if (!vis[curr->rchild]) {
				s.push(curr->rchild);
				tree_struct_code.push_back(0);
			} else {
				tree_struct_code.push_back(1); s.pop();
			}
		} else {
			tree_struct_code.push_back(1); s.pop();
			byte_sequence.push_back(curr->c);
		}

	}
}

Node* decode_hfmTree(const vector<bool> &tree_struct_code, const vector<unsigned char> &byte_sequence){
	/**
	 * 将01序列和字节序列解码成哈夫曼树
	 * @tree_struct_code: 01序列，表示一棵哈夫曼树
	 * @byte_sequence: 叶子节点序列
	 * 先序建树，0表示向下新建节点（当前节点没有左孩子则新建左孩子，有则新建右孩子）；
	 * 1表示回溯（向上）
	 */
	stack<Node *> s;
	Node *root;
	int p = 0;
	for ( auto i : tree_struct_code ) {
		if (i==0) {
			if (s.size()) {
				Node *curr = new Node(0,0);
				if (s.top()->lchild) {
					s.top()->rchild = curr;
				} else {
					s.top()->lchild = curr;
				}
				s.push(curr);
			} else {
				s.push( new Node(0, 0) );
			}
		} else {
			if (s.top()->lchild==NULL && s.top()->rchild==NULL){
				s.top()->c = byte_sequence[p++];
			}
			root = s.top(); s.pop();
		}
	}
	return root;
}

void get_encode_table(vector<string> &encode_table, Node * root, string curr_code = ""){
	/**
	 * 从哈夫曼树获取编码表
	 * @encode_table 输出，编码表，即（ 字节 --> 01序列 ）的映射关系
	 * @root 哈夫曼树的根节点
	 * @curr_code 用于递归迭代
	 */
	if (root->lchild==NULL){
		encode_table[root->c] = curr_code;
		return;
	}else{
		get_encode_table(encode_table, root->lchild, curr_code+"0");
		get_encode_table(encode_table, root->rchild, curr_code+"1");
	}
}

vector<unsigned char> zip_process(unsigned char *buf, int file_len){
	/**
	 * 将buf处开始，file_len字节长的数据压缩，返回压缩后的结果
	 * 压缩后的格式： 结尾无效位长度（3bit） + 表示树结构的01序列 + 表示叶子节点的byte序列 + 压缩表示的数据 + 填充的无效位
	 * @buf: 首地址
	 * @file_len: 需要压缩的长度，单位：字节
	 */

	/* 1. 统计每种字节出现的次数 */
	int byte_cnt[256] = {0};
	for (int i = 0; i < file_len; ++i){
		byte_cnt[buf[i]]++;
	}

	/* 2. 构建哈夫曼树 */
	Node *root = create_hfmTree_by_byte_cnt(byte_cnt);
	print_hfmTree(root); //log
	
	/* 3. 将哈夫曼树的结构编码，获得代表结构的01序列和叶子节点的序列 */
	vector<bool> tree_struct_code;
	vector<unsigned char> byte_sequence;
	encode_hfmTree(root, tree_struct_code, byte_sequence);
	//log
	cout << "__LINE__ = " << __LINE__ << endl; //log
	cout << "tree_struct_code.size() = " << tree_struct_code.size() << endl; //log
	for ( auto v : tree_struct_code ) {
		cout << v;
	}
	cout << endl;
	cout << "byte_sequence.size() = " << byte_sequence.size() << endl; //log
	for ( auto v : byte_sequence ) {
		printf("%d ", v);
	}
	cout << endl;

	/* 4. 获取每个字节对应的01序列(编码表) */
	vector<string > encode_table(256);
	get_encode_table(encode_table, root);
	//log
	for (int i = 0; i < 256; ++i){
		printf("%d -> %s\n", i, encode_table[i].c_str());
	}

	/* 5. 计算输出文件的大小(单位：比特)，创建输出缓冲区 */
	int tree_struct_code_len = tree_struct_code.size();
	int byte_sequence_len = byte_sequence.size()*8;
	int out_file_len = 3 + tree_struct_code_len + byte_sequence_len;
	for (int i = 0; i < 256; ++i){
		out_file_len += encode_table[i].size()*byte_cnt[i];
	}
	vector<unsigned char> out_buf_vector(out_file_len/8 + (out_file_len%8!=0));
	unsigned char* out_buf_char_star = &out_buf_vector[0];
	// log
	cout << "__LINE__ = " << __LINE__ << endl; //log
	cout << "tree_struct_code_len = " << tree_struct_code_len << endl; //log
	cout << "byte_sequence.size() = " << byte_sequence.size() << endl; //log
	cout << "out_file_len = " << out_file_len << endl; //log

	/* 6.1 将代表文件结尾多少位冗余的数字存进缓冲区头部，占3位（冗余位数只有可能是0~7）*/
	/*     类似于大端存储，高位在前，低位在后                                            */
	int useless_bit_cnt = (8 - out_file_len % 8) % 8;
	set_by_bit(out_buf_char_star, 0, (useless_bit_cnt>>2) & 1);
	set_by_bit(out_buf_char_star, 1, (useless_bit_cnt>>1) & 1);
	set_by_bit(out_buf_char_star, 2, (useless_bit_cnt>>0) & 1);
	cout << "in line " << __LINE__ << " useless_bit_cnt = " << useless_bit_cnt << endl; //log

	/* 6.2 将代表树的结构的01序列填入缓冲区 */
	int pointer = 3;
	for (int i = 0; i < tree_struct_code.size(); ++i){
		set_by_bit(out_buf_char_star, pointer++, tree_struct_code[i]);
	}

	/* 6.3 将代表叶子节点信息的叶子节点序列填入缓冲区 */
	for (int i = 0; i < byte_sequence.size(); ++i){
		for (int j = 0; j < 8; ++j){
			set_by_bit(out_buf_char_star, pointer++, get_by_bit(&byte_sequence[i], j));
		}
	}
	cout << "pointer = " << pointer << endl; //log

	/* 6.4 将每个字节对应的变长01序列填入缓冲区 */
	for (int i = 0; i < file_len; ++i){
		string const &code = encode_table[buf[i]];
		for ( auto v : code ){
			set_by_bit(out_buf_char_star, pointer++, v=='1');
		}
	}
	cout << "pointer = " << pointer << endl; //log
	printf("压缩率：%.3lf%%\n", (double)out_buf_vector.size()/file_len*100);
	return out_buf_vector;
}

vector<unsigned char> unzip_process(unsigned char *buf, int file_len){
	/**
	 * 将buf处开始，file_len字节长的数据解压，返回解压后的结果
	 * @buf: 首地址
	 * @file_len: buf的长度，单位：字节
	 */

	int pointer = 3; // 跳过记录结尾无效bit长度的32位

	/* 1. 解析提取结尾无效bit的长度 */
	int useless_bit_cnt = 0;
	useless_bit_cnt += get_by_bit(buf, 0) << 2;
	useless_bit_cnt += get_by_bit(buf, 1) << 1;
	useless_bit_cnt += get_by_bit(buf, 2) << 0;
	cout << "in line " << __LINE__ << " useless_bit_cnt = " << useless_bit_cnt << endl; //log
	cout << "__LINE__ = " << __LINE__ << endl; //log

	/* 2. 获取表示树结构的01序列 */
	int cnt_0 = 0;
	int cnt_1 = 0;
	vector<bool> tree_struct_code;
	do{
		bool v = get_by_bit(buf, pointer++);
		tree_struct_code.push_back(v);
		cnt_0 += v==0;
		cnt_1 += v==1;
	}while ( cnt_0 != cnt_1 );

	int byte_sequence_len = tree_struct_code.size()/2/2+1;

	// log
	cout << "__LINE__ = " << __LINE__ << endl; //log
	cout << "tree_struct_code.size() = " << tree_struct_code.size() << endl; //log
	for ( auto v : tree_struct_code ) {
		cout << v;
	}
	cout << endl;

	/* 3. 获取表示叶子信息的字节序列 */
	vector<unsigned char> byte_sequence;
	for (int i = 0; i < byte_sequence_len; ++i){
		unsigned char ch = 0;
		for (int j = 0; j < 8; ++j){
			ch <<= 1;
			ch += get_by_bit(buf, pointer++);
		}
		byte_sequence.push_back(ch);
	}

	//log 
	cout << "__LINE__ = " << __LINE__ << endl; //log
	cout << "byte_sequence.size() = " << byte_sequence.size() << endl; //log
	for ( auto v : byte_sequence ) {
		printf("%d ", v);
	}
	cout << endl;

	/* 4. 构建哈夫曼树 */
	Node *root = decode_hfmTree(tree_struct_code, byte_sequence);
	print_hfmTree(root);
	cout << "__LINE__ = " << __LINE__ << endl; //log

	/* 5. 根据哈夫曼树解压数据 */
	vector<unsigned char> ret;
	Node *curr = root;
	while ( pointer < file_len*8 - useless_bit_cnt ) {
		// cout << "pointer = " << pointer << endl; //log
		// cout << "get_by_bit(buf, pointer) = " << get_by_bit(buf, pointer) << endl; //log
		curr = get_by_bit(buf, pointer++) ? curr->rchild : curr->lchild;
		if (curr->lchild == NULL || curr->rchild==NULL) {
			// printf("curr->c = %d\n", curr->c);
			ret.push_back(curr->c);
			curr = root;
		}
	}
	return ret;
}

void linux_zip_file(const char *file_in, const char *file_out){
	/**
	 * Linux下，将file_in压缩后存储到file_out
	 */

	/* 1. 打开文件 */
	int fd = open(file_in, O_RDONLY);
	if (fd==-1) {
		throw string(file_in) + " in line " + to_string(__LINE__) + " error code = " + to_string(errno);
	}

	/* 2. 读取文件到缓冲区 */
	int file_len = lseek(fd, 0, SEEK_END); lseek(fd, 0, SEEK_SET);
	cout << "file_len = " << file_len << endl; //log
	unsigned char *buf = (unsigned char*)malloc(file_len);
	read(fd, buf, file_len);
	//log
	cout << "__LINE__ = " << __LINE__ << endl; //log
	// cout << "file_len = " << file_len << endl; //log
	// for (int i = 0; i < file_len; ++i){
	// 	putchar(buf[i]);
	// }
	

	/* 3. 执行压缩 */
	vector<unsigned char> out_buf = zip_process(buf, file_len);


	/* 4. 写入到输出文件 */
	int out_fd = open(file_out, O_CREAT|O_WRONLY);
	if (out_fd==-1) {
		throw string(file_out) + " in line " + to_string(__LINE__) + " error code = " + to_string(errno);
	}

	int n = write(out_fd, &out_buf[0], out_buf.size());
	if ( n<0 ) {
		throw string(file_out) + " in line " + to_string(__LINE__) + " error code = " + to_string(n);
	}

	/* 5. 关闭文件 */
	close(fd);
	close(out_fd);
	free(buf);
}

void linux_unzip_file(const char *file_in, const char *file_out){
	/**
	 * Linux下，将file_in解压后存储到file_out
	 */

	/* 1. 打开文件*/
	int fd = open(file_in, O_RDONLY);
	if (fd==-1) {
		throw string(file_in) + " in line " + to_string(__LINE__) + " error code = " + to_string(errno);
	}

	/* 2. 读取文件到buf */
	int file_len = lseek(fd, 0, SEEK_END); lseek(fd, 0, SEEK_SET);
	unsigned char *buf = (unsigned char*)malloc(file_len);
	read(fd, buf, file_len);

	/* 3. 解压到out_buf缓冲区 */
	vector<unsigned char> out_buf = unzip_process(buf, file_len);
	// cout << "__LINE__ = " << __LINE__ << endl; //log
	// cout << "out_buf.size() = " << out_buf.size() << endl; //log
	// for (auto i : out_buf ) {
	// 	printf("%c", i);
	// }
	cout << "-------------------------------------------------" << endl;
	/* 4. 写入到文件 */
	int out_fd = open(file_out, O_CREAT|O_WRONLY);
	cout << "out_fd = " << out_fd << endl; //log
	if (out_fd==-1) {
		throw string(file_out) + " in line " + to_string(__LINE__) + " error code = " + to_string(errno);
	}
	int n = write(out_fd, &out_buf[0], out_buf.size());
	if ( n<0 ) {
		throw string(file_out) + " in line " + to_string(__LINE__) + " error code = " + to_string(n);
	}

	/* 5. 关闭文件 */
	close(fd);
	close(out_fd);
	free(buf);
}

void windows_zip_file(const char *file_in, const char *file_out) {
	/**
	 * windows下，将file_in压缩后存储到file_out
	 */
	FILE *fp_in = fopen(file_in, "rb");
	if ( fp_in==NULL ) {
		throw string(file_in) + " in line " + to_string(__LINE__);
	}
	
	vector<unsigned char> buf;
	unsigned char ch;
	while(fread(&ch, 1, 1, fp_in)){
		buf.push_back(ch);
	}
	cout << "buf.size() = " << buf.size() << endl; //log
	vector<unsigned char> out_buf = zip_process(&buf[0], buf.size());

	FILE *fp_out = fopen(file_out, "wb");
	if ( fp_out==NULL ) {
		throw string(file_out) + " in line " + to_string(__LINE__);
	}
	fwrite(&out_buf[0], 1, out_buf.size(), fp_out);


	fclose(fp_in);
	fclose(fp_out);
}

void windows_unzip_file(const char *file_in, const char *file_out){
	/**
	 * windows下，将file_in解压后存储到file_out
	 */

	/* 1. 打开文件*/
	FILE *fp_in = fopen(file_in, "rb");
	if (fp_in==NULL) {
		throw "open file " + string(file_in) + " failed in line " + to_string(__LINE__);
	}
	cout << "fp_in = " << fp_in << endl; //log

	/* 2. 读取文件到buf */
	vector<unsigned char> buf;
	char ch;
	while ( fread(&ch, 1, 1, fp_in) ) {
		buf.push_back(ch);
	}
	cout << "buf.size() = " << buf.size() << endl; //log

	/* 3. 解压到out_buf缓冲区 */
	vector<unsigned char> out_buf = unzip_process(&buf[0], buf.size());


	/* 4. 写入到文件 */
	FILE *fp_out = fopen(file_out, "wb");
	if ( fp_out==NULL ) {
		throw "open file " + string(file_out) + " failed in line " + to_string(__LINE__);
	}
	int n = fwrite(&out_buf[0], 1, out_buf.size(), fp_out);
	cout << "n = " << n << endl; //log
	
	/* 5. 关闭文件 */
	fclose(fp_in);
	fclose(fp_out);
}

int main(int argc, char const *argv[]){
	// unsigned char buf[] = {1,2,3,1,2,3,4,5,6};
	// vector<unsigned char> out = zip_process(buf, sizeof buf);
	// vector<unsigned char> v = unzip_process(&out[0]);
	// for (auto i : v ) {
	// 	printf("%d\n", i);
	// }
	// return 0;
	#if 1
	freopen("out.txt", "w", stdout);
	unsigned char buf[1000];
	for (int i = 0; i < 1000; ++i){
		buf[i] = i*i;
	}
	vector<unsigned char> v = zip_process(buf, sizeof(buf)/sizeof(buf[0]));
	vector<unsigned char> res = unzip_process(&v[0], v.size());
	cout << "------------------------------------------------------------------------------------------" << endl;
	cout << "res.size() = " << res.size() << endl; //log
	bool right = true;
	for (int i = 0; i < 1000; ++i){
		right = right && res[i]==buf[i];
	}
	cout << "right = " << right << endl; //log
	fclose(stdout);
	system("out.txt");
	return 0;
	#endif

	// const char *a = "snake.mp4";
	// const char *b = "snake.mp4.zip";
	// const char *c = "snake0.mp4";

	const char *a = "a.txt";
	const char *b = "a.txt.zip";
	const char *c = "aa.txt";
	try{
		windows_zip_file(a, b);
		windows_unzip_file(b, c);
	}catch(string s) {
		cout << s << endl;
	}

	// unsigned char s[] = {1,1,1,2,2,3,4};
	// vector<unsigned char> v_char = zip_process(s, sizeof s);

	// for( auto v : v_char ) {
	// 	for (int j = 0; j < 8; ++j){
	// 		putchar(get_by_bit(&v, j)+'0');
	// 	}
	// 	cout << "" << endl;
	// }


	return 0;
}

/*

fileName_input -> fileName_output
zip [fileName_input] [fileName_output]

zip
请输入需要压缩的文件路径：
...
请输入压缩到的文件路径：
...





*/


