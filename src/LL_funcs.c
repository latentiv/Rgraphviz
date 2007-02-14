#include "common.h"
#include "util.h"
#include <stdio.h>

static inline Agraph_t* getAgraphPtr(SEXP graph)
{
    SEXP slotTmp = GET_SLOT(graph, Rf_install("agraph"));
    CHECK_Rgraphviz_graph(slotTmp);
    Agraph_t *g = R_ExternalPtrAddr(slotTmp);
    return g;
}

#define CLUSTERFLAG "cluster"

static Agraph_t* getClusterPtr(SEXP graph, SEXP cluster)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(NULL);

    int i = INTEGER(cluster)[0];
    char subGName[256];
    sprintf(subGName, "%s_%d", CLUSTERFLAG, i);

    Agraph_t *sg = agfindsubg(g, subGName);

    return(sg);
}

/*
 * TODO:
 * o. validate ptrs to graph/node/edge
 * -- getAttrs
 * o. handle non-exist attr
 * o. handle given default attr
 * -- setAttrs
 * o. check attr default/val
 * o. ??? validate attr default/val, e.g., red for color, 123 not for shape ???
 * o. call "agclose(g)" somewhere...
*/

static const Rgattr_t def_graph_attrs[] = {
            {"bgcolor",	"transparent"},
            {"fontcolor",	"black"},
            {"ratio",	"fill"},
            {"overlap",	""},
            {"splines",	"TRUE"},

            {"rank",	"same"},
            {"size",	"6.99, 6.99"},
            {"rankdir",	"TB"},	/* dot only */

            {NULL,		NULL}
        };

static const Rgattr_t def_cluster_attrs[] = {
            {"bgcolor",	"transparent"},
            {"color",	"black"},
            {"rank",	"same"},

            {NULL,		NULL}
        };

static const Rgattr_t def_node_attrs[] = {
            {"shape",	"circle"},
            {"fixedsize",	"TRUE"},
            {"fillcolor",	"transparent"},
            {"label",	""},
            {"color",	"black"},

            {"fontcolor",	"black"},
            {"fontsize",	"14"},
            {"height",	"0.5"},
            {"width",	"0.75"},

            {NULL,		NULL}
        };

static const Rgattr_t def_edge_attrs[] = {
            {"color",	"black"},
            {"dir",		"forward"},
            {"weight",	"1.0"},
            {"label",	""},
            {"fontcolor",	"black"},

            {"arrowhead",	"none"},
            {"arrowtail",	"none"},
            {"fontsize",	"14"},
            {"labelfontsize","11"},
            {"arrowsize",	"1"},

            {"headport",	"center"},
            {"layer",	""},
            {"style",	"solid"},
            {"minlen",	"1"},	/* dot only */
            {"len",	"1.0"},	/* neato only */

            {NULL, NULL}
        };

SEXP Rgraphviz_getDefAttrsGraph(SEXP graph)
{
    int nattr = 0;
    while ( def_graph_attrs[nattr].name ) nattr++;

    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    SEXP ans;
    PROTECT(ans = allocMatrix(STRSXP, nattr, 2));

    Agsym_t* sym;
    char* val;
    int i = 0, ii = 0;
    for ( i = 0, ii = 0; i < nattr; i++, ii++ )
    {
        sym = agfindattr(g, def_graph_attrs[i].name);
        val = sym? sym->value : NULL;
        if ( !val ) val = "ATTR_NOT_DEFINED";

        SET_STRING_ELT(ans, ii, mkChar(def_graph_attrs[i].name));
        SET_STRING_ELT(ans, nattr+ii, mkChar(val));

#if DEBUG
        if ( sym )
            printf(" attr name: %s \t attr val: %s \n", sym->name, sym->value);
#endif
    }
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_setDefAttrsGraph(SEXP graph, SEXP nnattr,
                                SEXP attrnames, SEXP attrvals)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    int nattr = INTEGER(nnattr)[0];
    int i;

    for ( i = 0; i < nattr; i++ )
        agraphattr(g, CHAR(STRING_ELT(attrnames, i)), CHAR(STRING_ELT(attrvals, i)));

    nattr = 0;
    while ( def_graph_attrs[nattr].name ) nattr++;

    for ( i = 0; i < nattr; i++ )
        if ( !agfindattr(g, def_graph_attrs[i].name) )
            agraphattr(g, def_graph_attrs[i].name, def_graph_attrs[i].value);

    return(R_NilValue);
}

