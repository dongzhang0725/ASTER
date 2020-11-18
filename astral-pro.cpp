#include<iostream>
#include<fstream>
#include<unordered_map>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<string>

using namespace std;

//#define LARGE_DATA
#ifdef LARGE_DATA
typedef __int128 score_t;

string to_string(const __int128 x){
	return to_string((double) x);
}

ostream& operator<<(ostream& cout, __int128 x){
	return cout << to_string(x);
}
#else
typedef long long score_t;
#endif

#include "multitree.hpp"
#include "algorithms.hpp"

const bool VERBOSE = true;

TripartitionInitializer tripInit;

unordered_map<string, string> leafname_mapping;
string TEXT;
int duploss = 0;
int nodecnt = 0;
int pos = 0;
int K = 0;
vector<string> id2name;
unordered_map<string, int> name2id;
	
class DynamicBitset{
	int size = 0;
	vector<uint64_t> vec;
	
public:
	DynamicBitset(){}
	DynamicBitset(int sz): size(sz), vec((sz + 63) / 64){}
	
	void set(int i){
		if (i >= size){
			size = i + 1;
			if ((size + 63) / 64 > vec.size()){
				vec.resize((size + 63) / 64);
			}
		}
		vec[i / 64] |= (1LL << (i % 64));
	}
	
	DynamicBitset operator|(const DynamicBitset &b) const{
		if (size < b.size) return b | *this;
		DynamicBitset res(size);
		for (int i = 0; i < b.vec.size(); i++){
			res.vec[i] = vec[i] | b.vec[i];
		}
		for (int i = b.vec.size(); i < vec.size(); i++){
			res.vec[i] = vec[i];
		}
		return res;
	}
	
	DynamicBitset operator&(const DynamicBitset &b) const{
		if (size < b.size) return b & *this;
		DynamicBitset res(b.size);
		for (int i = 0; i < b.vec.size(); i++){
			res.vec[i] = vec[i] & b.vec[i];
		}
		return res;
	}
	
	DynamicBitset operator^(const DynamicBitset &b) const{
		if (size < b.size) return b ^ *this;
		DynamicBitset res(size);
		for (int i = 0; i < b.vec.size(); i++){
			res.vec[i] = vec[i] ^ b.vec[i];
		}
		for (int i = b.vec.size(); i < vec.size(); i++){
			res.vec[i] = vec[i];
		}
		return res;
	}
	
	DynamicBitset operator-(const DynamicBitset &b) const{
		DynamicBitset res(size);
		for (int i = 0; i < vec.size(); i++){
			if (i < b.vec.size()) res.vec[i] = vec[i] & ~b.vec[i];
			else res.vec[i] = vec[i];
		}
		return res;
	}
	
	bool operator==(const DynamicBitset &b) const{
		if (size < b.size) return b == *this;
		for (int i = 0; i < b.vec.size(); i++){
			if (vec[i] != b.vec[i]) return false;
		}
		for (int i = b.vec.size(); i < vec.size(); i++){
			if (vec[i] != 0) return false;
		}
		return true;
	}
	
	bool operator!=(const DynamicBitset &b) const{
		return !(*this == b);
	}
	
	bool isDisjointTo(const DynamicBitset &b) const{
		if (size < b.size) return b.isDisjointTo(*this);
		for (int i = 0; i < b.vec.size(); i++){
			if ((vec[i] & b.vec[i]) != 0) return false;
		}
		return true;
	}
	
	vector<int> setBits() const{
		vector<int> res;
		for (int i = 0; i < vec.size(); i++){
			for (int j = 0; j < 64; j++){
				if (vec[i] & (1LL << j)) res.push_back(64 * i + j);
			}
		}
		return res;
	}
};

class GenetreeAnnotator{
private:
	struct Node{
		int leftChildId = -1, rightChildId = -1;
		DynamicBitset label;
		bool isDuplication = false;
		bool isLeaf = false;
		int score = -1;
		int leafId = -1;
	};
	
