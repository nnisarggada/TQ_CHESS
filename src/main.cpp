/* Program for detecting chess movement detection and tracking using OpenCV 3.4
 * Author: SaltFactory (https://gitlab.com/Salt_Factory, https://github.com/Salt-Factory)
 * demonstration over at https://www.youtube.com/watch?v=w67BJXWnMkw
 */

#include "chessdetection.h"

const int thresh_slider_max = 200;
int thresh_slider = 50;
int movementthreshold = 50;

static void on_trackbar(int, void*)
{
    movementthreshold = thresh_slider;
}

int movcount;   //counter that counts how many frames there were with 2 contours
bool turn = false; //boolean to remember who's turn it is. False = white, true = black
bool drawPossibleMoves = false;
vector<piece> pieceList; //vector containing all the pieces and their location (with (-1;-1) being taken)
vector<position> possiblePositions;
vector<Point2f> cornerlist;
string outputfile;

int main(int argc, const char **argv)
{
    //commandlineparser to parse the url flag (if there is any)
    CommandLineParser parser(argc, argv,
    "{ help h usage ?      || show this message }"
    "{ video url u p       || path to the video  (leave empty for webcam) \n example: 'schaakbord --url=ExcitingChessMatch.mp4'}"
    "{ textfile t output o || path to the textfile where the notation of the game is written to; default is 'chess.txt'}"
    "{ cam camera || camera to use (see cap opencv docs}"
    );

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    //parse the videolocation and outputlocation
    string video_location(parser.get<string>("video"));
    string output_location(parser.get<string>("textfile"));

    //set the outputfile variable to the correct location based on the argument
    if (output_location.empty())
    {
        outputfile = "chess.txt";
    }
    else
    {
        outputfile = output_location;
    }

    
    //make a videocapture element
    VideoCapture cap;
    if (video_location.empty()) //if no argument is given, load the webcam!
    {
        cout << "Using the webcam!" << endl;
        cap.open(0);//the index of the webcam, as listed in "ls /dev/video*". Videodevice0 is videofeed, videodevice1 is the audiofeed.
                    //when using an external webcam on a laptop (with an internal webcam), you might need to use videodevice2.
    }
    else
    {
        cout << "Using a video!" << endl;
        cout << "Opening " << (video_location) << endl;
        cap.open(video_location);
    }

    initPieceList(&pieceList); //fill the piecelist with pieces!
    for (int i = 0; i < (pieceList).size(); i++)
    {
        //make a nice debugprint of all the pieces
        cout << (pieceList)[i].pos.row << pieceList[i].pos.column << pieceList[i].nr << endl;

    }

    //Start the videocapture
    if (cap.isOpened() == false)
    {
        cerr << "Cannot open file or videofeed!" << endl;
        return -1;
    }
    cout << "Video loaded!" <<endl;
    double fps = cap.get(CAP_PROP_FPS);
    cout << fps << " frames per second" << endl;

    string windowname = "Configuration";
    namedWindow(windowname); //make a named window

    Mat frame; 
    bool bSuccess = cap.read(frame); //read a frame
    vector<Point2f> tilecorners;

    //this while loop will allow the user to play with the settings until the chessboardcorners are correctly set up and the user is satisfied
    while (true)
    {
        bool bSuccess = cap.read(frame);
        resize(frame,frame,Size(IMG_H,IMG_W)); //resize so it fits on my screen

        if (bSuccess == false)
        {
            cout << "End of video!" << endl;
            waitKey(0);
            exit(1);
        }

        findAllChessboardCorners(frame, &tilecorners); //find the corners
        cornerlist = tilecorners;
        drawPoints(tilecorners, frame); //draw the cornerpoints
        imshow(windowname,frame); //show the image
        int key = waitKey(0);
        if (key == 27)
        {
            cout << "Esc" << endl;
            exit(1);
        }
        if (key == 13) //if enter is pressed, we exit our loop
        {
            cout << "Values saved" << endl;
            destroyAllWindows();
            break;
        }
    }

    windowname = "Chessmatch";
    namedWindow(windowname); //make a named window
    Point p;
    setMouseCallback(windowname, on_mouse, &p);

    createTrackbar("movement threshold", windowname, &thresh_slider, thresh_slider_max, on_trackbar);

    bool movement = false;
    //create a backgroundsubtractor
    auto bgdet = createBackgroundSubtractorMOG2();
    bgdet->setBackgroundRatio(0.5); //TODO: explain why this is needed

    //create an element for future erosion
    Mat element = getStructuringElement( MORPH_RECT, Size(5,5), Point(2,2));

    Mat bg; //mat to contain our background
    while(true)
    {
        bool bSuccess = cap.read(frame);
        resize(frame,frame,Size(IMG_H, IMG_W)); //resize the image so it fits

        if (bSuccess == false)
        {
            cout << "End of video!" << endl;
            waitKey(0);
            exit(1);
        }

        Mat fgmask; //create a foregroundmask
        bgdet->apply(frame, fgmask); //apply the foregroundmask on the image
        bgdet->getBackgroundImage(bg); //get the background
        erode(fgmask, fgmask, element); //erode the mask, to reduce the noise

        vector<Rect> boundRectList; //make an empty vector of bounding rectangles
        if (detectMovement(fgmask, &boundRectList)) //if we detect movement, we then need to find the movement (aka find out what moved to where)
        {
            findMovement(boundRectList, tilecorners);
        }

        drawPoints(tilecorners, frame); //draw the cornerpoints
        hconcat(frame, bg, frame); //concat the frame and the background
        //convert the masks type so it's the same as the frame's and the background's type
        fgmask.convertTo(fgmask, CV_8UC3);
        cvtColor(fgmask, fgmask, COLOR_GRAY2BGR);
        hconcat(frame, fgmask, frame); //concat the foregroundmask to the frame
        imshow(windowname,frame); //show the three together in one big happy window :)
        int key = waitKey(10);
        if (key == 27)
        {
            cout << "Esc" << endl;
            exit(1);
        }
        if (key == 13) //if enter is pressed, we exit our loop
        {
            cout << "End of capture" << endl;
            destroyAllWindows();
            break;
        }
    }
}

