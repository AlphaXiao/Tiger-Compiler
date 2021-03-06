/*
 * graph.c - Functions to manipulate and create control flow and
 *           interference graphs.
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "errormsg.h"
#include "table.h"

struct G_graph_ {int nodecount;
		 G_nodeList mynodes, mylast;
		   };

struct G_node_ {
  G_graph mygraph;
  int mykey;
  G_nodeList succs;
  G_nodeList preds;
  void *info;
};

G_graph G_Graph(void)
{G_graph g = (G_graph) checked_malloc(sizeof *g);
 g->nodecount = 0;
 g->mynodes = NULL;
 g->mylast = NULL;
 return g;
}

int G_nodesNumber(G_graph g) {
  return g->nodecount;
}

int G_nodeKey(G_node node) {
  return node->mykey;
}

G_nodeList G_NodeList(G_node head, G_nodeList tail)
{G_nodeList n = (G_nodeList) checked_malloc(sizeof *n);
 n->head=head;
 n->tail=tail;
 return n;
}

/* generic creation of G_node */
G_node G_Node(G_graph g, void *info)
{G_node n = (G_node)checked_malloc(sizeof *n);
 G_nodeList p = G_NodeList(n, NULL);
 assert(g);
 n->mygraph=g;
 n->mykey=g->nodecount++;

 if (g->mylast==NULL)
   g->mynodes=g->mylast=p;
 else g->mylast = g->mylast->tail = p;

 n->succs=NULL;
 n->preds=NULL;
 n->info=info;
 return n;
}

G_nodeList G_nodes(G_graph g)
{
  assert(g);
  return g->mynodes;
} 

/* return true if a is in l list */
bool G_inNodeList(G_node a, G_nodeList l) {
  G_nodeList p;
  for(p=l; p!=NULL; p=p->tail)
	if (p->head==a) return TRUE;
  return FALSE;
}

void G_addEdge(G_node from, G_node to) {
  assert(from);  assert(to);
  assert(from->mygraph == to->mygraph);
  if (G_goesTo(from, to)) return;
  to->preds=G_NodeList(from, to->preds);
  from->succs=G_NodeList(to, from->succs);
}

static G_nodeList delete(G_node a, G_nodeList l) {
  assert(a && l);
  if (a==l->head) return l->tail;
  else return G_NodeList(l->head, delete(a, l->tail));
}

void G_rmEdge(G_node from, G_node to) {
  assert(from && to);
  to->preds=delete(from,to->preds);
  from->succs=delete(to,from->succs);
}

 /**
  * Print a human-readable dump for debugging.
  */
void G_show(FILE *out, G_nodeList p, void showInfo(void *)) {
  for (; p!=NULL; p=p->tail) {
	G_node n = p->head;
	G_nodeList q;
	assert(n);
	if (showInfo) 
	  showInfo(n->info);
	fprintf(out, " (%d): ", n->mykey); 
	for(q=G_succ(n); q!=NULL; q=q->tail) 
		   fprintf(out, "%d ", q->head->mykey);
	fprintf(out, "\n");
  }
}

G_nodeList G_succ(G_node n) { assert(n); return n->succs; }

G_nodeList G_pred(G_node n) { assert(n); return n->preds; }

bool G_goesTo(G_node from, G_node n) {
  return G_inNodeList(n, G_succ(from));
}

/* return length of predecessor list for node n */
static int inDegree(G_node n)
{ int deg = 0;
  G_nodeList p;
  for(p=G_pred(n); p!=NULL; p=p->tail) deg++;
  return deg;
}

/* return length of successor list for node n */
static int outDegree(G_node n)
{ int deg = 0;
  G_nodeList p; 
  for(p=G_succ(n); p!=NULL; p=p->tail) deg++;
  return deg;
}

int G_degree(G_node n) {return inDegree(n)+outDegree(n);}

/* put list b at the back of list a and return the concatenated list */
static G_nodeList cat(G_nodeList a, G_nodeList b) {
  if (a==NULL) return b;
  else return G_NodeList(a->head, cat(a->tail, b));
}

/* create the adjacency list for node n by combining the successor and 
 * predecessor lists of node n */
G_nodeList G_adj(G_node n) {return cat(G_succ(n), G_pred(n));}

void *G_nodeInfo(G_node n) {return n->info;}



/* G_node table functions */

G_table G_empty(void) {
  return TAB_empty();
}

void G_enter(G_table t, G_node node, void *value)
{
  TAB_enter(t, node, value);
}

void *G_look(G_table t, G_node node)
{
  return TAB_look(t, node);
}

/* My_G_NodeList functions */

