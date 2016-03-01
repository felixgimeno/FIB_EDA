#include "Player.hh"
#include <set>
#include <queue>
#include <vector>
#include <deque>
using namespace std;
#define PLAYER_NAME Freedom
struct PLAYER_NAME:public Player{
static Player *factory (){return new PLAYER_NAME;}
Dir dir;
bool modo_atacar;
//separar logica AI, de logica correctitud
//precalcular distancias desde beans, nuvols, capsules, for CPU and AI
//precalcular pos de beans, nuvols, capsules y hacer A* (si h es consist. hacemos Dijkstra, es h consist? h = manhattan distance to goal is monotone/consistent (the pathmax of it is itself)).
//hacer que bfs calcule distancia hasta ball
//hacer que vaya a atacar a un goku sin fuerza con bola
static int h(Pos w, Pos ww){return abs(w.i-ww.i)+abs(w.j -ww.j);}
int dist(Pos w, Dir d, Pos ww){return 1 + h(w+d,ww)-h(w,ww);}/*supossing w+d has no rock*/
Pos get_min(const Pos p, const vector<Magic_Bean>& P){
	if (P.empty()) return p;
	Pos q = p;
	for (Magic_Bean b: P) if (b.present) q = b.pos;
	int min_ = h(p,q);
	for (int i = 0; i < (int)P.size();++i) if ( (P[i].present or P[i].time < h(p,P[i].pos)) and p!= P[i].pos and h(p,P[i].pos)< min_ and cell(P[i].pos).type == Bean){min_=h(p,P[i].pos); q = P[i].pos;}
	return q;
	}
Pos get_min(const Pos p, const vector<Kinton_Cloud>& P){
	if (P.empty()) return p;
	Pos q = p;
	for (Kinton_Cloud b: P) if (b.present) q = b.pos;
	int min_ = h(p,q);
	for (int i = 0; i < (int)P.size();++i) if ( (P[i].present or P[i].time < h(p,P[i].pos)) and p!= P[i].pos and h(p,P[i].pos)< min_ and cell(P[i].pos).type == Kinton){min_=h(p,P[i].pos); q = P[i].pos;}
	return q;
	}	
CType ai (){
	int A_ = max_strength()/4-10*moving_penalty(); //max( , max(0/*res_strength()-15*moving_penalty()*/))
	//A_ = max_strength()/2;
	A_ = max_strength()/2; //--
	int B_ = max_strength()/3;
	CType c = ( goku(me()).strength < A_ and not has_kinton(goku(me()).type) and kinton_life_time() > max(rows(),cols())/3 ? Kinton : (goku(me()).strength < B_ ? Bean : (not has_ball(goku(me()).type) ? Ball : Capsule ) ) );
	if (round()>nb_rounds()-50) c = (not has_ball(goku(me()).type) ? Ball : Capsule ) ;
	bi(A_,B_);
	return c;
	}
void bi (int A_, int B_){
	desired.clear();
	A_ = max_strength();// ???
	B_ = max_strength()/2;// 多多多
	if (goku(me()).strength < A_ and not (has_kinton(goku(me()).type) and (float)goku(me()).kinton >= 0.1*(float) h(goku(me()).pos,get_min(goku(me()).pos,kintons()) ))  and kinton_life_time() > max(rows(),cols())/3) desired.insert(Kinton);
	if (goku(me()).strength < B_ ) desired.insert(Bean);
	if (not has_ball(goku(me()).type)) desired.insert(Ball);
	else desired.insert(Capsule);
	}	
Pos get_pos_max_goku(void){
	Pos max_ = goku(me()).pos;
	int st = goku(me()).strength;
	for (int i = 0; i < nb_players(); ++i) if (goku(i).strength > st and goku(i).alive) {max_ = goku(i).pos; st = goku(i).strength;}
	return max_;
	}	
virtual void play (){
	if (not goku(me()).alive and round() % 2 and not has_kinton(goku(me()).type)) return;
	if (round() == 0) {dir = rand_dir (None); debug = false; modo_atacar = false;}
	Pos p = goku(me()).pos;
	CType m = ai();
	if (m == Bean){
		dir = bfs(p, Bean,get_min(p,beans()),false /* or true*/); 
	} else if (m == Kinton){
		dir = bfs(p,Kinton,get_min(p,kintons()),false);
	} else {	
		if (modo_atacar) dir = bfs(p, m, get_pos_max_goku(), true);
		else dir = bfs(p, m,p,false); 
	}
	int n = 1+goku(me()).strength-kamehame_penalty();
	bool t = 3*n >= 1+max_strength()-kamehame_penalty()+10*moving_penalty() or modo_atacar;
	const vector < Dir > a = {Left, Right,Top,Bottom} ;
	for (Dir d : a){
		if (t and goku(me()).strength > kamehame_penalty() and valid_kame(d,p) ) throw_kamehame(d);
	}
	
	//if (round()<540 and round()>515 and debug) cerr << "dir is " << d2c(dir) << endl;
	//if (is_goku(p+dir) and str(p+dir)>goku(me()).strength) throw_kamehame(dir); //--
	move(dir);
}
bool can_move (Pos p, Dir d){
	if ( not pos_ok(dest(p,d)) or cell(dest(p, d)).type == Rock) return false;
	const int id = cell (dest (p, d)).id;
	
	if (id != -1 and id != me() and goku(id).alive and goku (me ()).strength < goku (id).strength) return false;
	if (id != -1 and id != me() and goku(id).alive) {
		const Goku g = goku(id);
		if ( 2*max( g.strength, combat_penalty() ) < goku(me()).strength or ( has_ball(g.type) and not has_ball(goku(me()).type) )) return true;
		//return false;
		}
	if (is_goku(p+d, modo_atacar) and not modo_atacar) return false;
	if (is_goku((p+d)+d,modo_atacar)) return false; //--
	return true;
}
Dir rand_dir (Dir notd){const vector < Dir > a = {Left, Right,Top,Bottom} ;while (true){Dir d = a[randomize () % 4];if (d != notd) return d;}}
	
bool debug;
map < Pos, Pos > visited;
struct piku {int dist; Pos goal;};
set<CType> desired;
Dir bfs (Pos p, CType c, Pos goal, bool reach){
	const vector < Dir > a = {Left, Right,Top,Bottom} ;
	deque <Pos> Q; 	Q.push_back (p);
	visited.clear(); visited[p] = p;
	set<Pos> L; L.insert(p);
	//map<Dir, piku> F;
	while (not Q.empty ()){
		Pos s = Q.front (); Q.pop_front ();
		for (Dir d : a){
			if (can_move (s, d)){ 
				if (L.find(dest(s,d)) != L.end() ) continue;
				//return d;///
				if (debug) cerr << "debug: d is " << d << " pos is " << s << endl;
				if (dest(s,d)==goal or (is_present(dest(s,d)) and (
					cell(dest(s,d)).type == c   or desired.find( cell(dest(s,d)).type) != desired.end() ))){
					//?多 vamos a lo guay que mas cerca este
					if (debug) cerr << "ball found" << s << endl;
					return get(s,d,visited,p);
				}
				if (not reach or dist(s,d,goal)!= 0) Q.push_back(dest (s, d));
				if (reach and dist(s,d,goal)==0) Q.push_front(dest(s,d));//like best first search 
				visited[dest (s, d)]= s;
				L.insert(dest(s,d));
		    }
	    }
	}
	
	if (desired.find(Bean) == desired.end() or desired.find(Kinton) == desired.end()) {
		desired.insert(Bean);desired.insert(Kinton);return bfs(p,c,goal,reach);
		}
	return None;
}
bool is_present(const Pos& j){
	CType c = cell(j).type;
	if (c == Ball or c == Capsule) return true;
	if (c == Bean) {
		for (Magic_Bean b : beans()) if (b.pos == j and (b.present)) return true;
		return false;
		}
	if (c == Kinton) {
		for (Kinton_Cloud b: kintons()) if (b.pos == j and b.present) return true;
		return false;
		}
	if (cell(j).id != -1 and has_ball(goku(cell(j).id).type)) return true; //? redundant
	return false;
	}
bool is_present(const Pos& j, const Pos& p){
	const CType c = cell(j).type;
	if (c == Ball or c == Capsule) return true;
	if (c == Bean) {
		for (Magic_Bean b : beans()) if (b.pos == j and (b.present or b.time < h(p,j))) return true;
		return false;
		}
	if (c == Kinton) {
		for (Kinton_Cloud b: kintons()) if (b.pos == j and (b.present or b.time < h(p,j))) return true;
		return false;
		}
	return false;
	}	
Dir get(Pos s, Dir d, map < Pos, Pos >& V,Pos p){
	const vector < Dir > a = {Left, Right,Top,Bottom} ;
	//return Left;
	while(s != p){
		for (Dir dd : a) if (V[s]+dd==s) d =dd;
		s = V[s];
		if (debug) cerr << "debug 2: s is:" << s << " p is " << p << " d is " << d << endl;
		}
	return d;	
	}
Dir get(Pos s, Dir d, map < Pos, Pos >& V,Pos p, int& Distance){
	const vector < Dir > a = {Left, Right,Top,Bottom} ;
	//return Left;
	while(s != p){
		for (Dir dd : a) if (V[s]+dd==s) d =dd;
		s = V[s];
		Distance++;
		if (debug) cerr << "debug 2: s is:" << s << " p is " << p << " d is " << d << endl;
		}
	return d;	
	}
bool valid_kame(Dir d, Pos p){
	for (Pos q = p + d ; pos_ok(q) == true and cell(q).type != Rock; q+=d){if (is_goku(q, true)) return true;}
	return false;
}
bool is_goku(Pos p, bool forDestroying){
	if (not pos_ok(p)) return false;
	for (int i = 0; i < nb_players(); ++i ){if (goku(i).alive and goku(i).pos == p and i != me() and (goku(i).strength>min(2*goku(me()).strength,kamehame_penalty()) or ( forDestroying and (goku(i).balls > 2*goku(me()).balls or has_ball(goku(i).type))) )) return true;}
	return false;
	}
bool pos_amenazada(Pos p){
	if (not pos_ok(p)) return false;
	for (Dir d : {Left,Right,Top,Bottom})
		for (Pos q = p + d ; pos_ok(q) == true and cell(q).type != Rock; q+=d)
			for (int i = 0; i < nb_players(); ++i ) if (goku(i).alive and goku(i).pos == p and i != me() and goku(i).strength > kamehame_penalty()) return true;
	return false;
}
int str(Pos p){
	for (int i = 0; i < nb_players(); ++i ){if (goku(i).pos == p and i != me()) return goku(i).strength;}
	return 0;	
	}
	
//--Precalcular dist,dir a bean,capsule, nuvol
struct chupi {int Dist; Dir dir;};
map<Pos,chupi> bean_map;
void make_bean_map(void){
	vector<Magic_Bean> B = beans();
	for (int i = 0; i < nb_beans(); ++i){
		//Pos p = B[i].pos;
		//To do .....
		}
	}	
//--	
};
RegisterPlayer (PLAYER_NAME);