	vector<Node> node;
	
	tuple<int, int, int> createSubtree(const unordered_map<long long, string> &leafname, const unordered_map<long long, pair<long long, long long> > &children, const long long cur, vector<int> &rootId){
		if (children.count(cur) == 0){
			int curId = node.size();
			node.emplace_back();
			node[curId].isLeaf = true;
			node[curId].score = 0;
			if (name2id.count(leafname.at(cur)) == 0){
				name2id[leafname.at(cur)] = id2name.size();
				id2name.push_back(leafname.at(cur));
				for (int p = 0; p < tripInit.nodes.size(); p++){
					tripInit.leafParent[p].emplace_back();
				}
			}
			node[curId].leafId = name2id[leafname.at(cur)];
			node[curId].label.set(name2id[leafname.at(cur)]);
			return make_tuple(curId, -1, -1);
		}
		
		tuple<int, int, int> left = createSubtree(leafname, children, children.at(cur).first, rootId), right = createSubtree(leafname, children, children.at(cur).second, rootId);
		int cur0 = node.size();
		node.emplace_back();
		int cur1 = node.size();
		node.emplace_back();
		int cur2 = node.size();
		node.emplace_back();
		node[cur0].leftChildId = get<0>(left);
		node[cur0].rightChildId = get<0>(right);
		
		node[cur1].leftChildId = get<0>(left);
		node[cur2].leftChildId = get<0>(right);
		if (get<1>(left) != -1) node[get<1>(left)].rightChildId = cur2;
		if (get<2>(left) != -1) node[get<2>(left)].rightChildId = cur2;
		if (get<1>(right) != -1) node[get<1>(right)].rightChildId = cur1;
		if (get<2>(right) != -1) node[get<2>(right)].rightChildId = cur1;
		
		int root1 = node.size();
		node.emplace_back();
		node[root1].leftChildId = get<0>(left);
		node[root1].rightChildId = cur2;
		rootId.push_back(root1);
		int root2 = node.size();
		node.emplace_back();
		node[root2].leftChildId = get<0>(right);
		node[root2].rightChildId = cur1;
		rootId.push_back(root2);
		
		return make_tuple(cur0, cur1, cur2);
	}
	
	int scoreSubtree(int cur){
		if (node[cur].score != -1) return node[cur].score;
		node[cur].score = scoreSubtree(node[cur].leftChildId) + scoreSubtree(node[cur].rightChildId);
		node[cur].label = node[node[cur].leftChildId].label | node[node[cur].rightChildId].label;
		if (!node[node[cur].leftChildId].label.isDisjointTo(node[node[cur].rightChildId].label)){
			node[cur].score++;
			node[cur].isDuplication = true;
			if (node[cur].label != node[node[cur].leftChildId].label) node[cur].score++;
			if (node[cur].label != node[node[cur].rightChildId].label) node[cur].score++;
		}
		return node[cur].score;
	}
	
public:
	const vector<string> &leafnames() const{
		return id2name;
	}
	
	int annotateTree(const unordered_map<long long, string> &leafname, const unordered_map<long long, pair<long long, long long> > &children, const long long root){
		vector<int> rootId;
		tuple<int, int, int> left = createSubtree(leafname, children, children.at(root).first, rootId), right = createSubtree(leafname, children, children.at(root).second, rootId);
		if (get<1>(left) != -1) node[get<1>(left)].rightChildId = get<0>(right);
		if (get<2>(left) != -1) node[get<2>(left)].rightChildId = get<0>(right);
		if (get<1>(right) != -1) node[get<1>(right)].rightChildId = get<0>(left);
		if (get<2>(right) != -1) node[get<2>(right)].rightChildId = get<0>(left);
		
		int curId = node.size();
		node.emplace_back();
		node[curId].leftChildId = get<0>(left);
		node[curId].rightChildId = get<0>(right);
		rootId.push_back(curId);
		
		int bestscore = 999999, bestroot = -1;
		for (int root: rootId){
			int score = scoreSubtree(root);
			if (score < bestscore){
				bestscore = score;
				bestroot = root;
			}
		}
		duploss += bestscore;
		return bestroot;
	}
	