/*function to draw points on an image, together with their index in the vector
 *    input: the points to be drawn, in the form of a vector of Point2f and an image to draw them on
 *   output: void
 */
void drawPoints(vector<Point2f> pointlist, Mat img)
{
    for (int i = 0; i < pointlist.size(); i++)
    {
        Point pt = pointlist[i];
        circle(img, pt, 3, Scalar(0,255,0));
        putText(img, to_string(i), pt, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,255,0));
        
        if (!turn)
        {
            putText(img, "White", Point(20,200), FONT_HERSHEY_SIMPLEX, 1, Scalar(255));
        }
        else
        {
            putText(img, "Black", Point(20,200), FONT_HERSHEY_SIMPLEX, 1, Scalar(255));
        }
    }
    
    if (drawPossibleMoves)
    {
        for (int i = 0; i < possiblePositions.size(); i++)
        {
            Point centre;
            int x; int y;
            positionToCoord(possiblePositions[i], &x, &y);
            centre.x = x;
            centre.y = y;
            circle(img, centre, 10, Scalar(0,0,255));
        }

    }
}

/* Function that finds all the chessboardcorners and stores them in a vector
 * this function might seem a bit redundant, but this is for in the case of future improvement to the algorithm
 *  input: image containing a chessboard, a pointer to the destinationvector
 *  output: void, and in pointlist the inner points on the chessboard
 */
void findAllChessboardCorners(Mat img, vector<Point2f>* pointlist)
{
    //first we use a basic opencv function (thank god!)
    //this function however only returns the inner corners
    findChessboardCorners(img, Size(7,7), *pointlist);

}

/* Function that detects if there was a significant enough movement of a piece on the foregroundmask
 *  input: the foregroundmask and a pointer to a vector of Rectangles
 *  output: a boolean of succes, and a filled vector of bounding rectangles
 */
