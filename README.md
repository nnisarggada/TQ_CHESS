# Chess movement detection and tracking using OpenCV 3.4

This repo contains the code for a chess movement detection and tracking algorithm. 
The algorithm is made in C++, using CMake. 
Tested on a Fedora 27 Linux system with OpenCV 3.4 and CMake 3.11.2

## How to use/install?
After cloning the repo, use the commands
```
cmake CMakeLists.txt
make
./chessdetection
```
By default the camera int is 2, you should change this if you want to use a different camera. This is explained in the code.

## How does it work?

The algorithm uses standard backgroundsubtraction.

[short demonstration](https://www.youtube.com/watch?v=F_yXh4eZ7V4)

The left screen is the actual videofeed, the middle screen is the generated background, and the right screen is a binary mask of the foreground.


The approach it currently uses is a fixed-camera pointed at the chess board with strong lighting. The algorithm uses foregrounddetection to detect "movementholes".

After launching the program, the user can play around with the camera and lighting using an empty board. Once the user is happy with the video and the corners are detected, they can press "enter" and fill the board with the pieces. 

Once the background has updated, the game is ready to be played.
Every time that the foregroundmask has 2 contours for 80 frames (counter on the foregroundmask), it will register a move. It looks at the centrepoint of each contour, calculates what tile it's on and looks if there are any pieces on those tiles. Then, based on who's turn it is, it recognises the move and wether or not a piece has been taken.
The program automatically writes the move to a file called "chess.txt" using the Algebraic chess notation.

## List of features, problems & todo's

Features:
* Detect a chessboard
* Detect the movement of a piece
* Track the movement of a piece, with support of:
    * normal moves (eg e2->e4)
    * moves where an opponents piece is taken
* Write the moves in algebraic chess notation to a file
* When a piece on the live feed is clicked, it shows all the possible moves this piece can make.
* Threshold for the movement can be set on-the-fly.


Problems:
* Pieces on a same-coloured tile can sometimes go undetected.
* The camera, board and surroundings need to stay perfectly still, or it will trigger movement, and possible false positives.
* Because of the cooldown period after making a move, users can't blitz; the game would be going too fast for the algorithm to register.
* There's no detection for promotion, castling or en-passant. However, this shouldn't be a hard fix.
* When making a list of possible moves, it doesn't take the king being in check in account. E.g. when it's necessary to move a piece to block a check, it will still show all the possible moves (instead of just the ones that would block the check)

Todos:
* Add support for castling, promotion and en-passant.
* Play around with lighting to see if I can fix those issues.
* Enable boarddetection whilst the pieces are already on the board
* Detect the pieces based on how they look, not on where they are in the beginning of the game.
* Implementing check-detection would be pretty neat.
* Implementing an "illegal move"-detection would also be pretty neat!
* Rewrite everything more C++-like (with a class per piece, instead of a struct)


## URLs to YT demos

[short demonstration](https://www.youtube.com/watch?v=F_yXh4eZ7V4)
[long demonstratie](https://www.youtube.com/watch?v=w67BJXWnMkw)
