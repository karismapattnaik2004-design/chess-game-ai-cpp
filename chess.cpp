#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>
using namespace std;

// 1. inline, namespace & templates
namespace Util 
{
    inline bool inBounds(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }
    template<typename T>
    void myswap(T& a, T& b) { T t = a; a = b; b = t; }
}

// 2. Position class with operator overloading and call by reference
class Position 
{
public:
    int x, y;
    Position(int X = 0, int Y = 0) : x(X), y(Y) {}
    bool operator==(const Position& o) const { return x == o.x && y == o.y; }
    friend ostream& operator<<(ostream& os, const Position& p) {
        return os << char('a' + p.y) << (8 - p.x);
    }
};

// 3. Abstract Piece class (pure virtual + static members + player of 'this')
class Piece 
{
protected:
    char color;
    Position pos;
    static int totalPieces;
public:
    Piece(char c, Position p) : color(c), pos(p) { totalPieces++; }
    Piece(const Piece& o) : color(o.color), pos(o.pos) { totalPieces++; }
    virtual ~Piece() { totalPieces--; }

    virtual char symbol() const = 0;
    virtual bool canMoveTo(const Position& to) const = 0;
    virtual Piece* clone() const = 0;

    char getColor() const { return color; }
    Position getPos() const { return pos; }
    void setPos(const Position& p) { this->pos = p; }

    static int getTotalPieces() { return totalPieces; }
};
int Piece::totalPieces = 0;

// 4. Concrete piece classes: King, Rook, Queen, Bishop, Knight, Pawn

class King : public Piece 
{
public:
    King(char c, Position p) : Piece(c, p) {}
    char symbol() const override { return color == 'W' ? 'K' : 'k'; }
    bool canMoveTo(const Position& to) const override {
        int dx = abs(to.x - pos.x), dy = abs(to.y - pos.y);
        return (dx <= 1 && dy <= 1) && !(dx == 0 && dy == 0);
    }
    Piece* clone() const override { return new King(*this); }
};

class Rook : public Piece 
{
public:
    Rook(char c, Position p) : Piece(c, p) {}
    char symbol() const override { return color == 'W' ? 'R' : 'r'; }
    bool canMoveTo(const Position& to) const override {
        return (pos.x == to.x || pos.y == to.y) && !(pos == to);
    }
    Piece* clone() const override { return new Rook(*this); }
};

class Bishop : public Piece 
{
public:
    Bishop(char c, Position p) : Piece(c, p) {}
    char symbol() const override { return color == 'W' ? 'B' : 'b'; }
    bool canMoveTo(const Position& to) const override {
        return abs(to.x - pos.x) == abs(to.y - pos.y);
    }
    Piece* clone() const override { return new Bishop(*this); }
};

class Queen : public Piece 
{
public:
    Queen(char c, Position p) : Piece(c, p) {}
    char symbol() const override { return color == 'W' ? 'Q' : 'q'; }
    bool canMoveTo(const Position& to) const override {
        return (pos.x == to.x || pos.y == to.y) || abs(to.x - pos.x) == abs(to.y - pos.y);
    }
    Piece* clone() const override { return new Queen(*this); }
};

class Knight : public Piece 
{
public:
    Knight(char c, Position p) : Piece(c, p) {}
    char symbol() const override { return color == 'W' ? 'N' : 'n'; }
    bool canMoveTo(const Position& to) const override {
        int dx = abs(to.x - pos.x), dy = abs(to.y - pos.y);
        return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
    }
    Piece* clone() const override { return new Knight(*this); }
};

