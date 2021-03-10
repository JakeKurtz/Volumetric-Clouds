#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

using namespace std;

struct VFB
{
    unsigned int V;
    bool A;
    bool F;
    bool B;
    bool Fa; //Not required in A#2
    bool Ba; //Not required in A#2
};

class EdgeBuffer {
public:
    vector<vector<VFB*>> buf;

    EdgeBuffer(int size) {
        buf = vector<vector<VFB*>>(size);
    }
    void insertEdge(int a, int b) {
        if (find(b, buf[a]) != -1) {
            return;
        }
        else {

            VFB* n = new VFB;
            n->V = b;
            n->F = 0;
            n->B = 0;

            buf[a].push_back(n);
            return;
        }
    }
    void updateEB(int a, int b, bool front) {
        auto x = find(b, buf[a]);
        if (x != -1) {
            auto e = buf[a][x];
            if (front) {
                e->F ^= (unsigned int) 1;
            }
            else {
                e->B ^= (unsigned int) 1;
            }
        }
    }
    void resetFrontBackBits() {
        for (int j = 0; j < buf.size(); j++) {
            for (auto n : buf[j]) {
                n->F = 0;
                n->B = 0;
            }
        }
    }

private:
    // searches for target vertex in edge list
    int find(int target, vector<VFB*> list) {
        for (int i = 0; i < list.size(); i++) {
            if (list[i]->V == target) return i;
        }
        return -1;
    }
};