
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if 0
#include <unistd.h>
#include <getopt.h>
#endif

#if !defined(AMIGA) && !defined(__mac_os)
#include <malloc.h>
#endif
#include <string.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#if defined(__mac_os)
#include <types.h>
#include <Speech.h>
#else
#include <sys/types.h>
#endif
#include "megahal.h"

void TREE::free_tree(TREE *tree)
{
    static int level=0;
    register int i;

    if(tree==NULL)
		return;

    if(tree->tree!=NULL) {
	if(level==0)
		intrinsics::progress("Freeing tree", 0, 1);
	for(i=0; i<tree->branch; ++i) {
	    ++level;
	    free_tree(static_cast<TREE*>(tree->tree[i]));
	    --level;
	    if(level==0)
			intrinsics::progress(NULL, i, tree->branch);
	}
	if(level==0)
		intrinsics::progress(NULL, 1, 1);
	free(tree->tree);
    }
    free(tree);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	New_Node
 *
 *		Purpose:		Allocate a new node for the n-gram tree, and initialise
 *						its contents to sensible values.
 */
TREE *TREE::new_node(void)
{
    TREE *node=NULL;

    /*
     *		Allocate memory for the new node
     */
    node=(TREE *)malloc(sizeof(TREE));
    if(node==NULL) {
		intrinsics::error("new_node", "Unable to allocate the node.");
	goto fail;
    }

    /*
     *		Initialise the contents of the node
     */
    node->symbol=0;
    node->usage=0;
    node->count=0;
    node->branch=0;
    node->tree=NULL;

    return(node);

fail:
    if(node!=NULL) free(node);
    return(NULL);
}

MODEL *MODEL::allocate()
{
	MODEL *instance;
	instance=(MODEL *)malloc(sizeof(MODEL));
	return instance;
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Add_Symbol
 *
 *		Purpose:		Update the statistics of the specified tree with the
 *						specified symbol, which may mean growing the tree if the
 *						symbol hasn't been seen in this context before.
 */
TREE *TREE::add_symbol(BYTE2 symbol)
{
	TREE *tree = this;
    TREE *node=NULL;

    /*
     *		Search for the symbol in the subtree of the tree node.
     */
    node=tree->find_symbol_add(symbol);

    /*
     *		Increment the symbol counts
     */
    if((node->count<65535)) {
	node->count+=1;
	tree->usage+=1;
    }

    return(node);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Find_Symbol
 *
 *		Purpose:		Return a pointer to the child node, if one exists, which
 *						contains the specified symbol.
 */
TREE *TREE::find_symbol(int symbol)
{
//  register
	int i;
	TREE *node = this;
    TREE *found=NULL;
    bool found_symbol=FALSE;

    /*
     *		Perform a binary search for the symbol.
     */
    i=node->search_node(symbol, &found_symbol);
    if(found_symbol==TRUE) found= static_cast<TREE*>(node->tree[i]);

    return(found);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Find_Symbol_Add
 *
 *		Purpose:		This function is conceptually similar to find_symbol,
 *						apart from the fact that if the symbol is not found,
 *						a new node is automatically allocated and added to the
 *						tree.
 */
TREE *TREE::find_symbol_add(BYTE2 symbol)
{
//  register
	int i;
	TREE *node = this;
    TREE *found=NULL;
    bool found_symbol=FALSE;

    /*
     *		Perform a binary search for the symbol.  If the symbol isn't found,
     *		attach a new sub-node to the tree node so that it remains sorted.
     */
    i=node->search_node(symbol, &found_symbol);
    if(found_symbol==TRUE) {
	found=static_cast<TREE*>(node->tree[i]);
    } else {
	found=new_node();
	found->symbol=symbol;
	node->add_node(found, i);
    }

    return(found);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Add_Node
 *
 *		Purpose:		Attach a new child node to the sub-tree of the tree
 *						specified.
 */
void TREE::add_node(TREE *node, int position)
{
	TREE *tree = this;
//  register
	int i;

    /*
     *		Allocate room for one more child node, which may mean allocating
     *		the sub-tree from scratch.
     */
    if(tree->tree==NULL) {
	tree->tree=(NODE **)malloc(sizeof(TREE *)*(tree->branch+1));
    } else {
	tree->tree=(NODE **)realloc((TREE **)(tree->tree),sizeof(TREE *)*
				    (tree->branch+1));
    }
    if(tree->tree==NULL) {
	intrinsics::error("add_node", "Unable to reallocate subtree.");
	return;
    }

    /*
     *		Shuffle the nodes down so that we can insert the new node at the
     *		subtree index given by position.
     */
    for(i=tree->branch; i>position; --i)
	tree->tree[i]=tree->tree[i-1];

    /*
     *		Add the new node to the sub-tree.
     */
    tree->tree[position]=node;
    tree->branch+=1;
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Search_Node
 *
 *		Purpose:		Perform a binary search for the specified symbol on the
 *						subtree of the given node.  Return the position of the
 *						child node in the subtree if the symbol was found, or the
 *						position where it should be inserted to keep the subtree
 *						sorted if it wasn't.
 */
int TREE::search_node(int symbol, bool *found_symbol)
{
//  register
	TREE *node = this;
	int position;
    int min;
    int max;
    int middle;
    int compar;

    /*
     *		Handle the special case where the subtree is empty.
     */
    if(node->branch==0) {
	position=0;
	goto notfound;
    }

    /*
     *		Perform a binary search on the subtree.
     */
    min=0;
    max=node->branch-1;
    while(TRUE) {
	middle=(min+max)/2;
	compar=symbol-node->tree[middle]->symbol;
	if(compar==0) {
	    position=middle;
	    goto found;
	} else if(compar>0) {
	    if(max==middle) {
		position=middle+1;
		goto notfound;
	    }
	    min=middle+1;
	} else {
	    if(min==middle) {
		position=middle;
		goto notfound;
	    }
	    max=middle-1;
	}
    }

found:
    *found_symbol=TRUE;
    return(position);

notfound:
    *found_symbol=FALSE;
    return(position);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Save_Tree
 *
 *		Purpose:		Save a tree structure to the specified file.
 */
void TREE::save_tree(FILE *file, TREE *node)
{
    static int level=0;
//  register
	int i;

    fwrite(&(node->symbol), sizeof(BYTE2), 1, file);
    fwrite(&(node->usage), sizeof(BYTE4), 1, file);
    fwrite(&(node->count), sizeof(BYTE2), 1, file);
    fwrite(&(node->branch), sizeof(BYTE2), 1, file);

    if(level==0)
		intrinsics::progress("Saving tree", 0, 1);
    for(i=0; i<node->branch; ++i) {
	++level;
	save_tree(file, static_cast<TREE*>(node->tree[i]));
	--level;
	if(level==0)
		intrinsics::progress(NULL, i, node->branch);
    }
    if(level==0)
		intrinsics::progress(NULL, 1, 1);
}

/*---------------------------------------------------------------------------*/

/*
 *		Function:	Load_Tree
 *
 *		Purpose:		Load a tree structure from the specified file.
 */
void TREE::load_tree(FILE *file)
{
	TREE *node = this;
    static int level=0;
//  register
	int i;

    fread(&(node->symbol), sizeof(BYTE2), 1, file);
    fread(&(node->usage), sizeof(BYTE4), 1, file);
    fread(&(node->count), sizeof(BYTE2), 1, file);
    fread(&(node->branch), sizeof(BYTE2), 1, file);

    if(node->branch==0)
		return;

    node->tree=(NODE **)malloc(sizeof(TREE *)*(node->branch));
    if(node->tree==NULL) {
		intrinsics::error("load_tree", "Unable to allocate subtree");
	return;
    }

    if(level==0)
		intrinsics::progress("Loading tree", 0, 1);
    for(i=0; i<node->branch; ++i)
	{
		node->tree[i]=new_node();
		++level;
		TREE *tree = static_cast<TREE*>(node->tree[i]);
		tree->load_tree(file);
		--level;
		if(level==0)
			intrinsics::progress(NULL, i, node->branch);
    }
    if(level==0)
		intrinsics::progress(NULL, 1, 1);
}
