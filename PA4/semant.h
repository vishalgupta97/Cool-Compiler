#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <vector>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE 1
#define FALSE 0

class ClassTable;
class TableNode;
class ClassMethod;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.


class ClassMethod {
 public: 
 ClassMethod(Symbol n,Class_ r):name(n),ret_type(r){}
  void add_arg(Symbol id,Class_ type){
    args.push_back(id);
    types.push_back(type);
  }
  bool is_fun(Symbol fun){
    return name->equal_string(fun->get_string(),fun->get_len());
  }
  void inject_to_env(SymTable *table){
    int size=args.size();
    for(int i=0;i<size;i++){
      table->addid(args[i],types[i]);
    }
  }
  Symbol arg_type(int i){
    if(i<=(int)types.size())
      return types[i]->get_name();
    else
      return NULL;
  }
  Symbol return_type(){
    return ret_type->get_name();
  }
  bool check_arg_type(Formals args){
    unsigned int i=args->first();
    for(;args->more(i);i=args->next(i)){
      if(i>=types.size())
	return false;
      Formal fml=args->nth(i);
      if(fml->get_type()!=types[i]->get_name())
	return false;
    }
    if(i==types.size())
      return true;
    else
      return false;
  }
 private:
  Symbol name;
  Class_ ret_type;
  std::vector<Symbol> args;
  std::vector<Class_> types;
};

class TableNode {
 public:
 TableNode():cls(NULL),children(std::vector<TableNode>()){}
 TableNode(Class_ c):cls(c),children(std::vector<TableNode>()){}
  void dump(ostream& stream,int n){}
  void add_child(Class_ c){
    children.push_back(TableNode(c));
  }
  Class_ get_class(){
    return cls;
  }
  Symbol get_name(){
    return cls->get_name();
  }
  TableNode* find_child(Symbol c){
    Symbol s=cls->get_name();
    if(c->equal_string(s->get_string(),s->get_len())){
      return this;
    } else {
      int size=children.size();
      for(int i=0;i<size;i++){
	TableNode* result=children[i].find_child(c);
	if(result) return result;
      }
    }
    return NULL;
  }
  TableNode* lub(Symbol a,Symbol b){
    TableNode* result=NULL;
    int size=children.size();
    for(int i=0;i<size;i++){
      TableNode* anode=children[i].find_child(a);
      TableNode* bnode=children[i].find_child(b);
      if(bnode && anode){
	result=&children[i];
	break;
      }
    }
    if(result==NULL)
      return this;
    else
      return result->lub(a,b);
  }
  void add_method(Symbol fun,Class_ ret){
    methods.push_back(ClassMethod(fun,ret));
  }
  void add_method_arg(Symbol fun,Symbol id,Class_ t){
    int size=methods.size();
    for(int i=0;i<size;i++){
      if(methods[i].is_fun(fun)){
	methods[i].add_arg(id,t);
      }
    }
  }
  Symbol find_arg_type(Symbol fun,int n){
    int size=methods.size();
    for(int i=0;i<size;i++){
      if(methods[i].is_fun(fun)){
	return methods[i].arg_type(n);
      }
    }
    return NULL;
  }
  Symbol find_ret_type(Symbol fun){
    int size=methods.size();
    for(int i=0;i<size;i++){
      if(methods[i].is_fun(fun)){
	return methods[i].return_type();
      }
    }
    return NULL;
  }
  bool check_arg_type(Symbol fun,Formals args){
    int size=methods.size();
    for(int i=0;i<size;i++){
      if(methods[i].is_fun(fun)){
	return methods[i].check_arg_type(args);
      }
    }
    return true;
  }
  void inject_to_env(Symbol fun,SymTable *table){
    int size=methods.size();
    for(int i=0;i<size;i++){
      if(methods[i].is_fun(fun)){
	methods[i].inject_to_env(table);
      }
    }
  }
 private:
  Class_ cls;
  std::vector<ClassMethod> methods;
  std::vector<TableNode> children;
};



