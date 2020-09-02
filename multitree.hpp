#include<vector>
#include<array>

using namespace std;

struct TripartitionInitializer{
	struct Node{
		int up = -1, small = -1, large = -1;
		bool dup = false;
	};
	
	vector<Node> nodes;
	vector<vector<int> > leafParent;
};

struct Tripartition{
	struct Node{
		score_t x = 0, y = 0, z = 0, tx = 0, ty = 0, tz = 0, q = 0;
		int version = 0, up = -1, small = -1, large = -1; // -1 for dummy!
		bool dup = false;
		
		void update(int v, int t){
			if (version != v){
				x = 0;
				y = 0;
				z = t;
				tx = 0;
				ty = 0;
				tz = 0;
				q = 0;
				version = v;
			}
		}
	};
	
	score_t normal(Node& w){
		Node& u = nodes[w.small];
		Node& v = nodes[w.large];
		u.update(version, totalZ[w.small]);
		v.update(version, totalZ[w.large]);
		w.tx = u.tx + v.tx + u.y * v.z * (u.y + v.z - 2) + u.z * v.y * (u.z + v.y - 2);
		w.ty = u.ty + v.ty + u.x * v.z * (u.x + v.z - 2) + u.z * v.x * (u.z + v.x - 2);
		w.tz = u.tz + v.tz + u.x * v.y * (u.x + v.y - 2) + u.y * v.x * (u.y + v.x - 2);
		score_t oldQ = w.q;
		w.q = u.x * v.tx + u.y * v.ty + u.z * v.tz + v.x * u.tx + v.y * u.ty + v.z * u.tz
			+ (u.x - 1) * u.x * v.y * v.z //+ (v.x - 1) * v.x * u.y * u.z
			+ (u.y - 1) * u.y * v.x * v.z //+ (v.y - 1) * v.y * u.x * u.z
			+ (u.z - 1) * u.z * v.x * v.y //+ (v.z - 1) * v.z * u.x * u.y
			;
		return w.q - oldQ;
	}
	
	void special(Node& w){
		Node& u = nodes[w.small];
		Node& v = nodes[w.large];
		u.update(version, totalZ[w.small]);
		v.update(version, totalZ[w.large]);
		w.tx = u.tx + v.tx;
		w.ty = u.ty + v.ty;
		w.tz = u.tz + v.tz;
	}
	
	vector<vector<int> > leafParent;
	score_t totalScore = 0;
	vector<Node> nodes;
	vector<score_t> totalZ;
	int version = 0;
	
	Tripartition(TripartitionInitializer init): leafParent(init.leafParent), nodes(init.nodes.size()), totalZ(init.nodes.size()){
		for (int i = 0; i < nodes.size(); i++){
			nodes[i].up = init.nodes[i].up;
			nodes[i].small = init.nodes[i].small;
			nodes[i].large = init.nodes[i].large;
			nodes[i].dup = init.nodes[i].dup;
		}
	}
	
	void reset(){
		totalScore = 0;
		version++;
	}
	
	void addTotal(int i){
		for (int u: leafParent[i]){
			totalZ[u]++;
			int w = nodes[u].up;
			while (w != -1 && (nodes[w].dup == false || nodes[w].large == u)){
				totalZ[w]++;
				u = w;
				w = nodes[u].up;
			}
		}
	}
	
	void add(int x, int i){
		for (int u: leafParent[i]){
			nodes[u].update(version, totalZ[u]);
			if (x == 0) nodes[u].z++; else if (x == 1) nodes[u].x++; else nodes[u].y++;
			int w = nodes[u].up;
			while (w != -1 && (nodes[w].dup == false || nodes[w].large == u)){
				nodes[w].update(version, totalZ[w]);
				if (x == 0) nodes[w].z++; else if (x == 1) nodes[w].x++; else nodes[w].y++;
				if (nodes[w].dup) special(nodes[w]); else totalScore += normal(nodes[w]);
				u = w;
				w = nodes[u].up;
			}
			if (w != -1){
				nodes[w].update(version, totalZ[w]);
				special(nodes[w]);
			}
		}
	}
	
	void rmv(int x, int i){
		for (int u: leafParent[i]){
			nodes[u].update(version, totalZ[u]);
			if (x == 0) nodes[u].z--; else if (x == 1) nodes[u].x--; else nodes[u].y--;
			int w = nodes[u].up;
			while (w != -1 && (nodes[w].dup == false || nodes[w].large == u)){
				nodes[w].update(version, totalZ[w]);
				if (x == 0) nodes[w].z--; else if (x == 1) nodes[w].x--; else nodes[w].y--;
				if (nodes[w].dup) special(nodes[w]); else totalScore += normal(nodes[w]);
				u = w;
				w = nodes[u].up;
			}
			if (w != -1){
				nodes[w].update(version, totalZ[w]);
				special(nodes[w]);
			}
		}
	}
	
	score_t score(){
		return totalScore / 2;
	}
};
