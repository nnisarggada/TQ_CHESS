
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;


#define BIGNUMBER 100
#define C_INCR 1
#define C_DECR 10
#define IMG_H 450
#define IMG_W 450
#define THRESHOLD 50

#define PAWN_B 0
#define PAWN_W 1
#define KING_B 2
#define KING_W 3
#define QUEEN_B 4
#define QUEEN_W 5
#define BISH_B 6
#define BISH_W 7
#define KNIGHT_B 8
#define KNIGHT_W 9
#define ROOK_B 10
#define ROOK_W 11


struct position
{
    int row;
    int column;
} Position;

struct piece
{
    int nr;
    position pos;
} Piece;

void drawPoints(vector<Point2f> pointslist, Mat img);
void findAllChessboardCorners(Mat img, vector<Point2f>* pointlist);
bool detectMovement(Mat img, vector<Rect>* boundRectList);
void findMovement(vector<Rect> boundRectList, vector<Point2f> cornerlist);

void initPieceList(vector<piece>* pieceList);
string nrToString(int nr);
void toFile(piece p, bool capture);
void findLegalMoves(piece p);
bool findPieceOnPos(position p, piece* Piece);
position coordToPosition(int x, int y, vector<Point2f> cornerlist);
void positionToCoord(position pos, int*x, int*y);
void on_mouse(int e, int x, int y, int d, void *ptr);
