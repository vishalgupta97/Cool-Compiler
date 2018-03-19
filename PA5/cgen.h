#include <assert.h>
#include <stdio.h>
#include <vector>
#include "emit.h"
#include "cool-tree.h"
#include "symtab.h"

enum Basicness     {Basic, NotBasic};
#define TRUE 1
#define FALSE 0

class CgenClassTable;
//typedef CgenClassTable *CgenClassTableP;

class CgenNode;
//typedef CgenNode *CgenNodeP;

class CgenClassTable : public SymbolTable<Symbol,CgenNode> {
private:
  std::vector<CgenNode*> nds;
   ostream& str;
   int stringclasstag;
   int intclasstag;
   int boolclasstag;

// The following methods emit code for
// constants and global declarations.

   void code_global_data();
   void code_global_text();
   void code_bools(int);
   void code_select_gc();
   void code_constants();

// The following creates an inheritance graph from
// a list of classes.  The graph is implemented as
// a tree of `CgenNode', and class names are placed
// in the base class symbol table.

   void install_basic_classes();
   void install_class(CgenNode* nd);
   void install_classes(Classes cs);
   void build_inheritance_tree();
   void set_relations(CgenNode* nd);
   void collect_all_methods_impl(CgenNode* cls , 
				 std::vector<Symbol>* method_name, 
				 std::vector<Symbol>* class_name);
 public:
   CgenClassTable(Classes, ostream& str);
   void code();
   int size(CgenNode*);
   void emit_all_attrs(CgenNode* nd , ostream& str);
   void emit_all_methods(CgenNode* nd , ostream& str);
   int dispatch_table_offset(Symbol cls, Symbol fun);
   CgenNode* root();
   CgenNode* find_class(Symbol cls);
};


class CgenNode : public class__class {
private: 
   CgenNode* parentnd;                        // Parent of class
   std::vector<CgenNode*> children;                  // Children of class
   Basicness basic_status;                    // `Basic' if class is basic
                                              // `NotBasic' otherwise
  std::vector<Symbol> method_name;
   std::vector<Symbol> attr_offset;
public:
   CgenNode(Class_ c,
            Basicness bstatus,
            CgenClassTable* class_table);

   void add_child(CgenNode* child);
   std::vector<CgenNode*> get_children() { return children; }
   void set_parentnd(CgenNode* p);
   CgenNode* get_parentnd() { return parentnd; }
   int basic() { return (basic_status == Basic); }
   void code(ostream& str, CgenClassTable* env);
   void pre_fun(ostream& str);
   void post_fun(int num_arg, ostream& str);
   void emit_init_fun(ostream& str, CgenClassTable* env);
   std::vector<Symbol>* methods() { return &method_name; }
   void generate_attr_offset_impl(std::vector<Symbol>* table);
   void generate_attr_offset() { generate_attr_offset_impl(&attr_offset); }
   int find_offset(Symbol cls);
};

class BoolConst 
{
 private: 
  int val;
 public:
  BoolConst(int);
  void code_def(ostream&, int boolclasstag);
  void code_ref(ostream&) const;
};
