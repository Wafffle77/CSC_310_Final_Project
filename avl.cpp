#include "avl.h"
#include<iostream>
using namespace std;



entry_t AVL::Search(name_t key){
	Node* cur = root;

	while(cur!=nullptr && strncmp(cur->data.name,key, sizeof(name_t))!=0){
		if(strcmp(key,cur->data.name)<0){
			cur=cur->left;
		}
		else if(strcmp(key,cur->data.name)>0){
			cur=cur->right;
		}
	}
	return cur->data;
}
int AVL::get_balance_factor(Node* node){
	if(node==nullptr){
		return 0;
	}
	if(node->left==nullptr&&node->right==nullptr){
		return 0;
	}
	if(node->left!=nullptr&&node->right==nullptr){
		return -node->left->height;
	}
	if(node->left==nullptr&&node->right!=nullptr){
		return node->right->height;
	}
	return node->right->height - node->left->height;
}


AVL::AVL(){
	root=nullptr;
};

AVL::~AVL(){
	root=nullptr;
};


AVL::Node* AVL::Insert(Node* cur,Node* item){
	if(cur==nullptr){
		return item;
	}


	else if(strncmp(item->data.name,cur->data.name, sizeof(name_t))<0){
		cur->left = Insert(cur->left,item);
	}
	else if(strncmp(item->data.name,cur->data.name, sizeof(name_t))>0){
		cur->right = Insert(cur->right,item);
	}



	if(cur->right&&cur->left){
		cur->height = 1+ max( cur->right->height, cur-> left->height);
	}
	else if(cur->right){
		cur->height = 1+ cur->right->height;
	}
	else if(cur->left){
		cur->height = 1+ cur->left->height;
	}

	int bf = get_balance_factor(cur);
	//right-right case
	if(bf > 1 && get_balance_factor(cur->right)>1){
		return left_rotate(cur);
	}
	//left-left case
	if(bf < -1 && get_balance_factor(cur->left)<-1){
		return right_rotate(cur);
	}
	
	//right-left case
	if(bf > 1 && get_balance_factor(cur->right)<-1){
		cur->right=left_rotate(cur->right);
		return left_rotate(cur);
	}
	//left-right case
	if(bf < -1 && get_balance_factor(cur->right)>1){
		cur->left=right_rotate(cur->left);
		return right_rotate(cur);
	}

	return cur;
}



void AVL::Insert(entry_t item){
	Node* cur = new Node(item);
	root = Insert(root,cur);
}

AVL::Node* AVL::Remove(Node* cur,name_t name){
	if(cur==nullptr){
		return cur;
	}
	else if(strncmp(cur->data.name,name,sizeof(name_t))<0){
		cur->left=Remove(cur->left,name);
	}
	else if(strncmp(cur->data.name,name,sizeof(name_t))>0){
		cur->right=Remove(cur->right,name);
	}
	else{
		//one child and leaf cases
		if(!cur->left){
			Node* temp = cur->right;
			delete cur;
			return temp;
		}
		if(!cur->right){
			Node* temp = cur->left;
			delete cur;
			return temp;
		}


		//get smallest child in right subtree
		Node* parent = cur;
		Node* tmp = cur->right;
		while(tmp->left){
			parent=tmp;
			tmp=tmp->left;
		}
		cur->data = tmp->data;
		strncpy(cur->data.name,tmp->data.name,sizeof(name_t));

		cur->right = Remove(cur->right,tmp->data.name);
	}
	
	if(!cur){
		return cur;
	}
	
	//update heights and balace factors
	
	if(cur->right&&cur->left){
		cur->height = 1+ max( cur->right->height, cur-> left->height);
	}
	else if(cur->right){
		cur->height = 1+ cur->right->height;
	}
	else if(cur->left){
		cur->height = 1+ cur->left->height;
	}

	int bf = get_balance_factor(cur);
	//right-right case
	if(bf > 1 && get_balance_factor(cur->right)>1){
		return left_rotate(cur);
	}
	//left-left case
	if(bf < -1 && get_balance_factor(cur->left)<-1){
		return right_rotate(cur);
	}
	//right-left case
	if(bf > 1 && get_balance_factor(cur->right)<-1){
		cur->right=right_rotate(cur->right);
		return left_rotate(cur);
	}
	//left-right case
	if(bf < -1 && get_balance_factor(cur->right)>1){
		cur->left=left_rotate(cur->left);
		return right_rotate(cur);
	}

	return cur;

}
void AVL::Remove(name_t name){
	root = Remove(root,name);
}