class Pawn : public Piece 
{
public:
    Pawn(char c, Position p) : Piece(c, p) {}
    char symbol() const override { return color == 'W' ? 'P' : 'p'; }
    bool canMoveTo(const Position& to) const override {
        int dir = (color == 'W' ? -1 : 1);
        int startRow = (color == 'W' ? 6 : 1);

        // Forward move
        if (to.y == pos.y)
        {
            // one step forward
            if (to.x == pos.x + dir) return true;
            // two steps forward if at start position
            if (pos.x == startRow && to.x == pos.x + 2 * dir) return true;
        }
        // Capture diagonally (simple)
        if ((to.y == pos.y + 1 || to.y == pos.y - 1) && to.x == pos.x + dir)
        {
            // We allow capture move but board.movePiece checks for piece color, so just allow move here
            return true;
        }
        return false;
    }
    Piece* clone() const override { return new Pawn(*this); 
    }
};


// 5. Board class (static count, file handling, friend Game & AIPlayer)
class Game;
class AIPlayer;
class Board 
{
    Piece* grid[8][8];
    static int instanceCount;
public:
    Board() { instanceCount++; memset(grid, 0, sizeof grid);
     }
    Board(const Board& o) { instanceCount++; copyFrom(o); 
    }
    ~Board() { clear(); instanceCount--;
     }
    Board& operator=(const Board& o) 
    {
        if (this != &o)
            {
             clear(); copyFrom(o);
            }
        return *this;
    }
    inline void clear()
    {
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++)
                delete grid[i][j], grid[i][j] = nullptr;
    }
    inline void copyFrom(const Board& o) 
    {
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++)
                grid[i][j] = o.grid[i][j] ? o.grid[i][j]->clone() : nullptr;
    }
    inline void display() const 
    {
        cout << "  a b c d e f g h\n";
        for (int i = 0; i < 8; i++)
        {
            cout << 8 - i << " ";
            for (int j = 0; j < 8; j++)
                cout << (grid[i][j] ? grid[i][j]->symbol() : '.') << " ";
            cout << 8 - i << "\n";
        }
        cout << "  a b c d e f g h\n";
    }
    bool movePiece(const Position& f, const Position& t, char playerColor)
    {
        if (!Util::inBounds(f.x, f.y) || !Util::inBounds(t.x, t.y)) return false;
        Piece* p = grid[f.x][f.y];
        if (!p || p->getColor() != playerColor) return false;
        if (!p->canMoveTo(t)) return false;

        // Prevent capturing own piece
        if (grid[t.x][t.y] && grid[t.x][t.y]->getColor() == playerColor) return false;

        delete grid[t.x][t.y];
        grid[t.x][t.y] = p; grid[f.x][f.y] = nullptr;
        p->setPos(t);
        return true;
    }
    bool save(const string& file = "save.txt") const
    {
        ofstream ofs(file);
        if (!ofs) return false;
        for (int i = 0; i < 8; i++) 
        {
            for (int j = 0; j < 8; j++)
                ofs << (grid[i][j] ? grid[i][j]->symbol() : '.');
            ofs << "\n";
        }
        return true;
    }
    bool load(const string& file = "save.txt")
    {
        ifstream ifs(file);
        if (!ifs) return false;
        clear();
        for (int i = 0; i < 8; i++) 
        {
            string s; getline(ifs, s);
            for (int j = 0; j < 8; j++)
            {
                char c = s[j];
                Position p(i, j);
                if (c == 'K') grid[i][j] = new King('W', p);
                else if (c == 'k') grid[i][j] = new King('B', p);
                else if (c == 'Q') grid[i][j] = new Queen('W', p);
                else if (c == 'q') grid[i][j] = new Queen('B', p);
                else if (c == 'R') grid[i][j] = new Rook('W', p);
                else if (c == 'r') grid[i][j] = new Rook('B', p);
                else if (c == 'B') grid[i][j] = new Bishop('W', p);
                else if (c == 'b') grid[i][j] = new Bishop('B', p);
                else if (c == 'N') grid[i][j] = new Knight('W', p);
                else if (c == 'n') grid[i][j] = new Knight('B', p);
                else if (c == 'P') grid[i][j] = new Pawn('W', p);
                else if (c == 'p') grid[i][j] = new Pawn('B', p);
            }
        }
        return true;
    }
    friend class Game;
    friend class AIPlayer;
};
int Board::instanceCount = 0;