bool detectMovement(Mat img, vector<Rect>* boundRectList)
{

    //first threshold the image on grayvalue 200, so that it becomes binary (with "0"="0" and "1"="255")
    img = img > 200;

    //create an element for morphological operations
    Mat element = getStructuringElement(MORPH_RECT, Size(13,13), Point(6,6));
    dilate(img,img,element); //dilate for when a piece leaves behind 2 holes instead of 1 hole on a tile, to increase the chance of them becoming one contour

    //create empty vector of contours
    vector <vector<Point>> contours;
    //find the contours
    findContours(img.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

    //find the bounding rectangles
    for (int i = 0; i < contours.size(); i ++)
    {
        Rect boundRect = boundingRect(Mat(contours[i]));
        (*boundRectList).push_back(boundRect);
    }

    //draw the bounding rectangles
    for (int i = 0; i < (*boundRectList).size(); i++)
    {
        rectangle(img, (*boundRectList)[i], Scalar(255));
    }

    //if there's 2 contours, increase the movcount
    //(also increase if the movcount is currently negative)
    //else we decrease it
    if (contours.size() == 2 || movcount < 0)
    {
        movcount += C_INCR;
    }
    else if (movcount > 0)
    {
        movcount--;
    }
    putText(img, to_string(movcount), Point(20,100), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255));

    //once it passes a threshold, we can assume a turn happened!
    if (movcount > movementthreshold)
    {
        movcount = -250;
        turn = !turn;
        return true;
    }
    return false;
}

/* Function that finds the specific movement that happened on a turn
 *  input: bounding rectangles of the 2 contours on the foreground, and a vector of cornerpoints
 *  output: void
 */