SEXP Rgraphviz_getAttrsGraph(SEXP graph, SEXP attrname)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    char *val = agget(g, STR(attrname));

    if ( !val ) /* no such attr */
    {
        val = "N/A";
    }
    else if ( !strlen(val) ) /* attr defined but use default */
    {
        val = "Default";
    }

    SEXP ans;
    PROTECT(ans = allocVector(STRSXP, 1));
    SET_STRING_ELT(ans, 0, mkChar(val));
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_setAttrsGraph(SEXP graph,
                             SEXP attrname, SEXP attrval, SEXP default_val)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    /* 0 for success, -1 otherwise */
#if GRAPHVIZ_MAJOR == 2 && GRAPHVIZ_MINOR <= 7
    Agsym_t* a = agfindattr(g, STR(attrname));
    if ( !a ) a = agraphattr(g, STR(attrname), STR(default_val));
    int r = agset(g, STR(attrname), STR(attrval));
#else
    int r = agsafeset(g, STR(attrname), STR(attrval), STR(default_val));
#endif

    SEXP ans;
    PROTECT(ans = NEW_LOGICAL(1));
    LOGICAL(ans)[0] = r? FALSE : TRUE;
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_getDefAttrsCluster(SEXP graph, SEXP cluster)
{
    Agraph_t *sg = getClusterPtr(graph, cluster);
    if ( !sg ) return(R_NilValue);

    int nattr = 0;
    while ( def_cluster_attrs[nattr].name ) nattr++;

    SEXP ans;
    PROTECT(ans = allocMatrix(STRSXP, nattr, 2));

    Agsym_t* sym;
    char* val;
    int i = 0, ii = 0;
    for ( i = 0, ii = 0; i < nattr; i++, ii++ )
    {
        sym = agfindattr(sg, def_cluster_attrs[i].name);
        val = sym? sym->value : NULL;
        if ( !val ) val = "ATTR_NOT_DEFINED";

        SET_STRING_ELT(ans, ii, mkChar(def_cluster_attrs[i].name));
        SET_STRING_ELT(ans, nattr+ii, mkChar(val));
    }
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_setDefAttrsCluster(SEXP graph, SEXP cluster,
                                  SEXP nnattr, SEXP attrnames, SEXP attrvals)
{
    Agraph_t *sg = getClusterPtr(graph, cluster);
    if ( !sg ) return(R_NilValue);

    int nattr = INTEGER(nnattr)[0];
    int i;

    for ( i = 0; i < nattr; i++ )
    {
        Agsym_t *r = agraphattr(sg,
                                CHAR(STRING_ELT(attrnames, i)),
                                CHAR(STRING_ELT(attrvals, i)));
    }

    nattr = 0;
    while ( def_cluster_attrs[nattr].name ) nattr++;

    for ( i = 0; i < nattr; i++ )
        if ( !agfindattr(sg, def_cluster_attrs[i].name) ) 
        {
            Agsym_t *r = agraphattr(sg, 
		      def_cluster_attrs[i].name, def_cluster_attrs[i].value); 
        }

    return(R_NilValue);
}

SEXP Rgraphviz_getAttrsCluster(SEXP graph, SEXP cluster, SEXP attrname)
{
    Agraph_t *sg = getClusterPtr(graph, cluster);
    if ( !sg ) return(R_NilValue);

    char *val = agget(sg, STR(attrname));

    if ( !val ) /* no such attr */
    {
        val = "N/A";
    }
    else if ( !strlen(val) ) /* attr defined but use default */
    {
        val = "Default";
    }

    SEXP ans;
    PROTECT(ans = allocVector(STRSXP, 1));
    SET_STRING_ELT(ans, 0, mkChar(val));
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_setAttrsCluster(SEXP graph, SEXP cluster,
                               SEXP attrname, SEXP attrval, SEXP default_val)
{
    Agraph_t *sg = getClusterPtr(graph, cluster);
    if ( !sg ) return(R_NilValue);

    /* 0 for success, -1 otherwise */
#if GRAPHVIZ_MAJOR == 2 && GRAPHVIZ_MINOR <= 7
    Agsym_t* a = agfindattr(sg, STR(attrname));
    if ( !a ) a = agraphattr(sg, STR(attrname), STR(default_val));
    int r = agset(sg, STR(attrname), STR(attrval));
#else
    int r = agsafeset(sg, STR(attrname), STR(attrval), STR(default_val));
#endif

    SEXP ans;
    PROTECT(ans = NEW_LOGICAL(1));
    LOGICAL(ans)[0] = r? FALSE : TRUE;
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_getDefAttrsNode(SEXP graph)
{
    int nattr = 0;
    while ( def_node_attrs[nattr].name ) nattr++;

    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    Agnode_t *n = g->proto->n;

    SEXP ans;
    PROTECT(ans = allocMatrix(STRSXP, nattr, 2));

    Agsym_t* sym;
    char* val;
    int i = 0, ii = 0;
    for ( i = 0, ii = 0; i < nattr; i++, ii++ )
    {
        sym = agfindattr(n, def_node_attrs[i].name);
        val = sym? sym->value : NULL;
        if ( !val ) val = "ATTR_NOT_DEFINED";
        
        SET_STRING_ELT(ans, ii, mkChar(def_node_attrs[i].name));
        SET_STRING_ELT(ans, nattr+ii, mkChar(val));

#if DEBUG
        if ( sym )
            printf(" attr name: %s \t attr val: %s \n", sym->name, sym->value);
#endif
    }

    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_setDefAttrsNode(SEXP graph, SEXP nnattr,
                               SEXP attrnames, SEXP attrvals)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    int nattr = INTEGER(nnattr)[0];
    int i;
    for ( i = 0; i < nattr; i++ )
        agnodeattr(g, CHAR(STRING_ELT(attrnames, i)),
                   CHAR(STRING_ELT(attrvals, i)));

    nattr = 0;
    while ( def_node_attrs[nattr].name ) nattr++;

    for ( i = 0; i < nattr; i++ )
        if ( !agfindattr(g, def_node_attrs[i].name) )
            agnodeattr(g, def_node_attrs[i].name, def_node_attrs[i].value);

    return(R_NilValue);
}

SEXP Rgraphviz_getAttrsNode(SEXP graph, SEXP node, SEXP attrname)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    Agnode_t *n = agfindnode(g, STR(node));
    if ( !n ) return(R_NilValue);

    char *val = agget(n, STR(attrname));

    if ( !val ) /* no such attr */
    {
        val = "N/A";
    }
    else if ( !strlen(val) ) /* attr defined but use default */
    {
        val = "Default";
    }

    SEXP ans;
    PROTECT(ans = allocVector(STRSXP, 1));
    SET_STRING_ELT(ans, 0, mkChar(val));
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_setAttrsNode(SEXP graph, SEXP node,
                            SEXP attrname, SEXP attrval, SEXP default_val)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    Agnode_t *n = agfindnode(g, STR(node));
    if ( !n ) return(R_NilValue);

#if GRAPHVIZ_MAJOR == 2 && GRAPHVIZ_MINOR <= 7
    Agsym_t* a = agfindattr(n, STR(attrname));
    if ( !a ) a = agnodeattr(g, STR(attrname), STR(default_val));
    int r = agset(n, STR(attrname), STR(attrval));
#else
    int r = agsafeset(n, STR(attrname), STR(attrval), STR(default_val));
#endif

    SEXP ans;
    PROTECT(ans = NEW_LOGICAL(1));
    LOGICAL(ans)[0] = r? FALSE : TRUE;
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_getDefAttrsEdge(SEXP graph)
{
    int nattr = 0;
    while ( def_edge_attrs[nattr].name ) nattr++;

    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    Agedge_t *e = g->proto->e;

    SEXP ans;
    PROTECT(ans = allocMatrix(STRSXP, nattr, 2));

    Agsym_t* sym;
    char* val;
    int i = 0, ii = 0;
    for ( i = 0, ii = 0; i < nattr; i++, ii++ )
    {
        sym = agfindattr(e, def_edge_attrs[i].name);
        val = sym? sym->value : NULL;
        if ( !val ) val = "ATTR_NOT_DEFINED";

        SET_STRING_ELT(ans, ii, mkChar(def_edge_attrs[i].name));
        SET_STRING_ELT(ans, nattr+ii, mkChar(val));

#if DEBUG
        if ( sym )
            printf(" attr name: %s \t attr val: %s \n", sym->name, sym->value);
#endif
    }
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_setDefAttrsEdge(SEXP graph, SEXP nnattr,
                               SEXP attrnames, SEXP attrvals)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    int nattr = INTEGER(nnattr)[0];
    int i;

    for ( i = 0; i < nattr; i++ )
        agedgeattr(g, CHAR(STRING_ELT(attrnames, i)),
                   CHAR(STRING_ELT(attrvals, i)));

    nattr = 0;
    while ( def_edge_attrs[nattr].name ) nattr++;

    for ( i = 0; i < nattr; i++ )
        if ( !agfindattr(g, def_edge_attrs[i].name) )
            agedgeattr(g, def_edge_attrs[i].name, def_edge_attrs[i].value);

    return(R_NilValue);
}

SEXP Rgraphviz_getAttrsEdge(SEXP graph, SEXP from, SEXP to, SEXP attrname)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    Agnode_t *u = agfindnode(g, STR(from));
    Agnode_t *v = agfindnode(g, STR(to));
    if ( !u || !v ) return(R_NilValue);

    Agedge_t *e = agfindedge(g, u, v);
    if ( !e ) return(R_NilValue);

    char *val = agget(e, STR(attrname));

    if ( !val ) /* no such attr */
    {
        val = "N/A";
    }
    else if ( !strlen(val) ) /* attr defined but use default */
    {
        val = "Default";
    }

    SEXP ans;
    PROTECT(ans = allocVector(STRSXP, 1));
    SET_STRING_ELT(ans, 0, mkChar(val));
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_setAttrsEdge(SEXP graph, SEXP from, SEXP to,
                            SEXP attrname, SEXP attrval, SEXP default_val)
{
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    Agnode_t *u = agfindnode(g, STR(from));
    Agnode_t *v = agfindnode(g, STR(to));
    if ( !u || !v ) return(R_NilValue);

    Agedge_t *e = agfindedge(g, u, v);
    if ( !e ) return(R_NilValue);

#if GRAPHVIZ_MAJOR == 2 && GRAPHVIZ_MINOR <= 7
    Agsym_t* a = agfindattr(e, STR(attrname));
    if ( !a ) a = agedgeattr(g, STR(attrname), STR(default_val));
    int r= agset(e, STR(attrname), STR(attrval));
#else
    int r = agsafeset(e, STR(attrname), STR(attrval), STR(default_val));
#endif

    SEXP ans;
    PROTECT(ans = NEW_LOGICAL(1));
    LOGICAL(ans)[0] = r? FALSE : TRUE;
    UNPROTECT(1);
    return(ans);
}

SEXP Rgraphviz_toFile(SEXP graph, SEXP layoutType, SEXP filename, SEXP filetype)
{
#if GRAPHVIZ_MAJOR == 2 && GRAPHVIZ_MINOR >= 4
    Agraph_t *g = getAgraphPtr(graph);
    if ( !g ) return(R_NilValue);

    gvLayout(gvc, g, STR(layoutType));

#if GRAPHVIZ_MINOR == 4
    FILE *fp = fopen(STR(filename), "w");
    gvRender(gvc, g, STR(filetype), fp);
    fclose(fp);
#else
    gvRenderFilename(gvc, g, STR(filetype), STR(filename));
    gvFreeLayout(gvc, g);
#endif

#endif

    return(R_NilValue);
}

/*
 * g: graphNEL
 * nodes = nodes(g), 	strings
 * edges_from = edgeMatrix(g)["from",], edges_to = edgeMatrix(g)["to", ],  ints
 * nsubG = no. of subgraphs
 * subGIndex = subgraph-index for nodes, ints
 * recipK = combined reciprocal directed edges or not
*/
SEXP LLagopen(SEXP name, SEXP kind,
              SEXP nodes, SEXP subGIndex,
              SEXP edges_from, SEXP edges_to,
              SEXP nsubG, SEXP subGs,
              SEXP recipK)
{
    Agraph_t *g, *tmpGraph;
    Agraph_t **sgs;
    Agnode_t *head, *tail, *curNode;
    Agedge_t *curEdge;
    SEXP curSubG, curSubGEle;

    int recip = INTEGER(recipK)[0];
    char subGName[256];
    int ag_k = 0;
    int nsg = INTEGER(nsubG)[0];
    int i;
    int whichSubG;

    if ( length(edges_from) != length(edges_to) )
        error("length of edges_from must be equal to length of edges_to");

    if ( length(nodes) != length(subGIndex) )
        error("length of nodes must be equal to length of subGIndex");

    if (!isString(name)) error("name must be a string");

    if (!isInteger(kind)) error("kind must be an integer value");

    ag_k = INTEGER(kind)[0];
    if ((ag_k < 0)||(ag_k > 3))
        error("kind must be an integer value between 0 and 3");

    aginit();
    g = agopen(STR(name), ag_k);

    /* create subgraphs */
    sgs = (Agraph_t **)R_alloc(nsg, sizeof(Agraph_t *));
    if ( nsg > 0 && !sgs )
        error("Out of memory while allocating subgraphs");

    for (i = 0; i < nsg; i++) {
        curSubG = VECTOR_ELT(subGs, i);

        // First see if this is a cluster or not
        curSubGEle = getListElement(curSubG, CLUSTERFLAG);
        if ( curSubGEle == R_NilValue || LOGICAL(curSubGEle)[0] )
            sprintf(subGName, "%s_%d", CLUSTERFLAG, i+1);
        else
            sprintf(subGName, "%d", i+1);

        sgs[i] = agsubg(g, subGName);
    }

#if DEBUG
    printf(" nodes: ");
    for (i = 0; i < length(nodes); i++) {
        printf("%s ", CHAR(STRING_ELT(nodes, i)));
    }
    printf("\n");
    printf(" edges: ");
    for (i = 0; i < length(edges_from); i++) {
        printf("%d - %d     %s - %s \n",
               INTEGER(edges_from)[i],
               INTEGER(edges_to)[i],
               CHAR(STRING_ELT(nodes, INTEGER(edges_from)[i]-1)),
               CHAR(STRING_ELT(nodes, INTEGER(edges_to)[i]-1)));
    }
#endif

    /* create nodes */
    for (i = 0; i < length(nodes); i++) {
        whichSubG = INTEGER(subGIndex)[i];
        tmpGraph = whichSubG > 0 ? sgs[whichSubG-1] : g;

        curNode = agnode(tmpGraph, CHAR(STRING_ELT(nodes, i)));

        /*	printf(" node %d in subgraph %d \n", i, whichSubG); */
    }

    /* create the edges */
    char* node_f; char* node_t;
    for (i = 0; i < length(edges_from); i++) {
        node_f = CHAR(STRING_ELT(nodes, INTEGER(edges_from)[i]-1));
        node_t = CHAR(STRING_ELT(nodes, INTEGER(edges_to)[i]-1));

        tail = agfindnode(g, node_f);
        if ( !tail ) error("Missing tail node");

        head = agfindnode(g, node_t);
        if ( !head ) error("Missing head node");

        whichSubG = INTEGER(subGIndex)[INTEGER(edges_from)[i]-1];
        tmpGraph = whichSubG > 0 ? sgs[whichSubG-1] : g;

        /* recipEdges == "combined" in directed graph */
        if ( (ag_k == 1 || ag_k == 3 ) && recip == 0 &&
             (curEdge = agfindedge(tmpGraph, head, tail)) )
        {
#if GRAPHVIZ_MAJOR == 2 && GRAPHVIZ_MINOR <= 7
            Agsym_t* a = agfindattr(curEdge, "dir");
            if ( !a ) a = agedgeattr(g, "dir", "forward");
            int r= agset(curEdge, "dir", "both");
#else
            int r = agsafeset(curEdge, "dir", "both", "forward");
#endif
        }
        else
        {
            curEdge = agedge(tmpGraph, tail, head);
        }
    }

    return(buildRagraph(g));

}

