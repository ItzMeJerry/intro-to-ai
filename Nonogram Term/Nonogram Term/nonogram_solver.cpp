/*
 * nonogram_solver.cpp - Unified translation unit for Nonogram solver
 * 
 * This file consolidates all implementation files into a single translation unit
 * for simplified compilation. All headers remain separate for interface consistency.
 * 
 * Improvements:
 * - Renamed functions for better readability and clarity
 * - Consolidated all .cpp files (Hash.cpp, puzzle.cpp, Parsers.cpp, lineSolver.cpp, 
 *   Search.Solver.cpp, main.cpp) into one file
 * - Maintains all existing functionality and behavior
 */

#include <cstdio>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <queue>
#include <cmath>
#include <stack>
#include <algorithm>
#include <cstdio>


#define P_SIZE 25
#define S_SIZE P_SIZE * P_SIZE

#define STACK_MAX_DEPTH 626


/*use hash tables?*/
//#define _USE_QUEUE_

#define SOLVED 0
#define UNSOLVED 1
#define CONFLICT 2
#define TIMEOUT 3
#define MANY_SOLUTION 4

#define MODE 1
#define INPUT_FILE "input.txt"
#define OUTPUT_FILE "solution.txt"
#define LOG_FILE "log.txt"
#define LIGHT_NODE_LIMITED 15000
#define HEAVT_NODE_LIMITED 60000

#define SCHDULE_NUM_QUESTIONS 10



typedef unsigned long long int LineMask;


#define SHIFT_R(a,s) ( (LineMask)a>>((s)*2) )

#define SHIFT_L(a,s) ( (LineMask)a<<((s)*2) )

#define MASK50 1125899906842623ULL /*00000001111111111111111111111111111111*/