void findMovement(vector<Rect> boundRectList, vector<Point2f> cornerlist)
{
    cout << "Detecting Movement" << endl;
    vector<position> poslist; //make empty vector which will contain the detected positions

    for (int i = 0; i < boundRectList.size(); i++) 
    {
        int x = boundRectList[i].x + boundRectList[i].width/2; //calculate the centre of the boundingrects
        int y = boundRectList[i].y + boundRectList[i].height/2;
        //cout << "x: " << x << " y: " << y << endl;
        Point pt(x,y);
        
        //this for runs through the entire list of cornerpoints, and checks what tile the piece is on
        //TODO: make this a seperate function
        //the forloop starts with the topmost cornerpoint
        //if the x&y of the centre of the contour are smaller, we can assume this is the tile the piece is/was on
        //else if the index modulo 7 equals 6 (the last cornerpoint in a row), and the y of the centre is smaller, and the x is larger, we can assume it is/was on the last tile on that row
        //best way to understand the below mess is by testing the program itself
        /*
        for (int j = 0; j < cornerlist.size(); j++)
        {
            if (x < cornerlist[j].x && y < cornerlist[j].y)
            {
                position p;
                p.column = j%7;
                p.row = j/7;
                cout << "Movement" << i << " " << p.column << " " << p.row << endl;
                poslist.push_back(p);
                break;
            }
            if (j%7 == 6 && y < cornerlist[j].y && x > cornerlist[j].x)
            {
                position p;
                p.column = 7;
                p.row = j/7;
                cout << "Movement" << i << " " << p.column << " " << p.row << endl;
                poslist.push_back(p);
                break;
            }
            if (j > 41 && y > cornerlist[j].y && x < cornerlist[j].x)
            {
                position p;
                p.column = j%7;
                p.row = 7;
                cout << "Movement" << i << " " << p.column << " " << p.row << endl;
                poslist.push_back(p);
                break;
            }
            if (j == 48 && y > cornerlist[j].y && x > cornerlist[j].x)
            {
                position p;
                p.column = 7;
                p.row = 7;
                cout << "Movement" << i << " " << p.column << " " << p.row << endl;
                poslist.push_back(p);
                break;
                
            }
        }
        */
        position p = coordToPosition(x,y,cornerlist);
        poslist.push_back(p);
        cout << "Movement" << i << " " << p.column << " " << p.row << endl;
    }
    //now we have the 2 areas where movement has been detected
    //we just need to find out what piece moved and whether it slayed another piece
    
    //create three empty vectors of ints
    vector<int> posint;
    vector<int> pieceint;
    vector<int> colourint;

    //TODO: do this part in a loop, cause why not?
    //start by checking if there was a piece on the first pos (when row=row and col=col)
    for (int i = 0; i < pieceList.size(); i++)
    {
        if (pieceList[i].pos.row == poslist[0].row && pieceList[i].pos.column == poslist[0].column)
        {
            cout << nrToString(pieceList[i].nr) << " was found" << endl;;
            pieceint.push_back(i);
            posint.push_back(0);
            colourint.push_back(pieceList[i].nr%2);
        }
    }
    //check if there was a piece on the second position
    for (int i = 0; i < pieceList.size(); i++)
    {
        if (pieceList[i].pos.row == poslist[1].row && pieceList[i].pos.column == poslist[1].column)
        {
            cout << nrToString(pieceList[i].nr) << " was found" << endl;;
            pieceint.push_back(i);
            posint.push_back(1);
            colourint.push_back(pieceList[i].nr%2);
        }
    }
    //if only one piece was found, it's simple: the piece moved from his position to the other
    if (posint.size() == 1)
    {//only one piece moved
       pieceList[pieceint[0]].pos = poslist[!posint[0]]; 
       cout << nrToString(pieceList[pieceint[0]].nr) << " moved to " << poslist[!posint[0]].row << " " << poslist[!posint[0]].column << endl;
       toFile(pieceList[pieceint[0]],false);
    }
    
    //if two pieces were found, a piece took another piece, but which piece took which?
    //to help us we can use the "turn" boolean: if it's white's turn, white took black (and the opposite!)
    if (posint.size() == 2)
    {//one piece moved, another one got slain
        if (turn)//it was white's turn, so the white piece moved and black got slain
        {
            if (colourint[0] == 1)
            {//the piece was a white piece
                pieceList[pieceint[0]].pos = poslist[!posint[0]];
                cout << nrToString(pieceList[pieceint[0]].nr) << " moved to " << poslist[!posint[0]].row << " " << poslist[!posint[0]].column << endl;
                cout << " and slayed " << nrToString(pieceList[pieceint[1]].nr) << endl;
                position p;
                p.column = -1;
                p.row = -1;
                pieceList[pieceint[1]].pos = p;
                toFile(pieceList[pieceint[0]],true);
            }
            else 
            {//the piece was a black piece
                pieceList[pieceint[1]].pos = poslist[!posint[1]];
                cout << nrToString(pieceList[pieceint[1]].nr) << " moved to " << poslist[!posint[1]].row << " " << poslist[!posint[0]].column << endl;
                cout << " and slayed " << nrToString(pieceList[pieceint[0]].nr) << endl;
                position p;
                p.column = -1;
                p.row = -1;
                pieceList[pieceint[0]].pos = p;
                toFile(pieceList[pieceint[1]],true);
            }
        }
        else
        {
            if (colourint[0] == 1)
            {//the piece was a white piece
                pieceList[pieceint[1]].pos = poslist[!posint[1]];
                cout << nrToString(pieceList[pieceint[1]].nr) << " moved to " << poslist[!posint[1]].row << " " << poslist[!posint[1]].column << endl;
                cout << " and slayed " << nrToString(pieceList[pieceint[0]].nr) << endl;
                position p;
                p.column = -1;
                p.row = -1;
                pieceList[pieceint[0]].pos = p;
                toFile(pieceList[pieceint[1]],true);
            }
            else 
            {//the piece was a black piece
                pieceList[pieceint[0]].pos = poslist[!posint[0]];
                cout << nrToString(pieceList[pieceint[0]].nr) << " moved to " << poslist[!posint[0]].row << " " << poslist[!posint[0]].column << endl;
                cout << " and slayed " << nrToString(pieceList[pieceint[1]].nr) << endl;
                position p;
                p.column = -1;
                p.row = -1;
                pieceList[pieceint[1]].pos = p;
                toFile(pieceList[pieceint[0]],true);
            }
        }
    }
    

}