class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  ostream& error_stream;
  TableNode root;

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
  ostream& err(){ return error_stream;}
  
  void add_class(Class_ c,Classes classes){
    if(c->get_parent()->equal_string("Bool",4)||c->get_parent()->equal_string("SELF_TYPE",9)||c->get_parent()->equal_string("String",6)){
      semant_error(c);
      err()<<"Class cannot inherits Bool,SELF_TYPE or Strings"<<endl;
      cerr<<"Compilation halted due to static semantic errors."<<endl;
      exit(1);
    }
    TableNode* parent=root.find_child(c->get_parent());
    if(parent==NULL&&classes){
      for(int i=classes->first();classes->more(i);i=classes->next(i)){
	Class_ cls=classes->nth(i);
	if(c->get_parent()==cls->get_name()){
	  add_class(cls,classes);
	  parent=root.find_child(c->get_parent());
	  break;
	}
      }
    }
    TableNode* existing=root.find_child(c->get_name());
    if(existing){
      return;
    }
    if(!is_root(c->get_name())){
      if(parent){
	parent->add_child(c);
      }
      else{
	semant_error(c);
	err()<<"Class inherits from invalid class"<<endl;
	cerr<<"Compilation halted due to static semantci errors."<<endl;
	exit(1);
      }
    }
  }
  void add_class_method(Class_ c,Symbol fun,Class_ ret){
    TableNode* node=root.find_child(c->get_name());
    if(node==NULL){
      semant_error(c);
      err()<<"Function return type is invalid"<<endl;
      return ;
    }
    node->add_method(fun,ret);
  }
  bool check_arg_type(Class_ c,Symbol fun,Formals args){
    TableNode* node=root.find_child(c->get_name());
    bool result=node->check_arg_type(fun,args);
    if(is_root(c->get_name())||result==false)
      return result;
    else
      return check_arg_type(root.find_child(c->get_parent())->get_class(),fun,args);
  }
  bool check_arg_name(Formals args){
    for(unsigned int i=args->first();args->more(i);i=args->next(i)){
      Formal fml1=args->nth(i);
      for(unsigned int j=args->first();args->more(j);j=args->next(j)){
	Formal fml2=args->nth(j);
	if(i!=j&&fml1->get_name()==fml2->get_name())
	  return false;
      }
    }
    return true;
  }
  void add_class_method_args(Class_ c,Symbol fun,Formals args){
    if(!is_root(c->get_name())){
      if(!check_arg_type(root.find_child(c->get_parent())->get_class(),fun,args)){
	semant_error(c);
	err()<<"Incompatible number of type of formal parameters in redefined function"<<endl;
	return ;
      }
      if(!check_arg_name(args)){
	semant_error(c);
	err()<<"Formal parameter is multiply defined"<<endl;
	return;
      }
    }
    TableNode* node=root.find_child(c->get_name());
    if(node==NULL){
      semant_error(c);
      err()<<"Argument type is invalid"<<endl;
      return;
    }
    for(int i=args->first();args->more(i);i=args->next(i)){
      Formal fml=args->nth(i);
      if(fml->get_name()->equal_string("self",4)){
	semant_error(c);
	err()<<"self cannot be the name of a formal parameter"<<endl;
	return;
      }
      if(fml->get_name()->equal_string("SELF_TYPE",9)){
	semant_error(c);
	err()<<"SELF_TYPE cannot be the name of a formal parameter"<<endl;
	return;
      }
      node->add_method_arg(fun,fml->get_name(),find_class(fml->get_type()));
    }
  }
  void register_methods(Class_ cls){
    Features methods=cls->get_features();
    for(int j=methods->first();methods->more(j);j=methods->next(j)){
      Feature method=methods->nth(j);
      if(method->is_method()){
	add_class_method(cls,method->get_name(),find_class(method->get_type()));
	add_class_method_args(cls,method->get_name(),method->get_formals());
      }
    }
  }
Symbol find_fun_arg_type(Class_ c, Symbol fun, int i) {
    TableNode* node = root.find_child(c->get_name());
    if (node==NULL)
      return NULL;
    Symbol ret = node->find_arg_type(fun, i);
    if (ret)
      return ret;
    if (!is_root(c->get_name())) {
      Class_ p = find_class(c->get_parent());
      return find_fun_arg_type(p, fun, i);
    }
    else
      return NULL;
  }
  Symbol find_ret_type(Class_ c, Symbol fun) {
       TableNode* node = root.find_child(c->get_name());
    if (node==NULL)
      return NULL;
    Symbol ret = node->find_ret_type(fun);
    if (ret)
      return ret;
    if (!is_root(c->get_name())) {
      Class_ p = find_class(c->get_parent());
      return find_ret_type(p, fun);
    }
    else
      return NULL;
  }
  void inject_to_env(Class_ c, Symbol fun, SymTable *table) {
    TableNode* node = root.find_child(c->get_name());
    if (node) {
      semant_error(c);
      return;
    }    
    node->inject_to_env(fun, table);
  }
  bool is_subtype(Symbol c, Symbol p) {
    TableNode* parent = root.find_child(p);
    assert(parent);
    if (parent->find_child(c) )
      return true;
    else
      return false;
  }
  bool is_root(Symbol c) {
    Symbol rn = root.get_name();
    return c->equal_string(rn->get_string(), rn->get_len());  
  }
  Class_ find_class(Symbol c) {
    TableNode* node = root.find_child(c);
    if (node)
      return node->get_class();
    else
      return NULL;
  }
  Class_ lub(Symbol a, Symbol b) {
    if (a==NULL || b==NULL)
      return NULL;
    return root.lub(a,b)->get_class();
  }
};




#endif

