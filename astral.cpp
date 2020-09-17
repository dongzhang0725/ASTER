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

#include "genetree.hpp"
#include "algorithms.hpp"


TripartitionInitializer tripInit;

unordered_map<string, string> leafname_mapping;
string TEXT;
int nodecnt = 0;
int pos = 0;
int K = 0;
int part = 0;
vector<string> names;
unordered_map<string, int> name2id;


string MAPPING(int begin, int end){
	string s;
	for (int i = begin; i < end && TEXT[i] != ':'; i++){
		if (TEXT[i] != '\"' && TEXT[i] != '\'') s += TEXT[i];
	}
	if (leafname_mapping.count(s)) return leafname_mapping[s];
	else return s;
}

void parseTaxa(){
	int i = pos;
	while (TEXT[pos] != '(' && TEXT[pos] != ',' && TEXT[pos] != ')') pos++;
	
	if (TEXT[pos] == '(') {
		while (TEXT[pos] != ')'){
			pos++;
			parseTaxa();
			while (TEXT[pos] != ')' && TEXT[pos] != ',') pos++;
		}
		pos++;
	}
	else {
		string name = MAPPING(i, pos);
		if (name2id.count(name) == 0){
			name2id[name] = names.size();
			names.push_back(name);
		}
	}
}

void parse(int parent = -1){
	int i = pos, cur;
	while (TEXT[pos] != '(' && TEXT[pos] != ',' && TEXT[pos] != ')') pos++;
	
	if (TEXT[pos] == '(') {
		cur = nodecnt++;
		while (TEXT[pos] != ')'){
			pos++;
			parse(cur);
			while (TEXT[pos] != ')' && TEXT[pos] != ',') pos++;
		}
		pos++;
	}
	else cur = name2id[MAPPING(i, pos)];
	if (parent != -1) tripInit.parentChild[part].back().push_back({parent, cur});
	else tripInit.roots[part].push_back(cur);
}

void readInputTrees(string input, string mapping) {
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
		parseTaxa();
		while (pos < TEXT.size() && TEXT[pos] != '(') pos++;
	}
	tripInit.nTaxa = names.size();
	pos = 0;
	while (pos < TEXT.size()){
		part = K % tripInit.roots.size();
		tripInit.parentChild[part].emplace_back();
		nodecnt = names.size();
		parse();
		while (pos < TEXT.size() && TEXT[pos] != '(') pos++;
		K++;
	}
}

string helpText = R"V0G0N(astral_feast [-o oFilePath -r nRound -s nSample -p probability -t nThread -a taxonNameMaps] inputGeneTrees
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
	string outputFile, mappingFile;
	ofstream fileOut;
	if (argc == 1) {cerr << helpText; return 0;}
	for (int i = 1; i < argc; i += 2){
		if (strcmp(argv[i], "-a") == 0) mappingFile = argv[i + 1];
		
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
		nPartitions = nThreads / nRounds;
		nThreads = nRounds;
	}
	for (int i = 0; i < nPartitions; i++){
		tripInit.roots.emplace_back();
		tripInit.parentChild.emplace_back();
	}
	readInputTrees(argv[argc - 1], mappingFile);
	
	/*
	for (auto e: tripInit.parentChild[0]){
		cerr << e.first << " ";
		if (e.second < tripInit.nTaxa) cerr << names[e.second] << endl; else cerr << e.second << endl;
	}
	*/
	
	
	cerr << "#Species: " << names.size() << endl;
	cerr << "#Genetrees: " << K << endl;
	cerr << "#Rounds: " << nRounds << endl;
	cerr << "#Samples: " << nSample << endl;
	cerr << "#Threads: " << nThreads << "x" << nPartitions << endl;
	cerr << "p = " << p << endl;
	
	ConstrainedOptimizationAlgorithm alg(names.size(), tripInit, names);
	auto res = alg.run(nRounds, nThreads);
	cerr << "Score: " << res.first/2 << endl;
	cerr << res.second << endl;
	
	res = alg.run(nSample, nThreads, p);
	cerr << "Score: " << res.first/2 << endl;
	fout << res.second << endl;
	
	//cerr << alg.printOptimalTreeWithScore() << endl;
	return 0;
}