//sadly not even the ugliest function I've ever written
//initialises a vector of pieces
void initPieceList(vector<piece> (*pieceList))
{
    //add in the white pawns
    for (int i = 0; i < 8; i++)
    {
        piece pawn;
        position pos;
        pos.column = i;
        pos.row = 1;

        pawn.nr = PAWN_W;
        pawn.pos = pos;
        (*pieceList).push_back(pawn);
    }

    //add in the black pawns
    for (int i = 0; i < 8; i++)
    {
        piece pawn;
        position pos;
        pos.column = i;
        pos.row = 6;

        pawn.nr = PAWN_B;
        pawn.pos = pos;
        (*pieceList).push_back(pawn);
    }
    
    {
        piece rook;
        position pos;
        pos.column = 0;
        pos.row = 0;

        rook.nr = ROOK_W;
        rook.pos = pos;
        (*pieceList).push_back(rook);

        pos.column = 7;
        rook.pos = pos;
        (*pieceList).push_back(rook);

        pos.row = 7;
        rook.pos = pos;
        rook.nr = ROOK_B;
        (*pieceList).push_back(rook);

        pos.column = 0;
        rook.pos = pos;
        (*pieceList).push_back(rook);

    }
    {
        piece knight;
        position pos;
        pos.column = 1;
        pos.row = 0;

        knight.nr = KNIGHT_W;
        knight.pos = pos;
        (*pieceList).push_back(knight);

        pos.column = 6;
        knight.pos = pos;
        (*pieceList).push_back(knight);

        pos.row = 7;
        knight.pos = pos;
        knight.nr = KNIGHT_B;
        (*pieceList).push_back(knight);

        pos.column = 1;
        knight.pos = pos;
        (*pieceList).push_back(knight);

    }
    {
        piece bish;
        position pos;
        pos.column = 2;
        pos.row = 0;

        bish.nr = BISH_W;
        bish.pos = pos;
        (*pieceList).push_back(bish);

        pos.column = 5;
        bish.pos = pos;
        (*pieceList).push_back(bish);

        pos.row = 7;
        bish.pos = pos;
        bish.nr = BISH_B;
        (*pieceList).push_back(bish);

        pos.column = 2;
        bish.pos = pos;
        (*pieceList).push_back(bish);

    }
    {
        piece king;
        position pos;
        pos.column = 3;
        pos.row = 0;

        king.nr = KING_W;
        king.pos = pos;
        (*pieceList).push_back(king);

        pos.row = 7;
        king.pos = pos;
        king.nr = KING_B;
        (*pieceList).push_back(king);
    }
    {
        piece queen;
        position pos;
        pos.column = 4;
        pos.row = 0;

        queen.nr = QUEEN_W;
        queen.pos = pos;
        (*pieceList).push_back(queen);

        pos.row = 7;
        queen.pos = pos;
        queen.nr = QUEEN_B;
        (*pieceList).push_back(queen);
    }
}

/* Function to convert a pieceint to it's stringname
 *  input: the number in int
 *  output: the name of the piece belonging to that number
 */
string nrToString(int nr)
{
    switch(nr){
        case PAWN_B: return "Black Pawn"; break;
        case PAWN_W: return "White Pawn"; break;
        case KING_B: return "Black King"; break;
        case KING_W: return "White King"; break;
        case BISH_B: return "Black Bishop"; break;
        case BISH_W: return "White Bishop"; break;
        case QUEEN_B: return "Black Queen"; break;
        case QUEEN_W: return "White Queen"; break;
        case KNIGHT_B: return "Black Knight"; break;
        case KNIGHT_W: return "White Knight"; break;
        case ROOK_B: return "Black Rook"; break;
        case ROOK_W: return "White Rook"; break;
    }
    return "";

}

/* Function to write the movement of a piece to a file
 * in the official Algebraic chess notation
 *  input: the piece that moved, and a bool of whether it captured another piece
 *  output: void! (And string in a textfile)
 */
void toFile(piece p, bool capture)
{
    vector<string> letterlist = {"a","b","c","d","e","f","g","h"};
    ofstream file;
    file.open(outputfile, std::ios_base::app);

    string letter = "";
    switch(p.nr){
        case KING_W: letter = "K"; break;
        case KING_B: letter = "K"; break;
        case BISH_W: letter = "B"; break;
        case BISH_B: letter = "B"; break;
        case ROOK_W: letter = "R"; break;
        case ROOK_B: letter = "R"; break;
        case QUEEN_W: letter = "Q"; break;
        case QUEEN_B: letter = "Q"; break;
        case KNIGHT_W : letter = "N"; break;
        case KNIGHT_B : letter = "N"; break;
    }

    file << letter;

    if (capture)
    {
        file << "x";
    }
    file << letterlist[7-p.pos.column];
    file << p.pos.row << endl;
    file.close();

}

