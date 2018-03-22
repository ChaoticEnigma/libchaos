#include "tests.h"
#include "zgraph.h"

namespace LibChaosTest {

void graph(){
    ZList<ZString> verts1 = {"A","B","C","D","E"};
    ZGraph<ZString, int> graph1(verts1);

    LOG("Vertices:");
    ZArray<ZString> verts = graph1.vertices();
    for(auto i = verts.begin(); i.more(); ++i){
        LOG(*i);
    }
    
    graph1.connect("A", "B", 1);
    
    TASSERT(graph1.connected("A", "B"));
}

ZArray<Test> graph_tests(){
    return {
        { "graph", graph, true, {} },
    };
}

}