My_G_nodeList My_Empty_G_nodeList() {
	My_G_nodeList list = (My_G_nodeList) checked_malloc(sizeof *list);;
	list->tail = list->head = NULL;
	return list;
}

// construct My_G_NodeList from G_nodeList
My_G_nodeList cloneFromGnodeList(G_nodeList list) {
    My_G_nodeList t1 = My_Empty_G_nodeList();
    while(list) {
        appendMyGnodeList(t1, list->head);
        list = list->tail;
    }
    return t1;
}


void appendMyGnodeList(My_G_nodeList list, G_node node) {
  	assert(list && node);
	if(list->head == NULL) {
		list->head = list->tail = G_NodeList(node, NULL);
	} else {
		list->tail->tail = G_NodeList(node, NULL);
		list->tail = list->tail->tail;
	}
}

// TODO, not tested yet
// append a G_node in the front of list
void insertFrontMyGnodeList(My_G_nodeList list, G_node node) {
    assert(list && node);
    if(list->head == NULL) {
        list->head = list->tail = G_NodeList(node, NULL);
    }  else {
        list->head = G_NodeList(node, list->head);
    }
}

My_G_nodeList cloneMyGnodeList(My_G_nodeList t1) {
	assert(t1);
	My_G_nodeList list = My_Empty_G_nodeList();
	G_nodeList now = t1->head;
	while(now) {
		appendMyGnodeList(list, now->head);
		now = now->tail;
	}
	return list;
}

int findInMyGnodeList(My_G_nodeList list, G_node t) {
	assert(list);
	G_nodeList now = list->head;
	while(now) {
		if(now->head == t) {
			return TRUE;
		}
		now = now->tail;
	}
	return FALSE;
}

// append a G_node in the end of list, if the node is already exist, then do nothing
void checkedAppendMyGnodeList(My_G_nodeList list, G_node node) {
    if(findInMyGnodeList(list, node) == FALSE)
        appendMyGnodeList(list, node);
}

My_G_nodeList unionMyGnodeList(My_G_nodeList t1, My_G_nodeList t2) {
	assert(t1); assert(t2);
	My_G_nodeList ret = cloneMyGnodeList(t1);
	G_nodeList now = t2->head;
	while(now) {
		if(findInMyGnodeList(t1, now->head) == FALSE) {
			appendMyGnodeList(ret, now->head);
		}
		now = now->tail;
	}
	return ret;
}

My_G_nodeList subMyGnodeList(My_G_nodeList t1, My_G_nodeList t2) {
	assert(t1); assert(t2);
	My_G_nodeList ret = My_Empty_G_nodeList();
	G_nodeList now = t1->head;
	while(now) {
		if(findInMyGnodeList(t2, now->head) == FALSE) {
			appendMyGnodeList(ret, now->head);
		}
		now = now->tail;
	}
	return ret;
}

// not tested yet
// determine whether My_Gnode_List is empty
bool emptyMyGnodeList(My_G_nodeList t1) {
    assert(t1);
    if(t1->head == NULL) {
        return TRUE;
    }
    return FALSE;
}


// not tested yet
// pop first element from My_G_nodeList 
G_node popMyGnodeList(My_G_nodeList t1) {
    assert(t1);
    if(emptyMyGnodeList(t1))
        return NULL;
    G_node ret = t1->head->head;
    t1->head = t1->head->tail;
    if(t1->head == NULL)
        t1->tail = NULL;
    return ret;
}

/* My_G_bitMatrix functions */

/* create a new bitMatrix, the element number is same to list*/
My_G_bitMatrix My_G_BitMatrix(int n) {
    My_G_bitMatrix bitMatrix = (My_G_bitMatrix)checked_malloc(sizeof *bitMatrix);
    bitMatrix->num = n;
    bitMatrix->matrix = (bool *)checked_malloc(n * n * sizeof(bool));
    int i = 0;
    for(; i < n*n; ++i) {
        (bitMatrix->matrix)[i] = FALSE;
    }
    return bitMatrix;
}

/* add edge in matrix, add t1->t2*/
void My_G_bitMatrixAdd(My_G_bitMatrix bitMatrix, int t1, int t2) {
    (bitMatrix->matrix)[t1*bitMatrix->num+t2] = TRUE;
}
/* remove edge in matrix, remove t1->t2*/
void My_G_bitMatrixRemove(My_G_bitMatrix bitMatrix, int t1, int t2) {
    (bitMatrix->matrix)[t1*bitMatrix->num+t2] = FALSE;
}
/* check whether t1 is connect with t2*/
bool My_G_bitMatrixIsConnect(My_G_bitMatrix bitMatrix, int t1, int t2) {
    return (bitMatrix->matrix)[t1*bitMatrix->num+t2] == TRUE;
}