// 6. Abstract Player + inheritance
class Player
{
protected:
    char color;
public:
    Player(char c) : color(c) {}
    virtual ~Player() {}
    virtual Position getFrom(Board&) = 0;
    virtual Position getTo(Board&, Position) = 0;
    char getColor() const { return color; }
};

class HumanPlayer : public Player 
{
public:
    HumanPlayer(char c) : Player(c) {}
    Position getFrom(Board&) override {
        while (true) {
            cout << "Enter piece position (e.g. e2): ";
            string s; cin >> s;
            if (s.length() == 2) {
                int x = 8 - (s[1] - '0');
                int y = s[0] - 'a';
                if (Util::inBounds(x, y)) return Position(x, y);
            }
            cout << "Invalid input.\n";
        }
    }
    Position getTo(Board&, Position) override 
    {
        while (true) {
            cout << "Enter target position (e.g. e4): ";
            string s; cin >> s;
            if (s.length() == 2) {
                int x = 8 - (s[1] - '0');
                int y = s[0] - 'a';
                if (Util::inBounds(x, y)) return Position(x, y);
            }
            cout << "Invalid input.\n";
        }
    }
};

class AIPlayer : public Player
{
public:
    AIPlayer(char c) : Player(c) { srand(time(0)); }
    Position getFrom(Board& board) override {
        // Random pick a piece of AI's color that can move somewhere
        vector<Position> candidates;
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++) {
                Piece* p = board.grid[i][j];
                if (p && p->getColor() == color) {
                    candidates.push_back(Position(i, j));
                }
            }
        if (candidates.empty()) return Position(-1, -1);
        return candidates[rand() % candidates.size()];
    }
    Position getTo(Board& board, Position from) override {
        Piece* p = board.grid[from.x][from.y];
        if (!p) return Position(-1, -1);
        // Try random positions until valid move found (simple AI)
        for (int attempt = 0; attempt < 64; attempt++) {
            int x = rand() % 8, y = rand() % 8;
            Position to(x, y);
            if (p->canMoveTo(to) && (!board.grid[x][y] || board.grid[x][y]->getColor() != color))
                return to;
        }
        return Position(-1, -1);
    }
};

// 7. Game class - manages board, players, play loop, file handling
class Game 
{
    Board board;
public:
    Game() {
        setup();
    }
    void setup() {
        // Setup initial positions - simple pawns and kings only for demo
        board.clear();
        for (int i = 0; i < 8; i++) {
            board.grid[1][i] = new Pawn('B', Position(1, i));
            board.grid[6][i] = new Pawn('W', Position(6, i));
        }
        board.grid[0][4] = new King('B', Position(0, 4));
        board.grid[7][4] = new King('W', Position(7, 4));
    }
    bool loadGame(const string& file) { return board.load(file); }
    void saveGame(const string& file = "save.txt") { board.save(file); }
    void play(Player* p1, Player* p2) {
        Player* turn = p1;
        while (true) {
            board.display();
            cout << (turn->getColor() == 'W' ? "White" : "Black") << "'s turn\n";
            Position from = turn->getFrom(board);
            if (from.x == -1) { cout << "No valid moves, game over.\n"; break; }
            Position to = turn->getTo(board, from);
            if (to.x == -1) { cout << "No valid moves, game over.\n"; break; }
            if (!board.movePiece(from, to, turn->getColor())) {
                cout << "Invalid move, try again.\n";
                continue;
            }
            saveGame();

            turn = (turn == p1 ? p2 : p1);
        }
    }
};

int main() 
{
    Game g;
    HumanPlayer hp('W');
    AIPlayer ai('B');

    cout << "Load saved game? (1 = yes, 0 = no): ";
    int c; cin >> c;
    if (c == 1) g.loadGame("save.txt");
    g.play(&hp, &ai);

    return 0;
}