/* Function that finds all the legal moves for a piece
 *
 */
void findLegalMoves(piece p)
{
    //TODO: REWORK EVERYTHING, so that every piece has it's own class (instead of just having a struct and identifier)
    //one of the functions to implement in each class would be legal move generation
    if (p.nr == PAWN_B || p.nr == PAWN_W)
    {
        int rowoffset = (p.nr == PAWN_B ? -1 : 1); //the rowoffset depends on wether the pawn is black or white
        position frontpos;
        frontpos.column = p.pos.column;
        frontpos.row = p.pos.row + rowoffset;
        piece Piece;
        //if there's no piece on the space in front of the pawn, add the front of the pawn to the list of possible positions
        if (!findPieceOnPos(frontpos, &Piece))
        {
            possiblePositions.push_back(frontpos);

            // also check both diagonal tiles
            int i = -1;
            while (i < 2)
            {
                position sidepos;
                sidepos.column = p.pos.column + i;
                sidepos.row = p.pos.row + rowoffset;
                piece Piece;
                if (findPieceOnPos(sidepos, &Piece) && Piece.nr%2 != p.nr%2)
                {//if there's a piece on the position, and it's of the other player, the pawn can take it
                    possiblePositions.push_back(sidepos);
                }
                i += 2;
            }

            if (p.pos.row == 1 && p.nr == PAWN_W)
            {//pawn can jump two tiles
               frontpos.row++; 
               if (!findPieceOnPos(frontpos, &Piece))
               {
                   possiblePositions.push_back(frontpos);
               }
            }
            if (p.pos.row == 6 && p.nr == PAWN_B)
            {//pawn can jump two tiles
               frontpos.row--; 
               if (!findPieceOnPos(frontpos, &Piece))
               {
                   possiblePositions.push_back(frontpos);
               }
            }
        }
    }
    if (p.nr == BISH_B || p.nr == BISH_W || p.nr == QUEEN_B || p.nr == QUEEN_W)
    {//diagonal movement
        
        for (int coffset = -1; coffset < 2; coffset += 2)
        {
            for (int roffset = -1; roffset < 2; roffset += 2)
            {
                position checkpos;
                checkpos.row = p.pos.row;
                checkpos.column = p.pos.column;
                bool continueflag = true;

                while(0 < checkpos.column && checkpos.column < 7 && 0 < checkpos.row && checkpos.row < 7 && continueflag)
                {
                    checkpos.column += coffset;
                    checkpos.row += roffset;
                    piece pi;
                    if (!findPieceOnPos(checkpos, &pi))
                    {
                        possiblePositions.push_back(checkpos);
                    }
                    else if (pi.nr%2 != p.nr%2)
                    {
                        possiblePositions.push_back(checkpos);
                        continueflag = false;
                    }
                    else
                    {
                        continueflag = false;
                    }
                }
            }
        }
    }
    if (p.nr == ROOK_B || p.nr == ROOK_W || p.nr == QUEEN_B || p.nr == QUEEN_W)
    {//vertical and horizontal movement
        //first check the rows
        for (int roffset = -1; roffset < 2; roffset += 2)
        {
            position checkpos;
            checkpos.row = p.pos.row;
            checkpos.column = p.pos.column;
            bool continueflag = true;

            while (0 < checkpos.row && checkpos.row < 7 && continueflag)
            {
                checkpos.row += roffset;
                piece pi;
                if (!findPieceOnPos(checkpos, &pi))
                {
                    possiblePositions.push_back(checkpos);
                }
                else if (pi.nr%2 != p.nr%2)
                {
                    possiblePositions.push_back(checkpos);
                    continueflag = false;
                }
                else
                {
                    continueflag = false;
                }
            }

        }
        //next check the columns
        for (int coffset = -1; coffset < 2; coffset += 2)
        {
            position checkpos;
            checkpos.row = p.pos.row;
            checkpos.column = p.pos.column;
            bool continueflag = true;

            while (0 < checkpos.column && checkpos.column < 7 && continueflag)
            {
                checkpos.column += coffset;
                piece pi;
                if (!findPieceOnPos(checkpos, &pi))
                {
                    possiblePositions.push_back(checkpos);
                }
                else if (pi.nr%2 != p.nr%2)
                {
                    possiblePositions.push_back(checkpos);
                    continueflag = false;
                }
                else
                {
                    continueflag = false;
                }
            }

        }

    }
    if (p.nr == KNIGHT_B || p.nr == KNIGHT_W)
    {//horsey movements
        vector<vector<int>> movelist = {{-2, -1}, {-2,1}, {-1, -2}, {-1,2}, {1,2}, {1,-2} ,{2,1}, {2,-1}};
        for (int i = 0; i < movelist.size(); i++)
        {
            position checkpos;
            checkpos.column = p.pos.column + movelist[i][0];
            checkpos.row = p.pos.row + movelist[i][1];
            piece pi;
            if (!findPieceOnPos(checkpos, &pi) || pi.nr%2 != p.nr%2)
            {
                possiblePositions.push_back(checkpos);
            }
        }
    }
    if (p.nr == KING_B || p.nr == KING_W)
    {
        position checkpos;
        for (int i = -1; i < 2; i++)
        {
            checkpos.column = p.pos.column + i;
            for (int j = -1; j < 2; j++)
            {
                piece pi;
                checkpos.row = p.pos.row + j;
                if (0 <= checkpos.row && checkpos.row <= 7 && checkpos.column <= 7 && 0 <= checkpos.column)
                {
                    if (!findPieceOnPos(checkpos, &pi) || pi.nr%2 != p.nr%2)
                    {
                        possiblePositions.push_back(checkpos);

                    }
                }
            }
        }

    }

    cout << "Found " << possiblePositions.size() << " possible positions!" << endl;
    for (int i = 0; i < possiblePositions.size(); i++)
    {
        cout << i << ": " << possiblePositions[i].row << " " << possiblePositions[i].column << endl;
    }
}