	int buildTree(int cur, int p) const{
		int w = tripInit.nodes[p].size();
		tripInit.nodes[p].emplace_back();
		
		if (node[cur].isLeaf){
			tripInit.leafParent[p][node[cur].leafId].push_back(w);
			//cerr << id2name[node[cur].leafId];
			return w;
		}
		int left = node[cur].leftChildId, right = node[cur].rightChildId;
		int large, small;
		if (node[left].label.setBits().size() > node[right].label.setBits().size()){
			large = left;
			small = right;
		}
		else {
			large = right;
			small = left;
		}
		//cerr << "(";
		int u = buildTree(small, p);
		//cerr << ",";
		int v = buildTree(large, p);
		//cerr << ")";
		tripInit.nodes[p][w].small = u;
		tripInit.nodes[p][w].large = v;
		tripInit.nodes[p][u].up = w;
		tripInit.nodes[p][v].up = w;
		if (node[cur].isDuplication){
			tripInit.nodes[p][w].dup = true;
			for (int i: (node[cur].label - node[large].label).setBits()){
				tripInit.leafParent[p][i].push_back(w);
			} //cerr << "+";
		}
		else {
			tripInit.nodes[p][w].dup = false;
		}
		return w;
	}
};

string MAPPING(int begin, int end){
	string s;
	for (int i = begin; i < end && TEXT[i] != ':'; i++){
		if (TEXT[i] != '\"' && TEXT[i] != '\'') s += TEXT[i];
	}
	if (leafname_mapping.count(s)) return leafname_mapping[s];
	else return s;
}

long long parse(unordered_map<long long, string> &leafname, unordered_map<long long, pair<long long, long long> > &children){
	int i = pos;
	long long cur;
	while (TEXT[pos] != '(' && TEXT[pos] != ',') pos++;
	if (TEXT[pos] == '(') {
		pos++;
		cur = parse(leafname, children);
		while (TEXT[pos] != ',') pos++;
	}
	else {
		cur = nodecnt++;
		leafname[cur] = MAPPING(i, pos);
	}
	while (TEXT[pos] != ')'){
		i = ++pos;
		while (TEXT[pos] != ')' && TEXT[pos] != '(' && TEXT[pos] != ',') pos++;
		if (TEXT[pos] == '(') {
			pos++;
			long long left = cur, right = parse(leafname, children);
			cur = nodecnt++;
			children[cur] = {left, right};
			while (TEXT[pos] != ',' && TEXT[pos] != ')') pos++;
		}
		else {
			long long left = cur, right = nodecnt++;
			leafname[right] = MAPPING(i, pos);
			cur = nodecnt++;
			children[cur] = {left, right};
		}
	}
	pos++;
	return cur;
}

void annotate(string input, string mapping){
	if (mapping != ""){
		ifstream fmap(mapping);
		string gname, sname;
		while (fmap >> gname){
			fmap >> sname;
			leafname_mapping[gname] = sname;
		}
	}
	ifstream fin(input);
	string line;
	while (getline(fin, line)) TEXT += line;
	while (pos < TEXT.size()){
		while (pos < TEXT.size() && TEXT[pos] != '(') pos++;
		if (pos < TEXT.size()) {
			pos++;
			unordered_map<long long, string> leafname;
			unordered_map<long long, pair<long long, long long> > children;
			long long root = parse(leafname, children);
			GenetreeAnnotator ga;
			int iroot = ga.annotateTree(leafname, children, root);
			ga.buildTree(iroot, K % tripInit.nodes.size());
			K++;
			if (VERBOSE && (K & 511) == 0) cerr << "Read " << K << " genetrees and found " << id2name.size() << " species.\n";
		}
	}
}