// Optimized popcount for 64-bit values - now constexpr for compile-time evaluation
[[nodiscard]] constexpr inline unsigned int count64(LineMask block) noexcept {
	block -= ((block >> 1) & 0x5555555555555555ULL);
	block = ((block >> 2) & 0x3333333333333333ULL) + (block & 0x3333333333333333ULL);
	return ((block + (block >> 4) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL) >> 56;
}

#define COUNT64(x) ( count64(x & MASK50) )


// Sizeof( MyQueue ) = 268 bytes
struct MyQueue {
public:
	inline void initial() noexcept {
		memset(inQ, 0, sizeof(inQ));
		f_pos = 0;
		r_pos = 0;
		max_size = P_SIZE * 2 + 1;
	}

	[[nodiscard]] inline bool isInQ(int index) const noexcept {
		return inQ[index];
	}

	inline void setInQ(int index, bool value) noexcept {
		inQ[index] = value;
	}

	inline void pushQ(int index) noexcept {
		if (inQ[index])
			return;
		dataQ[r_pos] = index;
		r_pos = (r_pos + 1) % max_size;
		setInQ(index, true);
	}

	[[nodiscard]] inline unsigned char popAndFront() noexcept {
		unsigned char uFront = dataQ[f_pos];
		f_pos = (f_pos + 1) % max_size;
		setInQ(uFront, false);
		return uFront;
	}

	[[nodiscard]] inline bool isEmpty() const noexcept {
		return r_pos == f_pos;
	}

	inline void clear() noexcept {
		memset(inQ, 0, sizeof(inQ));
		f_pos = 0;
		r_pos = 0;
	}

public:
	bool inQ[P_SIZE * 2];
	int dataQ[P_SIZE * 2 + 1];
	unsigned int f_pos;
	unsigned int r_pos;
	unsigned int max_size;
};

// Sizeof( LineNumbers ) = 64 bytes
struct LineNumbers {
	unsigned char mCount;
	unsigned char mNumbers[P_SIZE + 2];
	unsigned char mSum[P_SIZE + 2];
	LineMask hashKey;
};

// Sizeof( Puzzle ) = 3200 bytes
struct Puzzle {
	Puzzle();

	Puzzle(const char *);

	~Puzzle();

	LineNumbers mLines[2 * P_SIZE];
};

class NodeQueue {
public:
	static MyQueue myQ;
};

// Sizeof( Board ) = 408 bytes
struct Board {
	short numOfSquareOnBoard;
	LineMask colString[P_SIZE];
	LineMask rowString[P_SIZE];
	//LinkedList unknownList ;
};


// Optimized with noexcept - no dynamic memory allocation
inline void setBoardSquare(int x, int y, Board &sol, LineMask iSet) noexcept {
	sol.colString[x] &= ~(SHIFT_L(iSet, y));
	sol.rowString[y] &= ~(SHIFT_L(iSet, x));
}


#define SQUARE_BLOCK 1
#define SQUARE_SPACE 2
#define SQUARE_UNKNOWN 3
#define SQUARE 3

[[nodiscard]] constexpr inline int getBoardSquare(int x, int y, const Board &sol) noexcept {
	return (int) (SHIFT_R(sol.rowString[y], x) & SQUARE);
}

inline void initializeBoard(Board &sol) noexcept {
	memset(&sol, 0xff, sizeof(Board));
	sol.numOfSquareOnBoard = 0;
	NodeQueue::myQ.initial();
	//system( "pause" ) ;
	for (int i = 0; i < P_SIZE * 2; i++)
		NodeQueue::myQ.pushQ(i);
}

void printBoardSolution(const Board &sol);

void parseLineNumbers(Puzzle &quest, const char *sInput);


inline void setSquareAndNotify(int i, int j, Board &sol, int toSet) noexcept {
	setBoardSquare(i, j, sol, toSet);
	++sol.numOfSquareOnBoard;
	NodeQueue::myQ.pushQ(i);
	NodeQueue::myQ.pushQ(j + P_SIZE);
};


void parseInputFile(char *path, Puzzle *quests);

[[nodiscard]] int lineSolver(const Puzzle &prob, Board &sol) noexcept;


[[nodiscard]] bool solveLineConstraint(const LineNumbers &des, LineMask inString, int i, int j, LineMask &settleString,
				  unsigned char *Dtable = NULL) noexcept;

// Sizeof( DependencyTable ) = 5032 bytes
struct DependencyTable {
	LineMask pointTables[P_SIZE][P_SIZE];
	LineMask updateTable;
	LineMask tempUpdateTable;
	LineMask thisUpdateTable;
	LineMask thisTempTable;
};

using namespace std ;


static Board ifGuessBlackBoard[P_SIZE][P_SIZE];
static Board ifGuessWhiteBoard[P_SIZE][P_SIZE];
extern DependencyTable dbTable;
extern int NUM_OF_QUESTIONS;

struct SquareToGo {
    Board firstGo;
    Board secondGo;
};

// Inline helper to reduce call stack depth - tries both guesses for a square
// Returns SOLVED, CONFLICT, or UNSOLVED
inline int tryBothSquareStates(const Puzzle &problem,
                          Board &correctBoard,
                          Board &ifGuessBlackBoard,
                          Board &ifGuessWhiteBoard,
                          int x,
                          int y
) noexcept {
    Board resumeBoard = correctBoard;
    setSquareAndNotify(x, y, correctBoard, SQUARE_SPACE);

    dbTable.thisTempTable = 0;
    int state = lineSolver(problem, correctBoard);
    const LineMask spaceTable = dbTable.thisTempTable;
    dbTable.thisUpdateTable |= spaceTable;

    if (state == SOLVED)
        return SOLVED;

    if (state == CONFLICT) {
        correctBoard = resumeBoard;
        setSquareAndNotify(x, y, correctBoard, SQUARE_BLOCK);
        dbTable.thisTempTable = 0;
        state = lineSolver(problem, correctBoard);
        dbTable.tempUpdateTable |= dbTable.thisTempTable;

        if (state == SOLVED)
            return SOLVED;
        if (state == CONFLICT)
            return CONFLICT;
    } else {
        ifGuessWhiteBoard = correctBoard;
        correctBoard = resumeBoard;
        setSquareAndNotify(x, y, correctBoard, SQUARE_BLOCK);
        dbTable.thisTempTable = 0;
        state = lineSolver(problem, correctBoard);
        dbTable.thisUpdateTable |= dbTable.thisTempTable;

        if (state == SOLVED)
            return SOLVED;

        if (state == CONFLICT) {
            correctBoard = ifGuessWhiteBoard;
            dbTable.tempUpdateTable |= spaceTable;
        } else {
            ifGuessBlackBoard = correctBoard;
            correctBoard = resumeBoard;

            bool isUpdate = false;
            for (int i = 0; i < P_SIZE; ++i) {
                const LineMask oneRow = ifGuessBlackBoard.rowString[i] | ifGuessWhiteBoard.rowString[i];
                if (oneRow != correctBoard.rowString[i]) {
                    isUpdate = true;
                    correctBoard.numOfSquareOnBoard += count64((correctBoard.rowString[i] ^ oneRow));
                    correctBoard.rowString[i] = correctBoard.rowString[i] & oneRow;
                    NodeQueue::myQ.pushQ(i + P_SIZE);
                }

                const LineMask oneCol = ifGuessBlackBoard.colString[i] | ifGuessWhiteBoard.colString[i];
                if (oneCol != correctBoard.colString[i]) {
                    isUpdate = true;
                    correctBoard.colString[i] = correctBoard.colString[i] & oneCol;
                    NodeQueue::myQ.pushQ(i);
                }
            }

            if (isUpdate) {
                dbTable.thisTempTable = 0;
                state = lineSolver(problem, correctBoard);
                dbTable.tempUpdateTable |= dbTable.thisTempTable;

                if (state == SOLVED)
                    return SOLVED;
                if (state == CONFLICT)
                    return CONFLICT;
            }
        }
    }

    return UNSOLVED;
}

// Optimized 2-SAT solver with reduced stack depth
// All helper functions are inline to minimize call overhead
inline int solveWithTwoSat(const Puzzle &problem,
                        Board &correctBoard,
                        SquareToGo &whereCanIgo
) noexcept {
    Board resumeBoard;
    dbTable.updateTable = 0;
    while (true) {
        dbTable.tempUpdateTable = 0;
        resumeBoard = correctBoard;
        for (int i = 0; i < P_SIZE; ++i) {
            for (int j = 0; j < P_SIZE; ++j) {
                if (getBoardSquare(i, j, correctBoard) == SQUARE_UNKNOWN && (
                        (dbTable.updateTable & dbTable.pointTables[i][j]) || (dbTable.updateTable == 0))
                ) {
                    dbTable.thisUpdateTable = 0;
                    const int state = tryBothSquareStates(problem,
                                               correctBoard,
                                               ifGuessBlackBoard[i][j],
                                               ifGuessWhiteBoard[i][j],
                                               i,
                                               j
                    );
                    if (state == SOLVED)
                        return SOLVED;
                    if (state == CONFLICT)
                        return CONFLICT;
                    dbTable.pointTables[i][j] = dbTable.thisUpdateTable;
                }
            }
        }
        dbTable.updateTable |= dbTable.tempUpdateTable;
        if (dbTable.tempUpdateTable == 0) {
            int x = 0;
            int y = 0;
            double maxScore = -1.0;
            for (int i = 0; i < P_SIZE; ++i) {
                for (int j = 0; j < P_SIZE; ++j) {
                    if (getBoardSquare(i, j, correctBoard) == SQUARE_UNKNOWN) {
                        const double score = min(ifGuessBlackBoard[i][j].numOfSquareOnBoard,
                                           ifGuessWhiteBoard[i][j].numOfSquareOnBoard) +
                                       1.85 * log(1.0 + static_cast<double>(abs(
                                                      ifGuessBlackBoard[i][j].numOfSquareOnBoard -
                                                      ifGuessWhiteBoard[i][j].numOfSquareOnBoard)));

                        if (score > maxScore) {
                            x = i;
                            y = j;
                            maxScore = score;
                        }
                    }
                }
            }

            whereCanIgo.firstGo = ifGuessWhiteBoard[x][y];
            whereCanIgo.secondGo = ifGuessBlackBoard[x][y];

            return UNSOLVED;
        }
    }
}

// Iterative DFS search to minimize stack depth
[[nodiscard]] int searchWithDepthFirstSearch(const Puzzle &problem,
                                   Board &solution,
                                   int &nodeCount
) noexcept;

// Main entry point for solving a puzzle
[[nodiscard]] int solvePuzzle(const Puzzle &problem,
                                     Board &solution,
                                     int &nodeCount
) noexcept;

#define HTABLE_SIZE 9999991ULL


struct hashNode {
	LineNumbers lineProblem;
	LineMask nowString;
	LineMask settleString;
};


void initializeHashTable() noexcept;

void insertHashEntry(const LineNumbers &problem, const LineMask &nowString, const LineMask &settleString) noexcept;

[[nodiscard]] bool findHashEntry(const LineNumbers &problem, const LineMask &nowString, LineMask &settleString) noexcept;

using namespace std;

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

LineMask zHashKeyTable[13][26];
static hashNode hashTable[HTABLE_SIZE];
MyQueue NodeQueue::myQ;
unsigned char DPtable[P_SIZE + 1][(P_SIZE + 1) / 2];
DependencyTable dbTable;

constexpr const char* INPUT_FILENAME = INPUT_FILE;
constexpr const char* OUTPUT_FILENAME = OUTPUT_FILE;
constexpr const char* LOG_FILENAME = LOG_FILE;
int NUM_OF_QUESTIONS;

// =============================================================================
// HASH TABLE IMPLEMENTATION
// =============================================================================

void initializeHashTable() noexcept {
	std::memset(hashTable, 0, sizeof(hashNode) * HTABLE_SIZE);
	zHashKeyTable[0][0] = 759055707854222535ULL;
	zHashKeyTable[0][1] = 11872126079661544008ULL;
	zHashKeyTable[0][2] = 10666341186357044533ULL;
	zHashKeyTable[0][3] = 14271371299812038426ULL;
	zHashKeyTable[0][4] = 12410598966417316021ULL;
	zHashKeyTable[0][5] = 4133490037383340789ULL;
	zHashKeyTable[0][6] = 1506965450440365256ULL;
	zHashKeyTable[0][7] = 16303698503260591679ULL;
	zHashKeyTable[0][8] = 18194211981394086425ULL;
	zHashKeyTable[0][9] = 10856133602674542089ULL;
	zHashKeyTable[0][10] = 17959415083258366339ULL;
	zHashKeyTable[0][11] = 16996574565479338429ULL;
	zHashKeyTable[0][12] = 15278974144939869952ULL;
	zHashKeyTable[0][13] = 1178904170415774107ULL;
	zHashKeyTable[0][14] = 14312159022178454285ULL;
	zHashKeyTable[0][15] = 999233085284360399ULL;
	zHashKeyTable[0][16] = 3673610032419565815ULL;
	zHashKeyTable[0][17] = 10200191378812961596ULL;
	zHashKeyTable[0][18] = 17171052572236990857ULL;
	zHashKeyTable[0][19] = 12910624415628975563ULL;
	zHashKeyTable[0][20] = 1133753269132240824ULL;
	zHashKeyTable[0][21] = 16861286648502985131ULL;
	zHashKeyTable[0][22] = 7683044039126900139ULL;
	zHashKeyTable[0][23] = 1038922509328365637ULL;
	zHashKeyTable[0][24] = 1936665759235952618ULL;
	zHashKeyTable[0][25] = 11523903229869633698ULL;
	zHashKeyTable[1][0] = 4443749714730530452ULL;
	zHashKeyTable[1][1] = 4931317069979750352ULL;
	zHashKeyTable[1][2] = 2878540801725610348ULL;
	zHashKeyTable[1][3] = 6814224272552736412ULL;
	zHashKeyTable[1][4] = 8602295802373299017ULL;
	zHashKeyTable[1][5] = 817450930937265345ULL;
	zHashKeyTable[1][6] = 9729088585069824092ULL;
	zHashKeyTable[1][7] = 1826869469444344908ULL;
	zHashKeyTable[1][8] = 1832792314769568220ULL;
	zHashKeyTable[1][9] = 970911533329418781ULL;
	zHashKeyTable[1][10] = 12993319895482997274ULL;
	zHashKeyTable[1][11] = 1551611976840078566ULL;
	zHashKeyTable[1][12] = 1013083528060652185ULL;
	zHashKeyTable[1][13] = 11881704730350700540ULL;
	zHashKeyTable[1][14] = 2497447311264289494ULL;
	zHashKeyTable[1][15] = 5959612926913526374ULL;
	zHashKeyTable[1][16] = 6601168014724182827ULL;
	zHashKeyTable[1][17] = 18049490391873106099ULL;
	zHashKeyTable[1][18] = 16958398057790791008ULL;
	zHashKeyTable[1][19] = 1861772138104693913ULL;
	zHashKeyTable[1][20] = 16725831962028968827ULL;
	zHashKeyTable[1][21] = 11789023541005436750ULL;
	zHashKeyTable[1][22] = 2721076887055001735ULL;
	zHashKeyTable[1][23] = 963046728587824132ULL;
	zHashKeyTable[1][24] = 16199949645786162680ULL;
	zHashKeyTable[1][25] = 2585183267861616188ULL;
	zHashKeyTable[2][0] = 13442899657103353846ULL;
	zHashKeyTable[2][1] = 12670323325790699708ULL;
	zHashKeyTable[2][2] = 17966831504026015193ULL;
	zHashKeyTable[2][3] = 5511002498937852408ULL;
	zHashKeyTable[2][4] = 11553356999201370163ULL;
	zHashKeyTable[2][5] = 654272881313039396ULL;
	zHashKeyTable[2][6] = 4066899801665341059ULL;
	zHashKeyTable[2][7] = 17633713127922013165ULL;
	zHashKeyTable[2][8] = 295219453981572467ULL;
	zHashKeyTable[2][9] = 5306565072318765506ULL;
	zHashKeyTable[2][10] = 194862032116562003ULL;
	zHashKeyTable[2][11] = 13120655311599088812ULL;
	zHashKeyTable[2][12] = 10027809420366029844ULL;
	zHashKeyTable[2][13] = 13164134982359992265ULL;
	zHashKeyTable[2][14] = 437395564559695689ULL;
	zHashKeyTable[2][15] = 608492045632161667ULL;
	zHashKeyTable[2][16] = 1065720999459082908ULL;
	zHashKeyTable[2][17] = 18169099198871230912ULL;
	zHashKeyTable[2][18] = 18228051325149066345ULL;
	zHashKeyTable[2][19] = 16420075374420790763ULL;
	zHashKeyTable[2][20] = 2627401864689641493ULL;
	zHashKeyTable[2][21] = 16686033356496670341ULL;
	zHashKeyTable[2][22] = 18195765945304522206ULL;
	zHashKeyTable[2][23] = 1990826001870075319ULL;
	zHashKeyTable[2][24] = 15135534246272736394ULL;
	zHashKeyTable[2][25] = 7678572270892754578ULL;
	zHashKeyTable[3][0] = 4915743881155120569ULL;
	zHashKeyTable[3][1] = 15382883110411687878ULL;
	zHashKeyTable[3][2] = 3208677860630303494ULL;
	zHashKeyTable[3][3] = 11405585426356392905ULL;
	zHashKeyTable[3][4] = 3619901911520068310ULL;
	zHashKeyTable[3][5] = 15269347395525958321ULL;
	zHashKeyTable[3][6] = 463795585935020365ULL;
	zHashKeyTable[3][7] = 323309407431096893ULL;
	zHashKeyTable[3][8] = 6082498896797955114ULL;
	zHashKeyTable[3][9] = 12578189921767449685ULL;
	zHashKeyTable[3][10] = 144618453037943293ULL;
	zHashKeyTable[3][11] = 2904226964170863550ULL;
	zHashKeyTable[3][12] = 8814165841413682661ULL;
	zHashKeyTable[3][13] = 18067942192200254332ULL;
	zHashKeyTable[3][14] = 6088178713650187452ULL;
	zHashKeyTable[3][15] = 8470443469612998403ULL;
	zHashKeyTable[3][16] = 6987495215267895321ULL;
	zHashKeyTable[3][17] = 2450359492416774122ULL;
	zHashKeyTable[3][18] = 14111932932835372533ULL;
	zHashKeyTable[3][19] = 15134639079445810532ULL;
	zHashKeyTable[3][20] = 16980697896597851825ULL;
	zHashKeyTable[3][21] = 2046088857491236261ULL;
	zHashKeyTable[3][22] = 8853652317125007037ULL;
	zHashKeyTable[3][23] = 1684638287021652454ULL;
	zHashKeyTable[3][24] = 11956247818384589303ULL;
	zHashKeyTable[3][25] = 7340815900818344323ULL;
	zHashKeyTable[4][0] = 9557290848543934204ULL;
	zHashKeyTable[4][1] = 13828575586659117249ULL;
	zHashKeyTable[4][2] = 10928748662034900827ULL;
	zHashKeyTable[4][3] = 16184465235541545515ULL;
	zHashKeyTable[4][4] = 6266687536136366919ULL;
	zHashKeyTable[4][5] = 5426530894847719965ULL;
	zHashKeyTable[4][6] = 13152571260121756810ULL;
	zHashKeyTable[4][7] = 12246161496811710261ULL;
	zHashKeyTable[4][8] = 6517649567867222143ULL;
	zHashKeyTable[4][9] = 580863084621801998ULL;
	zHashKeyTable[4][10] = 1312891544210097081ULL;
	zHashKeyTable[4][11] = 10966547441209111942ULL;
	zHashKeyTable[4][12] = 13381938837246665789ULL;
	zHashKeyTable[4][13] = 8340957950309812544ULL;
	zHashKeyTable[4][14] = 3581451898302779457ULL;
	zHashKeyTable[4][15] = 9368000394858849431ULL;
	zHashKeyTable[4][16] = 7308611851001495041ULL;
	zHashKeyTable[4][17] = 8851113127088745940ULL;
	zHashKeyTable[4][18] = 14276240113269008173ULL;
	zHashKeyTable[4][19] = 490372281913590383ULL;
	zHashKeyTable[4][20] = 9648211762357338737ULL;
	zHashKeyTable[4][21] = 4209174509902788828ULL;
	zHashKeyTable[4][22] = 12539896776181909577ULL;
	zHashKeyTable[4][23] = 321131893796685793ULL;
	zHashKeyTable[4][24] = 17930851537743227476ULL;
	zHashKeyTable[4][25] = 11047484489807182366ULL;
	zHashKeyTable[5][0] = 9769561457821568427ULL;
	zHashKeyTable[5][1] = 1878109877624363053ULL;
	zHashKeyTable[5][2] = 8180335534929972581ULL;
	zHashKeyTable[5][3] = 1685787582733398475ULL;
	zHashKeyTable[5][4] = 8823408379554291918ULL;
	zHashKeyTable[5][5] = 2452913461454803949ULL;
	zHashKeyTable[5][6] = 3285386951354218491ULL;
	zHashKeyTable[5][7] = 15958363242992130620ULL;
	zHashKeyTable[5][8] = 5070203837403381359ULL;
	zHashKeyTable[5][9] = 8592996729534694203ULL;
	zHashKeyTable[5][10] = 2144532492139557833ULL;
	zHashKeyTable[5][11] = 8595151320355916694ULL;
	zHashKeyTable[5][12] = 4111633073542417775ULL;
	zHashKeyTable[5][13] = 16073167516644684090ULL;
	zHashKeyTable[5][14] = 14921232872118691769ULL;
	zHashKeyTable[5][15] = 18248584862374279404ULL;
	zHashKeyTable[5][16] = 5854918362689955315ULL;
	zHashKeyTable[5][17] = 5605473696763084716ULL;
	zHashKeyTable[5][18] = 4634058286818340089ULL;
	zHashKeyTable[5][19] = 15824751583198232594ULL;
	zHashKeyTable[5][20] = 7927379532644322117ULL;
	zHashKeyTable[5][21] = 3287564947286754174ULL;
	zHashKeyTable[5][22] = 11633191972662077398ULL;
	zHashKeyTable[5][23] = 2675743407400600703ULL;
	zHashKeyTable[5][24] = 17342098741278977254ULL;
	zHashKeyTable[5][25] = 202627089158275ULL;
	zHashKeyTable[6][0] = 13508947998995935195ULL;
	zHashKeyTable[6][1] = 13154969062426010308ULL;
	zHashKeyTable[6][2] = 4675279879065952604ULL;
	zHashKeyTable[6][3] = 14296610569370528522ULL;
	zHashKeyTable[6][4] = 6700660643361765415ULL;
	zHashKeyTable[6][5] = 2263586256334647694ULL;
	zHashKeyTable[6][6] = 11395805082994282493ULL;
	zHashKeyTable[6][7] = 14245903139927190272ULL;
	zHashKeyTable[6][8] = 4999094848529893121ULL;
	zHashKeyTable[6][9] = 12837979104350841162ULL;
	zHashKeyTable[6][10] = 15025661522744951970ULL;
	zHashKeyTable[6][11] = 4981677662198792478ULL;
	zHashKeyTable[6][12] = 11253247627290602958ULL;
	zHashKeyTable[6][13] = 16064711114660469251ULL;
	zHashKeyTable[6][14] = 16895494418771937184ULL;
	zHashKeyTable[6][15] = 15786488244016388961ULL;
	zHashKeyTable[6][16] = 5685786843405589761ULL;
	zHashKeyTable[6][17] = 10443628438009104823ULL;
	zHashKeyTable[6][18] = 3335168574974731943ULL;
	zHashKeyTable[6][19] = 16535407458984689505ULL;
	zHashKeyTable[6][20] = 11271530839338165296ULL;
	zHashKeyTable[6][21] = 12268383679988814307ULL;
	zHashKeyTable[6][22] = 12970492223527198255ULL;
	zHashKeyTable[6][23] = 10499682768902668294ULL;
	zHashKeyTable[6][24] = 10483433845573723716ULL;
	zHashKeyTable[6][25] = 16396574714988451087ULL;
	zHashKeyTable[7][0] = 13182362026222521242ULL;
	zHashKeyTable[7][1] = 10032961072849472479ULL;
	zHashKeyTable[7][2] = 9328652478331979952ULL;
	zHashKeyTable[7][3] = 6904564497726339991ULL;
	zHashKeyTable[7][4] = 1613365524275766750ULL;
	zHashKeyTable[7][5] = 7704640374587669602ULL;
	zHashKeyTable[7][6] = 5254203494666399497ULL;
	zHashKeyTable[7][7] = 12032763313590280360ULL;
	zHashKeyTable[7][8] = 17992726605871965135ULL;
	zHashKeyTable[7][9] = 15904062185373752637ULL;
	zHashKeyTable[7][10] = 7991895495104294893ULL;
	zHashKeyTable[7][11] = 532502632217709643ULL;
	zHashKeyTable[7][12] = 3557593412280107643ULL;
	zHashKeyTable[7][13] = 8270241588379152983ULL;
	zHashKeyTable[7][14] = 4515372992892596030ULL;
	zHashKeyTable[7][15] = 9223387754394478480ULL;
	zHashKeyTable[7][16] = 1286821572756027341ULL;
	zHashKeyTable[7][17] = 17869158897008401026ULL;
	zHashKeyTable[7][18] = 1320080521141852072ULL;
	zHashKeyTable[7][19] = 7884100809049428143ULL;
	zHashKeyTable[7][20] = 521446125401890978ULL;
	zHashKeyTable[7][21] = 8299456104643022943ULL;
	zHashKeyTable[7][22] = 9246745133015735911ULL;
	zHashKeyTable[7][23] = 4121222301738119830ULL;
	zHashKeyTable[7][24] = 6570152222751434703ULL;
	zHashKeyTable[7][25] = 4109886099917632561ULL;
	zHashKeyTable[8][0] = 10870644326992282004ULL;
	zHashKeyTable[8][1] = 1711220661820073572ULL;
	zHashKeyTable[8][2] = 17152761295905024888ULL;
	zHashKeyTable[8][3] = 12732480991574778562ULL;
	zHashKeyTable[8][4] = 1224578080386395683ULL;
	zHashKeyTable[8][5] = 15821794734242708184ULL;
	zHashKeyTable[8][6] = 16651366523420711966ULL;
	zHashKeyTable[8][7] = 1479432057303499100ULL;
	zHashKeyTable[8][8] = 17980379651286941408ULL;
	zHashKeyTable[8][9] = 2522732881074773570ULL;
	zHashKeyTable[8][10] = 13480939973557008780ULL;
	zHashKeyTable[8][11] = 6843581593579176755ULL;
	zHashKeyTable[8][12] = 6404193038964845569ULL;
	zHashKeyTable[8][13] = 12490053480717040445ULL;
	zHashKeyTable[8][14] = 11506696836100190992ULL;
	zHashKeyTable[8][15] = 7191489355786214932ULL;
	zHashKeyTable[8][16] = 3237743410922730555ULL;
	zHashKeyTable[8][17] = 13709708166701044880ULL;
	zHashKeyTable[8][18] = 8336143641159887817ULL;
	zHashKeyTable[8][19] = 1844638314023533118ULL;
	zHashKeyTable[8][20] = 5030622918803098121ULL;
	zHashKeyTable[8][21] = 4596012735048625614ULL;
	zHashKeyTable[8][22] = 3064393198265160488ULL;
	zHashKeyTable[8][23] = 5686302705909871166ULL;
	zHashKeyTable[8][24] = 5273025062446376010ULL;
	zHashKeyTable[8][25] = 5399063242685992491ULL;
	zHashKeyTable[9][0] = 5638311152600174187ULL;
	zHashKeyTable[9][1] = 7385617043925203729ULL;
	zHashKeyTable[9][2] = 10858814461594053459ULL;
	zHashKeyTable[9][3] = 14589212647143699987ULL;
	zHashKeyTable[9][4] = 8233862686256390624ULL;
	zHashKeyTable[9][5] = 15472868268300238518ULL;
	zHashKeyTable[9][6] = 17829916968657935831ULL;
	zHashKeyTable[9][7] = 10885558505415069530ULL;
	zHashKeyTable[9][8] = 15328201003703044614ULL;
	zHashKeyTable[9][9] = 360139693485680009ULL;
	zHashKeyTable[9][10] = 10131261970687357965ULL;
	zHashKeyTable[9][11] = 12401861429396637775ULL;
	zHashKeyTable[9][12] = 10958162750119692103ULL;
	zHashKeyTable[9][13] = 2708819993706501659ULL;
	zHashKeyTable[9][14] = 6349135367714372050ULL;
	zHashKeyTable[9][15] = 7005496928653674374ULL;
	zHashKeyTable[9][16] = 408700268364345494ULL;
	zHashKeyTable[9][17] = 5077576672506162206ULL;
	zHashKeyTable[9][18] = 14281328195458555598ULL;
	zHashKeyTable[9][19] = 17614776488785973778ULL;
	zHashKeyTable[9][20] = 14416841214793128816ULL;
	zHashKeyTable[9][21] = 2466571110623060883ULL;
	zHashKeyTable[9][22] = 3451062369842172441ULL;
	zHashKeyTable[9][23] = 14174139286819753846ULL;
	zHashKeyTable[9][24] = 4450513341500658814ULL;
	zHashKeyTable[9][25] = 285728340274758352ULL;
	zHashKeyTable[10][0] = 10747174989735955340ULL;
	zHashKeyTable[10][1] = 987682892211036935ULL;
	zHashKeyTable[10][2] = 9408748426967521851ULL;
	zHashKeyTable[10][3] = 12142967263543804484ULL;
	zHashKeyTable[10][4] = 3487091897510377924ULL;
	zHashKeyTable[10][5] = 9365701871589527998ULL;
	zHashKeyTable[10][6] = 17719510923323962860ULL;
	zHashKeyTable[10][7] = 13684805804189267786ULL;
	zHashKeyTable[10][8] = 4094997952697375273ULL;
	zHashKeyTable[10][9] = 16303171118951950146ULL;
	zHashKeyTable[10][10] = 1211971996311999564ULL;
	zHashKeyTable[10][11] = 13191394118655126410ULL;
	zHashKeyTable[10][12] = 995900035437721588ULL;
	zHashKeyTable[10][13] = 10853805865166160454ULL;
	zHashKeyTable[10][14] = 830967248388575377ULL;
	zHashKeyTable[10][15] = 4226178061931002011ULL;
	zHashKeyTable[10][16] = 8849380136075219555ULL;
	zHashKeyTable[10][17] = 13981695347778254365ULL;
	zHashKeyTable[10][18] = 8162625784110670648ULL;
	zHashKeyTable[10][19] = 944011483278227013ULL;
	zHashKeyTable[10][20] = 15196227996662312539ULL;
	zHashKeyTable[10][21] = 5439272479112023182ULL;
	zHashKeyTable[10][22] = 5450707187613430901ULL;
	zHashKeyTable[10][23] = 10306847319664130615ULL;
	zHashKeyTable[10][24] = 923524439044208780ULL;
	zHashKeyTable[10][25] = 18361397752759412508ULL;
	zHashKeyTable[11][0] = 16030522045324185611ULL;
	zHashKeyTable[11][1] = 13044001502870626970ULL;
	zHashKeyTable[11][2] = 10304705347122336651ULL;
	zHashKeyTable[11][3] = 11366665748778469063ULL;
	zHashKeyTable[11][4] = 270607423185149007ULL;
	zHashKeyTable[11][5] = 6283695468543643716ULL;
	zHashKeyTable[11][6] = 2593406968247605502ULL;
	zHashKeyTable[11][7] = 2894380828442048113ULL;
	zHashKeyTable[11][8] = 10559171723374841363ULL;
	zHashKeyTable[11][9] = 2843496428437549758ULL;
	zHashKeyTable[11][10] = 9676842070921619162ULL;
	zHashKeyTable[11][11] = 12680824187606258784ULL;
	zHashKeyTable[11][12] = 10675984168617482400ULL;
	zHashKeyTable[11][13] = 6652094990952263183ULL;
	zHashKeyTable[11][14] = 14643613270400594524ULL;
	zHashKeyTable[11][15] = 18193376564351555978ULL;
	zHashKeyTable[11][16] = 13613681308114392031ULL;
	zHashKeyTable[11][17] = 15019459475987182009ULL;
	zHashKeyTable[11][18] = 9063694019978219093ULL;
	zHashKeyTable[11][19] = 8472383287289689189ULL;
	zHashKeyTable[11][20] = 1311126057867428888ULL;
	zHashKeyTable[11][21] = 16810822765404951ULL;
	zHashKeyTable[11][22] = 10924686673993639495ULL;
	zHashKeyTable[11][23] = 23203051017681695ULL;
	zHashKeyTable[11][24] = 6221757625398127912ULL;
	zHashKeyTable[11][25] = 987455274408642379ULL;
	zHashKeyTable[12][0] = 5331283757361879929ULL;
	zHashKeyTable[12][1] = 10674776361076413657ULL;
	zHashKeyTable[12][2] = 12714393299481653788ULL;
	zHashKeyTable[12][3] = 16346556483683607778ULL;
	zHashKeyTable[12][4] = 6225857208318419701ULL;
	zHashKeyTable[12][5] = 3369707588546496327ULL;
	zHashKeyTable[12][6] = 9470970152979268086ULL;
	zHashKeyTable[12][7] = 13606139778906282085ULL;
	zHashKeyTable[12][8] = 12335969611361244351ULL;
	zHashKeyTable[12][9] = 16061657441760267693ULL;
	zHashKeyTable[12][10] = 4373274543781128153ULL;
	zHashKeyTable[12][11] = 9347857330272536820ULL;
	zHashKeyTable[12][12] = 13444086705852441758ULL;
	zHashKeyTable[12][13] = 14759959153837885923ULL;
	zHashKeyTable[12][14] = 15294360714516542464ULL;
	zHashKeyTable[12][15] = 13111196743008244047ULL;
	zHashKeyTable[12][16] = 16483571698429709984ULL;
	zHashKeyTable[12][17] = 11926923570193092792ULL;
	zHashKeyTable[12][18] = 14391820130289261433ULL;
	zHashKeyTable[12][19] = 6862630208141547385ULL;
	zHashKeyTable[12][20] = 6359267128607907145ULL;
	zHashKeyTable[12][21] = 3831464488789114341ULL;
	zHashKeyTable[12][22] = 7236403821834207107ULL;
	zHashKeyTable[12][23] = 18313502017041794723ULL;
	zHashKeyTable[12][24] = 16093261437324577464ULL;
	zHashKeyTable[12][25] = 3463888029380893773ULL;
}

void insertHashEntry(const LineNumbers &problem, const LineMask &nowString, const LineMask &settleString) noexcept {
	LineMask hashKey = problem.hashKey ^ nowString;
	hashKey %= HTABLE_SIZE;

	std::memcpy(&hashTable[hashKey].lineProblem, &problem, sizeof(problem));
	std::memcpy(&hashTable[hashKey].nowString, &nowString, sizeof(LineMask));
	std::memcpy(&hashTable[hashKey].settleString, &settleString, sizeof(LineMask));
}

[[nodiscard]] bool findHashEntry(const LineNumbers &problem, const LineMask &nowString, LineMask &settleString) noexcept {
	LineMask hashKey = problem.hashKey ^ nowString;
	hashKey %= HTABLE_SIZE;

	const bool nowStringMatches = (std::memcmp(&hashTable[hashKey].nowString, &nowString, sizeof(LineMask)) == 0);
	if (!nowStringMatches) return false;
	
	const bool problemMatches = (std::memcmp(&hashTable[hashKey].lineProblem, &problem, sizeof(problem)) == 0);
	if (!problemMatches) return false;

	std::memcpy(&settleString, &hashTable[hashKey].settleString, sizeof(LineMask));
	return true;
}

// =============================================================================
// PUZZLE IMPLEMENTATION
// =============================================================================

Puzzle::Puzzle() {
}

Puzzle::~Puzzle() {
}

Puzzle::Puzzle(const char *sInput) {
	parseLineNumbers(*this, sInput);
}

void parseLineNumbers(Puzzle &quest, const char *sInput) {
	char *tokenPtr;
	int lineCount = 0;
	char buffer[1300];
	strcpy(buffer, sInput);
	
	tokenPtr = strtok(buffer, "z");
	while (tokenPtr != nullptr) {
		const size_t tokenLength = strlen(tokenPtr);
		quest.mLines[lineCount].mCount = static_cast<unsigned char>(tokenLength);
		quest.mLines[lineCount].mNumbers[0] = 0;
		quest.mLines[lineCount].mSum[0] = 0;
		quest.mLines[lineCount].hashKey = 0;
		
		int tableIdx = 0;
		int cumulativeSum = 0;
		
		for (unsigned charIdx = 1; charIdx <= tokenLength; ++charIdx) {
			const unsigned charValue = static_cast<unsigned>(tokenPtr[charIdx - 1]) - static_cast<unsigned>('a') + 1;
			quest.mLines[lineCount].mNumbers[charIdx] = charValue;
			cumulativeSum += charValue;
			quest.mLines[lineCount].mSum[charIdx] = cumulativeSum;
			
			if (charValue > 0) {
				quest.mLines[lineCount].hashKey ^= zHashKeyTable[tableIdx][charValue];
				++tableIdx;
			}
		}
		
		tokenPtr = strtok(nullptr, "z");
		++lineCount;
	}
}

void printBoardSolution(const Board &sol) {
	for (unsigned rowIdx = 0; rowIdx < P_SIZE; ++rowIdx) {
		for (unsigned colIdx = 0; colIdx < P_SIZE; ++colIdx) {
			const int squareState = getBoardSquare(colIdx, rowIdx, sol);
			switch (squareState) {
				case SQUARE_BLOCK:
					printf("1");
					break;
				case SQUARE_SPACE:
					printf("0");
					break;
				case SQUARE_UNKNOWN:
					printf("-1");
					break;
				default:
					printf("-2");
					break;
			}
			
			const bool isLastColumn = (colIdx == P_SIZE - 1);
			if (!isLastColumn) {
				printf("\t");
			} else {
				printf("\n");
			}
		}
	}
}

// =============================================================================
// PARSERS IMPLEMENTATION
// =============================================================================

void parseInputFile(char *path, Puzzle *quests) {
	FILE *filePtr = fopen(path, "r");
	char lineBuffer[P_SIZE + 30];
	char puzzleBuffer[P_SIZE * P_SIZE + 30];
	char *bufferPtr = puzzleBuffer;
	
	for (int puzzleIdx = 0; puzzleIdx < NUM_OF_QUESTIONS; ++puzzleIdx) {
		fgets(lineBuffer, P_SIZE + 3, filePtr);
		
		for (int lineIdx = 0; lineIdx < P_SIZE * 2; ++lineIdx) {
			fgets(lineBuffer, P_SIZE + 3, filePtr);
			const size_t lineLength = strlen(lineBuffer);
			
			if (lineLength == 0) {
				*bufferPtr = 'a' - 1;
				++bufferPtr;
			} else {
				char *tokenPtr = strtok(lineBuffer, " \t");
				while (tokenPtr != nullptr) {
					const int tokenValue = atoi(tokenPtr);
					*bufferPtr = static_cast<char>(tokenValue + 'a' - 1);
					++bufferPtr;
					tokenPtr = strtok(nullptr, " \t");
				}
			}
			
			*bufferPtr = 'z';
			++bufferPtr;
		}
		
		*bufferPtr = 0;
		parseLineNumbers(quests[puzzleIdx], puzzleBuffer);
		bufferPtr = puzzleBuffer;
	}
	
	fclose(filePtr);
}

// =============================================================================
// LINE SOLVER IMPLEMENTATION
// =============================================================================

static LineMask blockMask[] = {
    0,
    0x1ull,
    0x5ull,
    0x15ull,
    0x55ull,
    0x155ull,
    0x555ull,
    0x1555ull,
    0x5555ull,
    0x15555ull,
    0x55555ull,
    0x155555ull,
    0x555555ull,
    0x1555555ull,
    0x5555555ull,
    0x15555555ull,
    0x55555555ull,
    0x155555555ull,
    0x555555555ull,
    0x1555555555ull,
    0x5555555555ull,
    0x15555555555ull,
    0x55555555555ull,
    0x155555555555ull,
    0x555555555555ull,
    0x1555555555555ull,
};

static LineMask newBlockMask[] = {
    0x2ull,
    0x6ull,
    0x16ull,
    0x56ull,
    0x156ull,
    0x556ull,
    0x1556ull,
    0x5556ull,
    0x15556ull,
    0x55556ull,
    0x155556ull,
    0x555556ull,
    0x1555556ull,
    0x5555556ull,
    0x15555556ull,
    0x55555556ull,
    0x155555556ull,
    0x555555556ull,
    0x1555555556ull,
    0x5555555556ull,
    0x15555555556ull,
    0x55555555556ull,
    0x155555555556ull,
    0x555555555556ull,
    0x1555555555556ull,
    0x5555555555556ull,
    0x15555555555556ull,
    0x55555555555556ull,
};

[[nodiscard]] bool solveLineConstraint(
    const LineNumbers &des,
    LineMask inString,
    int i,
    int j,
    LineMask &settleString,
    unsigned char *Dtable) noexcept {
    
    // Base case: completed successfully
    const bool isBaseCase = ((i == -1 || i == 0) && j == 0);
    if (isBaseCase) return true;
    
    // Out of bounds
    if (i < 0) return false;
    
    // Memoization check
    if (DPtable[i][j] == CONFLICT) return false;
    if (DPtable[i][j] == SOLVED) return true;

    const int currentSquare = SQUARE & SHIFT_R(inString, i-1);

    const int blockPosition = i - des.mNumbers[j];
    if (blockPosition < 0) {
        DPtable[i][j] = CONFLICT;
        return false;
    }
    
    const LineMask blockMaskForPosition = SHIFT_L(blockMask[des.mNumbers[j]], blockPosition) | 
                                          SHIFT_L(SQUARE_SPACE, blockPosition - 1);

    bool validSolution = false;
    
    // Check if we can place a block here
    const bool hasConflict = (blockMaskForPosition & (~inString)) != 0;
    const bool shouldTryBlock = (currentSquare != SQUARE_SPACE && !hasConflict && j > 0);
    
    if (shouldTryBlock) {
        if (solveLineConstraint(des, inString, blockPosition - 1, j - 1, settleString, Dtable)) {
            settleString |= blockMaskForPosition;
            validSolution = true;
        }
    }
    
    // Check if we can place a space here
    const bool shouldTrySpace = (currentSquare != SQUARE_BLOCK && (i > des.mSum[j]));
    if (shouldTrySpace) {
        if (solveLineConstraint(des, inString, i - 1, j, settleString, Dtable)) {
            settleString |= SHIFT_L(SQUARE_SPACE, (i-1));
            validSolution = true;
        }
    }
    
    DPtable[i][j] = validSolution ? SOLVED : CONFLICT;
    return validSolution;
}

[[nodiscard]] int lineSolver(const Puzzle &prob, Board &sol) noexcept {
    int currentState = UNSOLVED;
    
    while (!NodeQueue::myQ.isEmpty()) {
        const unsigned char lineIdx = NodeQueue::myQ.popAndFront();

        dbTable.thisTempTable |= (1ULL << lineIdx);

        LineMask settledMask = 0;
        const bool foundInHash = findHashEntry(prob.mLines[lineIdx], *(sol.colString + lineIdx), settledMask);
        
        if (!foundInHash) {
            std::memset(DPtable, UNSOLVED, sizeof(DPtable));
            const bool settlementSuccess = solveLineConstraint(prob.mLines[lineIdx], *(sol.colString + lineIdx), P_SIZE, prob.mLines[lineIdx].mCount, settledMask);
            
            if (!settlementSuccess) {
                NodeQueue::myQ.clear();
                return CONFLICT;
            }
            insertHashEntry(prob.mLines[lineIdx], *(sol.colString + lineIdx), settledMask);
        }
        
        LineMask changesMask = (sol.colString[lineIdx] ^ settledMask);
        
        // Optimized loop using bit manipulation
        for (int posIdx = 0; changesMask != 0 && posIdx < P_SIZE; ++posIdx) {
            const LineMask tempMask = (changesMask & 3);
            
            if (tempMask) {
                ++sol.numOfSquareOnBoard;
                const bool isColumnLine = (lineIdx < P_SIZE);
                
                if (isColumnLine) {
                    setBoardSquare(lineIdx, posIdx, sol, tempMask);
                    NodeQueue::myQ.pushQ(posIdx + P_SIZE);
                } else {
                    setBoardSquare(posIdx, lineIdx - P_SIZE, sol, tempMask);
                    NodeQueue::myQ.pushQ(posIdx);
                }
            }
            
            changesMask >>= 2;
        }
    }

    const bool isSolved = (sol.numOfSquareOnBoard == S_SIZE);
    return isSolved ? SOLVED : UNSOLVED;
}

// =============================================================================
// SEARCH SOLVER IMPLEMENTATION
// =============================================================================

int searchWithDepthFirstSearch(const Puzzle &problem,
                     Board &solution,
                     int &nodeCount
) noexcept {
    // Use a manual stack to avoid deep recursion
    // SearchNode is small (just a Board copy) and stored on heap via std::stack
    struct SearchNode {
        Board board;
        
        // Move constructor for efficiency - avoids deep Board copies
        SearchNode(Board &&b) : board(std::move(b)) {}
        SearchNode(const Board &b) : board(b) {}
    };
    
    std::stack<SearchNode> searchStack;
    searchStack.emplace(solution);
    
    while (!searchStack.empty()) {
        SearchNode current = std::move(searchStack.top());
        searchStack.pop();
        
        ++nodeCount;
        
        SquareToGo nextMove;
        const int twoSatResult = solveWithTwoSat(problem, current.board, nextMove);
        
        if (twoSatResult == SOLVED) {
            solution = current.board;
            return SOLVED;
        }
        
        if (twoSatResult != CONFLICT) {
            // Try second path first (will be popped last - depth-first order)
            searchStack.emplace(std::move(nextMove.secondGo));
            // Try first path
            searchStack.emplace(std::move(nextMove.firstGo));
        }
        // If CONFLICT, this path is abandoned (no push)
    }
    
    return CONFLICT;
}

int solvePuzzle(const Puzzle &problem,
                       Board &solution,
                       int &nodeCount
) noexcept {
    std::memset(dbTable.pointTables, 0, sizeof(dbTable.pointTables));
    
    const int lineSolverResult = lineSolver(problem, solution);
    
    if (lineSolverResult == SOLVED)
        return SOLVED;
    if (lineSolverResult == CONFLICT)
        return CONFLICT;
    
    return searchWithDepthFirstSearch(problem, solution, nodeCount);
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(int argc, char *argv[]) {
	initializeHashTable();
	
	const int totalQuestions = atoi(argv[1]);
	NUM_OF_QUESTIONS = totalQuestions;
	
	Puzzle puzzles[totalQuestions];
	parseInputFile(const_cast<char*>(INPUT_FILENAME), puzzles);
	
	Board boards[totalQuestions];
	freopen(OUTPUT_FILENAME, "w", stdout);
	FILE *logFile = fopen(LOG_FILENAME, "w");
	
	const clock_t startTime = clock();
	
	for (int questionIdx = 0; questionIdx < totalQuestions; ++questionIdx) {
		int nodeCounter = 0;
		const clock_t questionStartTime = clock();
		initializeBoard(boards[questionIdx]);

		const int solverState = solvePuzzle(puzzles[questionIdx], boards[questionIdx], nodeCounter);
		fprintf(stderr, "#%d solved!!!\n", questionIdx + 1);
		printf("$%d\n", questionIdx + 1);
		printBoardSolution(boards[questionIdx]);
		
		const double elapsedSeconds = static_cast<double>(clock() - questionStartTime) / CLOCKS_PER_SEC;
		fprintf(logFile, "#%d\t%f\n", questionIdx + 1, elapsedSeconds);
		fflush(stdout);
		fflush(logFile);
	}
	
	const clock_t endTime = clock();
	const double totalElapsed = static_cast<double>(endTime - startTime) / CLOCKS_PER_SEC;
	fprintf(logFile, "total time: %f\n", totalElapsed);
	fflush(logFile);
	fclose(logFile);

	return 0;
}