bool findPieceOnPos(position p, piece* Piece)
{
    if (7 < p.column || p.column < 0 || p.row < 0 || 7 < p.row)
    {
        return false;
    }
    for (int i = 0; i < pieceList.size(); i++)
    {
        if (pieceList[i].pos.column == p.column && pieceList[i].pos.row == p.row)
        {
            *Piece = pieceList[i];
            return true;
        }
    }
    return false;
}


position coordToPosition(int x, int y, vector<Point2f> cornerlist)
{
    for (int j = 0; j < cornerlist.size(); j++)
    {
        if (x < cornerlist[j].x && y < cornerlist[j].y)
        {
            position p;
            p.column = j%7;
            p.row = j/7;
            return p;
        }
        if (j%7 == 6 && y < cornerlist[j].y && x > cornerlist[j].x)
        {
            position p;
            p.column = 7;
            p.row = j/7;
            return p;
        }
        if (j > 41 && y > cornerlist[j].y && x < cornerlist[j].x)
        {
            position p;
            p.column = j%7;
            p.row = 7;
            return p;
        }
        if (j == 48 && y > cornerlist[j].y && x > cornerlist[j].x)
        {
            position p;
            p.column = 7;
            p.row = 7;
            return p;
        }
    }
    position p;
    p.column = -1;
    p.row = -1;
    return p;

}

void positionToCoord(position pos, int*x, int*y)
{
    (*x) = cornerlist[6].x + 10;
    (*y) = cornerlist[42].y + 10;
    for (int i = 0; i < 7; i++)
    {
        if (pos.column == i)
        {
            (*x) = cornerlist[i].x - 25;
        }

        if (pos.row == i)
        {
            (*y) = cornerlist[i*7].y - 25;
        }
    }

}

void on_mouse(int e, int x, int y, int d, void *ptr)
{
    if (e == EVENT_LBUTTONDBLCLK)
    {
       possiblePositions.clear();
       //get the position 
       position pos = coordToPosition(x,y,cornerlist);
       cout << "Clicked at position " << pos.row << " " << pos.column << endl;
       piece pi;
       if (findPieceOnPos(pos, &pi))
       {
           cout << "Found piece " << nrToString(pi.nr) << endl;
           findLegalMoves(pi);
           drawPossibleMoves = true;
       }
       else
       {
           cout << "No piece at this location!" << endl;
           drawPossibleMoves = false;
       }

    }

}
