#include "filesystem.h"
#include <cstring>
class AVL{
	private:
	struct Node{
		entry_t data;
		Node* left;
		Node* right;
		uint32_t height;

		Node(entry_t entry){
			data=entry;
			strncpy(data.name,entry.name,sizeof(name_t));
			left=nullptr;
			right=nullptr;
			height = 0;
		};
	};

	Node* left_rotate(Node* parent){
		Node* g = parent->right;
		parent->right = g->left;
		g->left = parent;

		parent->height = 1 + max(parent->left->height, parent->right->height);
		g->height = 1 + max(g->left->height, g->right->height);
		return g;
	}
	Node* right_rotate(Node* parent){
		Node* g = parent->left;
		parent->left= g->right;
		g->right = parent;

		parent->height = 1 + max(parent->left->height, parent->right->height);
		g->height = 1 + max(g->left->height, g->right->height);
		return g;
	}


	Node* root;

	Node* Insert(Node* cur,Node* item);

	int get_balance_factor(Node*);

	void Sort(Node* cur, vector<entry_t>* nums ){
		if(cur!=nullptr){
			Sort(cur->left,nums);
			nums->push_back(cur->data);
			Sort(cur->right,nums);
		}
	}
	

	public:

	AVL();
	~AVL();


	entry_t Search(name_t name);
	
	
	void Insert(entry_t entry);

	//glorified in order print
	vector<entry_t> Sort(){
		vector<entry_t> sorted_entries;
		Sort(root, &sorted_entries);
		return sorted_entries;
	}



};
