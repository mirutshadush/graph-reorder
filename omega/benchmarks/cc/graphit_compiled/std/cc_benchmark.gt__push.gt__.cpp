#include <iostream> 
#include <vector>
#include <cstdio>
#include <algorithm>
#include "intrinsics.h"
#ifdef GEN_PYBIND_WRAPPERS
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
namespace py = pybind11;
#endif
Graph edges;
int  * __restrict IDs;
template <typename APPLY_FUNC > VertexSubset<NodeID>* edgeset_apply_push_parallel_deduplicatied_from_vertexset_with_frontier(Graph & g , VertexSubset<NodeID>* from_vertexset, APPLY_FUNC apply_func) 
{ 
    int64_t numVertices = g.num_nodes(), numEdges = g.num_edges();
    from_vertexset->toSparse();
    long m = from_vertexset->size();
    // used to generate nonzero indices to get degrees
    uintT *degrees = newA(uintT, m);
    // We probably need this when we get something that doesn't have a dense set, not sure
    // We can also write our own, the eixsting one doesn't quite work for bitvectors
    //from_vertexset->toSparse();
    {
        ligra::parallel_for_lambda((long)0, (long)m, [&] (long i) {
            NodeID v = from_vertexset->dense_vertex_set_[i];
            degrees[i] = g.out_degree(v);
         });
    }
    uintT outDegrees = sequence::plusReduce(degrees, m);
    if (g.get_flags_() == nullptr){
      g.set_flags_(new int[numVertices]());
      ligra::parallel_for_lambda(0, (int)numVertices, [&] (int i) { g.get_flags_()[i]=0; });
    }
    VertexSubset<NodeID> *next_frontier = new VertexSubset<NodeID>(g.num_nodes(), 0);
    if (numVertices != from_vertexset->getVerticesRange()) {
        cout << "edgeMap: Sizes Don't match" << endl;
        abort();
    }
    if (outDegrees == 0) return next_frontier;
    uintT *offsets = degrees;
    long outEdgeCount = sequence::plusScan(offsets, degrees, m);
    uintE *outEdges = newA(uintE, outEdgeCount);
  ligra::parallel_for_lambda((long)0, (long)m, [&] (long i) {
    NodeID s = from_vertexset->dense_vertex_set_[i];
    int j = 0;
    uintT offset = offsets[i];
    for(NodeID d : g.out_neigh(s)){
      if( apply_func ( s , d  ) && CAS(&(g.get_flags_()[d]), 0, 1)  ) { 
        outEdges[offset + j] = d; 
      } else { outEdges[offset + j] = UINT_E_MAX; }
      j++;
    } //end of for loop on neighbors
  },16);
  uintE *nextIndices = newA(uintE, outEdgeCount);
  long nextM = sequence::filter(outEdges, nextIndices, outEdgeCount, nonMaxF());
  free(outEdges);
  free(degrees);
  next_frontier->num_vertices_ = nextM;
  next_frontier->dense_vertex_set_ = nextIndices;
  ligra::parallel_for_lambda((int)0, (int)nextM, [&] (int i) {
     g.get_flags_()[nextIndices[i]] = 0;
  });
  return next_frontier;
} //end of edgeset apply function 
struct IDs_generated_vector_op_apply_func_0
{
void operator() (NodeID v) 
  {
    IDs[v] = (1) ;
  };
};
struct updateEdge
{
bool operator() (NodeID src, NodeID dst) 
  {
    bool output2 ;
    bool IDs_trackving_var_1 = (bool) 0;
    IDs_trackving_var_1 = writeMin( &IDs[dst], IDs[src] ); 
    output2 = IDs_trackving_var_1;
    return output2;
  };
};
struct init
{
void operator() (NodeID v) 
  {
    IDs[v] = v;
  };
};
int main(int argc, char * argv[])
{
  // Should match the core count in gem5
  setWorkers(16);

  edges = builtin_loadEdgesFromFile ( argv_safe((1) , argv, argc)) ;
  IDs = new int [ builtin_getVertices(edges) ];
  ligra::parallel_for_lambda((int)0, (int)builtin_getVertices(edges) , [&] (int vertexsetapply_iter) {
    IDs_generated_vector_op_apply_func_0()(vertexsetapply_iter);
  });;
  int n = builtin_getVertices(edges) ;
  for ( int trail = (0) ; trail < (1) ; trail++ )
  {
    startTimer() ;
    VertexSubset<int> *  frontier = new VertexSubset<int> ( builtin_getVertices(edges)  , n);
    ligra::parallel_for_lambda((int)0, (int)builtin_getVertices(edges) , [&] (int vertexsetapply_iter) {
      init()(vertexsetapply_iter);
    });;
    while ( (builtin_getVertexSetSize(frontier) ) != ((0) ))
    {
      frontier = edgeset_apply_push_parallel_deduplicatied_from_vertexset_with_frontier(edges, frontier, updateEdge()); 
    }
    float elapsed_time = stopTimer() ;
    std::cout << "elapsed time: "<< std::endl;
    std::cout << elapsed_time<< std::endl;
	FILE * fp;
    fp = fopen("log.txt", "w");
    if (fp == NULL ) std::cout << "Error opening file\n";
    else fprintf(fp, "%f", elapsed_time);
    if (fp != NULL) fclose(fp);
  }
};
#ifdef GEN_PYBIND_WRAPPERS
PYBIND11_MODULE(, m) {
}
#endif