string helpText = R"V0G0N(astral-pro_feast [-c ConstraintPath -g GuidePath -o oFilePath -r nRound -s nSample -p probability -t nThread -a taxonNameMaps] inputGeneTrees
-c  path to constraint subtree file
-g  path to guide tree file
-o  path to output file (default: stdout)
-r  number of total rounds of placements (default: 5)
-s  number of total rounds of subsampling (default: 0)
-p  subsampling probability of keeping each taxon (default: 0.5)
-t  number of threads (default: 1)
-a  a list of gene name to taxon name maps, each line contains one gene name followed by one taxon name separated by a space or tab 
inputGeneTrees: the path to a file containing all gene trees in Newick format
)V0G0N";

int main(int argc, char** argv){
	int nThreads = 1, nRounds = 4, nSample = 0;
	bool phylip = false;
	double p = 0.5;
	string outputFile, mappingFile, guideFile, constraintFile, constraintTree;
	ofstream fileOut;
	if (argc == 1) {cerr << helpText; return 0;}
	for (int i = 1; i < argc; i += 2){
		if (strcmp(argv[i], "-a") == 0) mappingFile = argv[i + 1];
		
		if (strcmp(argv[i], "-c") == 0) constraintFile = argv[i + 1];
		if (strcmp(argv[i], "-g") == 0) guideFile = argv[i + 1];
		if (strcmp(argv[i], "-o") == 0) outputFile = argv[i + 1];
		if (strcmp(argv[i], "-r") == 0) sscanf(argv[i + 1], "%d", &nRounds);
		if (strcmp(argv[i], "-s") == 0) sscanf(argv[i + 1], "%d", &nSample);
		if (strcmp(argv[i], "-p") == 0) sscanf(argv[i + 1], "%lf", &p);
		if (strcmp(argv[i], "-t") == 0) sscanf(argv[i + 1], "%d", &nThreads);
		if (strcmp(argv[i], "-h") == 0) {cerr << helpText; return 0;}
	}
	ostream &fout = (outputFile == "") ? cout : fileOut;
	if (outputFile != "") fileOut.open(outputFile);
	
	int nPartitions = 1;
	if (nRounds < nThreads){
		nPartitions = (nRounds == 0) ? 1 : nThreads / nRounds;
		nThreads = nRounds;
	}
	for (int i = 0; i < nPartitions; i++){
		tripInit.nodes.emplace_back();
		tripInit.leafParent.emplace_back();
	}
	annotate(argv[argc - 1], mappingFile);
	
	cerr << "#Species: " << id2name.size() << endl;
	cerr << "#Genetrees: " << K << endl;
	cerr << "#Rounds: " << nRounds << endl;
	cerr << "#Samples: " << nSample << endl;
	cerr << "#Threads: " << nThreads << "x" << nPartitions << endl;
	cerr << "p = " << p << endl;
	
	cerr << "#Duploss: " << duploss << endl;
	
	ConstrainedOptimizationAlgorithm alg(id2name.size(), tripInit, id2name);
	
	if (guideFile != ""){
		ifstream fin(guideFile);
		string tree;
		while (getline(fin, tree)){
			alg.addGuideTree(tree, name2id);
		}
	}
	
	if (constraintFile != ""){
		ifstream fin(constraintFile);
		getline(fin, constraintTree);
	}
	
	auto res = (constraintTree == "") ? alg.run(nRounds, nThreads) : alg.constrainedRun(nRounds, nThreads, constraintTree, name2id);
	cerr << "Score: " << res.first << endl;
	cerr << res.second << endl;
	
	res = alg.run(nSample, nThreads, p);
	cerr << "Score: " << res.first << endl;
	fout << res.second << endl;
	
	cerr << "#EqQuartets: " << NUM_EQ_CLASSES << endl;
	//cerr << alg.printOptimalTreeWithScore() << endl;
	return 0;
}
