/*******************************************************************************************
*
*   raylib - tetris42
*
*   Sample game developed by Marc Palau and Ramon Santamaria
*
*   This game has been created using raylib v1.3 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2015 Ramon Santamaria (@raysan5)
* 
*   Copyright (c) 2023 Tadej Panjtar (@tpanj)
*
********************************************************************************************/

#include "raylib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
// #define SQUARE_SIZE             20

#define GRID_HORIZONTAL_SIZE    12
#define GRID_VERTICAL_SIZE      20

#define LATERAL_SPEED           10
#define TURNING_SPEED           12
#define FAST_FALL_AWAIT_COUNTER 30

#define FADING_TIME             33

#define NAME_SIZE               20

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum GridSquare { EMPTY, MOVING, FULL, BLOCK, FADING } GridSquare;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
char player [4][NAME_SIZE] = {"FOR ", "FOR ", "FOR ", "FOR "};
char title [8 * NAME_SIZE] = "Tetris ";

int screenWidth;
int screenHeight;

static int SQUARE_SIZE;
int MAX_PLAYERS = 2;
static int Gr = 0;
int masterOffsetX = 0;
int masterOffsetY = 0;

static bool gameOver [4] = {false, false, false, false};
static bool pause = false;

// Matrices
static GridSquare grid [4][GRID_HORIZONTAL_SIZE][GRID_VERTICAL_SIZE];
static GridSquare piece [4][4][4];
static GridSquare incomingPiece [4][4][4];

// Theese variables keep track of the active piece position
static int piecePositionX[4] = {0, 0, 0, 0};
static int piecePositionY[4] = {0, 0, 0, 0};

// Game parameters
static Color fadingColor[4];
//static int fallingSpeed;           // In frames

static bool beginPlay [4] = {true, true, true, true};      // This var is only true at the begining of the game, used for the first matrix creations
static bool pieceActive [4] = {false, false, false, false};
static bool detection [4] = {false, false, false, false};
static bool lineToDelete [4] = {false, false, false, false};

// Statistics
static int level[4] = {1, 1, 1, 1};
static int lines[4] = {0, 0, 0, 0};

// Counters
static int gravityMovementCounter [4] = {0, 0, 0, 0};
static int lateralMovementCounter [4] = {0, 0, 0, 0};
static int turnMovementCounter [4] = {0, 0, 0, 0};
static int fastFallMovementCounter [4] = {0, 0, 0, 0};

static int fadeLineCounter [4] = {0, 0, 0, 0};

// Based on level
static int gravitySpeed = 30;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);         // Initialize game
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(Color C1, Color C2, Color C3);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game
static void UpdateDrawFrame(void);  // Update and Draw (one frame)

// Additional module functions
static bool Createpiece();
static void GetRandompiece();
static void ResolveFallingMovement(bool *detection, bool *pieceActive, int Gr);
static bool ResolveLateralMovement();
static bool ResolveTurnMovement();
static void CheckDetection(bool *detection, int Gr);
static void CheckCompletion(bool *lineToDelete, int Gr);
static int DeleteCompleteLines();

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
#if defined PLAYERS
    MAX_PLAYERS=PLAYERS;
#else
    if (argc < 3)
    {
        MAX_PLAYERS = 1;
    }
    else
    {
        MAX_PLAYERS = argc - 1;
        for (int p = 0; p < MAX_PLAYERS; p++)
        {
            strcat(title, argv[p + 1]);
            if (p < MAX_PLAYERS - 1)
            {
                strcat(title, " vs ");
            }
        }
    }
#endif // defined PLAYERS

    screenWidth = 1920; // GetMonitorWidth(0); // <-- BUG: Returns always 0
    screenHeight = screenWidth / 1.7777;
    if (MAX_PLAYERS > 2)
    {
        SQUARE_SIZE = screenWidth / 80;
    }
    else
    {
        SQUARE_SIZE = screenWidth / 40;
    }

    if (1 == MAX_PLAYERS)
    {
        Gr = 1;
        InitGame();
#ifdef PLAYERS
        snprintf(player[Gr], NAME_SIZE, "FOR PLAYER %d", Gr);
#else
        if (argc == 1)
        {
            strcpy(player[Gr], "");
        }
        else
        {
            strncat(player[Gr], argv[argc - 1], NAME_SIZE);
        }
#endif
    }
    else
    {
        for (int p = 0; p < MAX_PLAYERS; p++)
        {
            Gr = p;
            InitGame();
#ifdef PLAYERS
        sprintf(player[Gr], "FOR PLAYER %d", Gr + 1);
#else
        strcat(player[Gr], argv[p + 1]);
#endif
        }
    }
    SetTraceLogLevel(LOG_ERROR);
    // Initialization (Note windowTitle is unused on Android)
    InitWindow(screenWidth, screenHeight, title);
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();
        //----------------------------------------------------------------------------------
    }
#endif
    char winner[6 * NAME_SIZE] = "THE WINNER";

    if (MAX_PLAYERS > 1)
    {
        // Descending sort winner(s)
        for (int i = 0; i < MAX_PLAYERS-1; i++)
        {
            for (int j = 0; j < MAX_PLAYERS-i-1; j++)
            {
                if (lines[j] < lines[j + 1])
                {
                    int tmp_lines;
                    char tmp_player [NAME_SIZE];
                    tmp_lines = lines[j];
                    lines[j] = lines[j + 1];
                    lines[j + 1] = tmp_lines;
                    strncpy(tmp_player, player[j], NAME_SIZE);
                    strncpy(player[j], player[j + 1], NAME_SIZE);
                    strncpy(player[j + 1], tmp_player, NAME_SIZE);
                }
            }
        }
        // Determine who the winner is
        bool equal_winners = false;
        for (int p = 0; p < MAX_PLAYERS-1; p++)
        {
            if (lines[p] == lines[p+1])
            {
                equal_winners = true;
                if (p == 0)
                {
                    strcat(winner, "S ARE ");
                    strcat(winner, player[p]+4);
                }
                strcat(winner, " AND ");
                strcat(winner, player[p+1]+4);
            }
            else if (!equal_winners)
            {
                strcat(winner, " IS ");
                strcat(winner, player[p]+4);
                break;
            }
        }
        printf("%s\n", winner);
        BeginDrawing();
        DrawText(winner, GetScreenWidth()/2 - MeasureText(winner, 50)/2, GetScreenHeight()/3 - 50, 50, RED);
        EndDrawing();
        WaitTime(5.0);
    }
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame();         // Unload loaded data (textures, sounds, models...)

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------
// Game Module Functions Definition
//--------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    // Initialize game statistics
    level[Gr] = 1;
    lines[Gr] = 0;

    fadingColor[Gr] = GRAY;

    piecePositionX[Gr] = 0;
    piecePositionY[Gr] = 0;

    pause = false;

    beginPlay[Gr] = true;
    pieceActive[Gr] = false;
    detection[Gr] = false;
    lineToDelete[Gr] = false;

    // Counters
    gravityMovementCounter[Gr] = 0;
    lateralMovementCounter[Gr] = 0;
    turnMovementCounter[Gr] = 0;
    fastFallMovementCounter[Gr] = 0;

    fadeLineCounter[Gr] = 0;
    gravitySpeed = 30;

    // Initialize grid matrices
    for (int i = 0; i < GRID_HORIZONTAL_SIZE; i++)
    {
        for (int j = 0; j < GRID_VERTICAL_SIZE; j++)
        {
            if ((j == GRID_VERTICAL_SIZE - 1) || (i == 0) || (i == GRID_HORIZONTAL_SIZE - 1)) grid[Gr][i][j] = BLOCK;
            else grid[Gr][i][j] = EMPTY;
        }
    }

    // Initialize incoming piece matrices
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j< 4; j++)
        {
            incomingPiece[Gr][i][j] = EMPTY;
        }
    }
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver[Gr])
    {
        if (IsKeyPressed('P')) pause = !pause;

        if (!pause)
        {
            if (!lineToDelete[Gr])
            {
                if (!pieceActive[Gr])
                {
                    // Get another piece
                    pieceActive[Gr] = Createpiece();

                    // We leave a little time before starting the fast falling down
                    fastFallMovementCounter[Gr] = 0;
                }
                else    // Piece falling
                {
                    // Counters update
                    fastFallMovementCounter[Gr]++;
                    gravityMovementCounter[Gr]++;
                    lateralMovementCounter[Gr]++;
                    turnMovementCounter[Gr]++;

                    if (Gr == 1)
                    {
                        // We make sure to move if we've pressed the key this frame
                        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) lateralMovementCounter[Gr] = LATERAL_SPEED;
                        if (IsKeyPressed(KEY_UP)) turnMovementCounter[Gr] = TURNING_SPEED;

                        // Fall down
                        if (IsKeyDown(KEY_DOWN) && (fastFallMovementCounter[Gr] >= FAST_FALL_AWAIT_COUNTER))
                        {
                            // We make sure the piece is going to fall this frame
                            gravityMovementCounter[Gr] += gravitySpeed;
                        }
                    }

                    if (Gr == 0)
                    {
                        // We make sure to move if we've pressed the key this frame
                        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D)) lateralMovementCounter[Gr] = LATERAL_SPEED;
                        if (IsKeyPressed(KEY_W)) turnMovementCounter[Gr] = TURNING_SPEED;

                        // Fall down
                        if (IsKeyDown(KEY_S) && (fastFallMovementCounter[Gr] >= FAST_FALL_AWAIT_COUNTER))
                        {
                            // We make sure the piece is going to fall this frame
                            gravityMovementCounter[Gr] += gravitySpeed;
                        }
                    }

                    if (Gr == 2)
                    {
                        // We make sure to move if we've pressed the key this frame
                        if (IsGamepadButtonPressed(0, 8) || IsGamepadButtonPressed(0, 6)) lateralMovementCounter[Gr] = LATERAL_SPEED;
                        if (IsGamepadButtonPressed(0, 5)) turnMovementCounter[Gr] = TURNING_SPEED;

                        // Fall down
                        if (IsGamepadButtonDown(0, 7) && (fastFallMovementCounter[Gr] >= FAST_FALL_AWAIT_COUNTER))
                        {
                            // We make sure the piece is going to fall this frame
                            gravityMovementCounter[Gr] += gravitySpeed;
                        }
                    }

                    if (Gr == 3)
                    {
                        // We make sure to move if we've pressed the key this frame
                        if (IsGamepadButtonPressed(1, 8) || IsGamepadButtonPressed(1, 6)) lateralMovementCounter[Gr] = LATERAL_SPEED;
                        if (IsGamepadButtonPressed(1, 5)) turnMovementCounter[Gr] = TURNING_SPEED;

                        // Fall down
                        if (IsGamepadButtonDown(1, 7) && (fastFallMovementCounter[Gr] >= FAST_FALL_AWAIT_COUNTER))
                        {
                            // We make sure the piece is going to fall this frame
                            gravityMovementCounter[Gr] += gravitySpeed;
                        }
                    }
                    if (gravityMovementCounter[Gr] >= gravitySpeed)
                    {
                        // Basic falling movement
                        CheckDetection(&detection[0], Gr);

                        // Check if the piece has collided with another piece or with the boundings
                        ResolveFallingMovement(&detection[0], &pieceActive[0], Gr);

                        // Check if we fullfilled a line and if so, erase the line and pull down the the lines[Gr] above
                        CheckCompletion(&lineToDelete[0], Gr);

                        gravityMovementCounter[Gr] = 0;
                    }

                    // Move laterally at player's will
                    if (lateralMovementCounter[Gr] >= LATERAL_SPEED)
                    {
                        // Update the lateral movement and if success, reset the lateral counter
                        if (!ResolveLateralMovement()) lateralMovementCounter[Gr] = 0;
                    }

                    // Turn the piece at player's will
                    if (turnMovementCounter[Gr] >= TURNING_SPEED)
                    {
                        // Update the turning movement and reset the turning counter
                        if (ResolveTurnMovement()) turnMovementCounter[Gr] = 0;
                    }
                }

                // Game over logic
                for (int j = 0; j < 2; j++)
                {
                    for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
                    {
                        if (grid[Gr][i][j] == FULL)
                        {
                            gameOver[Gr] = true;
                        }
                    }
                }
                if (gameOver[Gr] == true)
                {
                    printf("Player %s reached %d lines.\n", player[Gr]+4, lines[Gr]);
                }

            }
            else
            {
                // Animation when deleting lines[Gr]
                fadeLineCounter[Gr]++;

                if (fadeLineCounter[Gr]%8 < 4) fadingColor[Gr] = MAROON;
                else fadingColor[Gr] = GRAY;

                if (fadeLineCounter[Gr] >= FADING_TIME)
                {
                    int deletedLines = 0;
                    deletedLines = DeleteCompleteLines();
                    fadeLineCounter[Gr] = 0;
                    lineToDelete[Gr] = false;

                    lines[Gr] += deletedLines;
                }
            }
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame();
            gameOver[Gr] = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(Color C1, Color C2, Color C3)
{
        if (!gameOver[Gr])
        {
            // Draw gameplay area
            Vector2 offset;
            offset.x = screenWidth/2 - (GRID_HORIZONTAL_SIZE*SQUARE_SIZE/2) - 50 + masterOffsetX;
            offset.y = screenHeight/2 - ((GRID_VERTICAL_SIZE - 1)*SQUARE_SIZE/2) + SQUARE_SIZE*2 + masterOffsetY;

            offset.y -= 2*SQUARE_SIZE;

            int controller = offset.x;

            for (int j = 0; j < GRID_VERTICAL_SIZE; j++)
            {
                for (int i = 0; i < GRID_HORIZONTAL_SIZE; i++)
                {
                    // Draw each square of the grid
                    if (grid[Gr][i][j] == EMPTY)
                    {
                        DrawLine(offset.x, offset.y, offset.x + SQUARE_SIZE, offset.y, C1 );
                        DrawLine(offset.x, offset.y, offset.x, offset.y + SQUARE_SIZE, C1 );
                        DrawLine(offset.x + SQUARE_SIZE, offset.y, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, C1 );
                        DrawLine(offset.x, offset.y + SQUARE_SIZE, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, C1 );
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[Gr][i][j] == FULL)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, C2);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[Gr][i][j] == MOVING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, C3);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[Gr][i][j] == BLOCK)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, C1);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[Gr][i][j] == FADING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, fadingColor[Gr]);
                        offset.x += SQUARE_SIZE;
                    }
                }

                offset.x = controller;
                offset.y += SQUARE_SIZE;
            }

            // Draw incoming piece (semi hardcoded)
            int offsetY = 4;
            if (MAX_PLAYERS > 2)
            {
                offsetY = 15;
            }
            offset.x = screenWidth/2 + (GRID_HORIZONTAL_SIZE*SQUARE_SIZE/2) + masterOffsetX;
            offset.y = offsetY * SQUARE_SIZE + masterOffsetY;

            int controler = offset.x;

            for (int j = 0; j < 4; j++)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (incomingPiece[Gr][i][j] == EMPTY)
                    {
                        DrawLine(offset.x, offset.y, offset.x + SQUARE_SIZE, offset.y, C1 );
                        DrawLine(offset.x, offset.y, offset.x, offset.y + SQUARE_SIZE, C1 );
                        DrawLine(offset.x + SQUARE_SIZE, offset.y, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, C1 );
                        DrawLine(offset.x, offset.y + SQUARE_SIZE, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, C1 );
                        offset.x += SQUARE_SIZE;
                    }
                    else if (incomingPiece[Gr][i][j] == MOVING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, C2);
                        offset.x += SQUARE_SIZE;
                    }
                }

                offset.x = controler;
                offset.y += SQUARE_SIZE;
            }

            DrawText(player[Gr], offset.x, offset.y - 6*SQUARE_SIZE, SQUARE_SIZE/2, GRAY);
            DrawText("INCOMING:", offset.x, offset.y - 5*SQUARE_SIZE, SQUARE_SIZE/2, GRAY);
            DrawText(TextFormat("LINES:   %04i", lines[Gr]), offset.x, offset.y + 20, SQUARE_SIZE/2, GRAY);

            if (pause) DrawText("GAME PAUSED", screenWidth/2 - MeasureText("GAME PAUSED", 40)/2, screenHeight/2 - 40, 40, GRAY);
        }
        else DrawText("PRESS [ENTER] TO PLAY AGAIN", GetScreenWidth()/2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20)/2, GetScreenHeight()/2 - 50, 20, GRAY);

}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    if (1 == MAX_PLAYERS)
    {
        Gr = 1;
        UpdateGame();
    }
    else
    {
        for (int p = 0; p < MAX_PLAYERS; p++)
        {
            Gr = p;
            UpdateGame();
        }
        Gr = 0;
    }


    BeginDrawing();

    ClearBackground(RAYWHITE);

    if (2 == MAX_PLAYERS)
    {
        masterOffsetX = -580;
        DrawGame(SKYBLUE, BLUE, DARKBLUE);
        Gr++;
        masterOffsetX = 400;
    }
    else if (2 < MAX_PLAYERS)
    {
        masterOffsetY = -255;
        masterOffsetX = -580;
        DrawGame(SKYBLUE, BLUE, DARKBLUE);
        Gr++;
        masterOffsetX = 400;
    }

    DrawGame(PURPLE, VIOLET, DARKPURPLE);

    if (2 < MAX_PLAYERS)
    {
        Gr++;
        masterOffsetY = 246;
        if (3 == MAX_PLAYERS)
        {
            masterOffsetX = 0;
        }
        else
        {
            masterOffsetX = -580;
        }
        DrawGame(GREEN, LIME, DARKGREEN);
        if (4 == MAX_PLAYERS)
        {
            Gr++;
            masterOffsetX = 400;
            DrawGame(BEIGE, BROWN, DARKBROWN);
        }

    }

    //     DrawGame(LIGHTGRAY, GRAY, DARKGRAY);

    EndDrawing();

}

//--------------------------------------------------------------------------------------
// Additional module functions
//--------------------------------------------------------------------------------------
static bool Createpiece()
{
    piecePositionX[Gr] = (int)((GRID_HORIZONTAL_SIZE - 4)/2);
    piecePositionY[Gr] = 0;

    // If the game is starting and you are going to create the first piece, we create an extra one
    if (beginPlay[Gr])
    {
        GetRandompiece();
        beginPlay[Gr] = false;
    }

    // We assign the incoming piece to the actual piece
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j< 4; j++)
        {
            piece[Gr][i][j] = incomingPiece[Gr][i][j];
        }
    }

    // We assign a random piece to the incoming one
    GetRandompiece();

    // Assign the piece to the grid
    for (int i = piecePositionX[Gr]; i < piecePositionX[Gr] + 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (piece[Gr][i - (int)piecePositionX[Gr]][j] == MOVING) grid[Gr][i][j] = MOVING;
        }
    }

    return true;
}

static void GetRandompiece()
{
    int last_piece = 6;
    int random_waight = lines[Gr] + 223;

    // Depending on nr. of lines completed increase possibilities of receive advanced piece after 100 lines
    if (GetRandomValue(0, random_waight) > 300)
    {
        last_piece = 21;
    }
    int random = GetRandomValue(0, last_piece);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            incomingPiece[Gr][i][j] = EMPTY;
        }
    }

    switch (random)
    {
        case 0: { incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; } break;    //Cube
        case 1: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; } break;    //L
        case 2: { incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][0] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; } break;    //L inversa
        case 3: { incomingPiece[Gr][0][1] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][3][1] = MOVING; } break;    //Recta
        case 4: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][1] = MOVING; } break;    //Creu tallada
        case 5: { incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][3][2] = MOVING; } break;    //S
        case 6: { incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][3][1] = MOVING; } break;    //S inversa

        case 7: { incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][3][2] = MOVING; incomingPiece[Gr][0][1] = MOVING; } break;    //S big
        case 8: { incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][3][1] = MOVING; incomingPiece[Gr][0][2] = MOVING; } break;    //S big inversa
        case 9: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][0][1] = MOVING; } break;    //-L
        case 10: { incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][0] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][3][1] = MOVING; } break;    //-L inversa
        case 11: { incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][0] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][3][2] = MOVING; } break;    //T
        case 12: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][3][2] = MOVING;} break;    //L big
        case 13: { incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][3][2] = MOVING;} break;    //Factory
        case 14: { incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][2][3] = MOVING;} break;    //Factory inversa
        case 15: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][3] = MOVING; incomingPiece[Gr][1][3] = MOVING; } break;    //L long
        case 16: { incomingPiece[Gr][1][3] = MOVING; incomingPiece[Gr][2][0] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][2][3] = MOVING; } break;    //L long inversa
        case 17: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; } break;    //I
        case 18: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][2][0] = MOVING; } break;    //U
        case 19: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][0][1] = MOVING; } break;    //+
        case 20: { incomingPiece[Gr][1][0] = MOVING; incomingPiece[Gr][1][1] = MOVING; incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][1][3] = MOVING; } break;    //f
        case 21: { incomingPiece[Gr][1][2] = MOVING; incomingPiece[Gr][2][0] = MOVING; incomingPiece[Gr][2][1] = MOVING; incomingPiece[Gr][2][2] = MOVING; incomingPiece[Gr][2][3] = MOVING; } break;    //f inversa
    }
}

static void ResolveFallingMovement(bool *detection, bool *pieceActive, int Gr)
{
    // If we finished moving this piece, we stop it
    if (*(detection + Gr))
    {
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    grid[Gr][i][j] = FULL;
                    *(detection + Gr) = false;
                    *(pieceActive +Gr) = false;
                }
            }
        }
    }
    else    // We move down the piece
    {
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    grid[Gr][i][j+1] = MOVING;
                    grid[Gr][i][j] = EMPTY;
                }
            }
        }

        piecePositionY[Gr]++;
    }
}

static bool ResolveLateralMovement()
{
    bool collision = false;

    // Piece movement
    if ((IsKeyDown(KEY_LEFT) && Gr == 1)
        || (IsKeyDown(KEY_A) && Gr == 0)
        || (IsGamepadButtonPressed(0, 8) && Gr == 2)
        || (IsGamepadButtonPressed(1, 8) && Gr == 3)
    ) // Move left
    {
        // Check if is possible to move to left
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    // Check if we are touching the left wall or we have a full square at the left
                    if ((i-1 == 0) || (grid[Gr][i-1][j] == FULL)) collision = true;
                }
            }
        }

        // If able, move left
        if (!collision)
        {
            for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
            {
                for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)             // We check the matrix from left to right
                {
                    // Move everything to the left
                    if (grid[Gr][i][j] == MOVING)
                    {
                        grid[Gr][i-1][j] = MOVING;
                        grid[Gr][i][j] = EMPTY;
                    }
                }
            }

            piecePositionX[Gr]--;
        }
    }
    else if ((IsKeyDown(KEY_RIGHT) && Gr == 1)
            || (IsKeyDown(KEY_D) && Gr == 0)
            || (IsGamepadButtonPressed(0, 6) && Gr == 2)
            || (IsGamepadButtonPressed(1, 6) && Gr == 3)
    )  // Move right
    {
        // Check if is possible to move to right
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    // Check if we are touching the right wall or we have a full square at the right
                    if ((i+1 == GRID_HORIZONTAL_SIZE - 1) || (grid[Gr][i+1][j] == FULL))
                    {
                        collision = true;

                    }
                }
            }
        }

        // If able move right
        if (!collision)
        {
            for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
            {
                for (int i = GRID_HORIZONTAL_SIZE - 1; i >= 1; i--)             // We check the matrix from right to left
                {
                    // Move everything to the right
                    if (grid[Gr][i][j] == MOVING)
                    {
                        grid[Gr][i+1][j] = MOVING;
                        grid[Gr][i][j] = EMPTY;
                    }
                }
            }

            piecePositionX[Gr]++;
        }
    }

    return collision;
}

static bool ResolveTurnMovement()
{
    // Input for turning the piece
    if ( (IsKeyDown(KEY_UP) && Gr == 1)
       || (IsKeyDown(KEY_W) && Gr == 0)
       || (IsGamepadButtonPressed(0, 5) && Gr == 2)
       || (IsGamepadButtonPressed(1, 5) && Gr == 3)
    )
    {
        GridSquare aux;
        bool checker = false;

        // Check all turning possibilities
        if ((grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr]] == MOVING) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr]] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr]] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 3] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr]] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr]] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 3] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 3] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 3] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr]][piecePositionY[Gr]] == MOVING) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 3] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 3] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr]] == MOVING) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 2] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 2] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 1] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr]] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr]] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 3] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 1] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 1] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 2] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 3] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 3] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr]] == MOVING) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 1] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 1] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 2] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr]] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr]] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 3] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 2] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 3][piecePositionY[Gr] + 2] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr]][piecePositionY[Gr] + 1] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 3] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 3] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 1] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 2] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 2] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 1] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 1] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 1] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 2] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 1] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 1] != MOVING)) checker = true;

        if ((grid[Gr][piecePositionX[Gr] + 1][piecePositionY[Gr] + 2] == MOVING) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 2] != EMPTY) &&
            (grid[Gr][piecePositionX[Gr] + 2][piecePositionY[Gr] + 2] != MOVING)) checker = true;

        if (!checker)
        {
            aux = piece[Gr][0][0];
            piece[Gr][0][0] = piece[Gr][3][0];
            piece[Gr][3][0] = piece[Gr][3][3];
            piece[Gr][3][3] = piece[Gr][0][3];
            piece[Gr][0][3] = aux;

            aux = piece[Gr][1][0];
            piece[Gr][1][0] = piece[Gr][3][1];
            piece[Gr][3][1] = piece[Gr][2][3];
            piece[Gr][2][3] = piece[Gr][0][2];
            piece[Gr][0][2] = aux;

            aux = piece[Gr][2][0];
            piece[Gr][2][0] = piece[Gr][3][2];
            piece[Gr][3][2] = piece[Gr][1][3];
            piece[Gr][1][3] = piece[Gr][0][1];
            piece[Gr][0][1] = aux;

            aux = piece[Gr][1][1];
            piece[Gr][1][1] = piece[Gr][2][1];
            piece[Gr][2][1] = piece[Gr][2][2];
            piece[Gr][2][2] = piece[Gr][1][2];
            piece[Gr][1][2] = aux;
        }

        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[Gr][i][j] == MOVING)
                {
                    grid[Gr][i][j] = EMPTY;
                }
            }
        }

        for (int i = piecePositionX[Gr]; i < piecePositionX[Gr] + 4; i++)
        {
            for (int j = piecePositionY[Gr]; j < piecePositionY[Gr] + 4; j++)
            {
                if (piece[Gr][i - piecePositionX[Gr]][j - piecePositionY[Gr]] == MOVING)
                {
                    grid[Gr][i][j] = MOVING;
                }
            }
        }

        return true;
    }

    return false;
}

static void CheckDetection(bool *detection, int Gr)
{
    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
        {
            if ((grid[Gr][i][j] == MOVING) && ((grid[Gr][i][j+1] == FULL) || (grid[Gr][i][j+1] == BLOCK))) *(detection + Gr) = true;
        }
    }
}

static void CheckCompletion(bool *lineToDelete, int Gr)
{
    int calculator = 0;

    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        calculator = 0;
        for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
        {
            // Count each square of the line
            if (grid[Gr][i][j] == FULL)
            {
                calculator++;
            }

            // Check if we completed the whole line
            if (calculator == GRID_HORIZONTAL_SIZE - 2)
            {
                *(lineToDelete + Gr) = true;
                calculator = 0;
                // points++;

                // Mark the completed line
                for (int z = 1; z < GRID_HORIZONTAL_SIZE - 1; z++)
                {
                    grid[Gr][z][j] = FADING;
                }
            }
        }
    }
}

static int DeleteCompleteLines()
{
    int deletedLines = 0;

    // Erase the completed line
    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        while (grid[Gr][1][j] == FADING)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                grid[Gr][i][j] = EMPTY;
            }

            for (int j2 = j-1; j2 >= 0; j2--)
            {
                for (int i2 = 1; i2 < GRID_HORIZONTAL_SIZE - 1; i2++)
                {
                    if (grid[Gr][i2][j2] == FULL)
                    {
                        grid[Gr][i2][j2+1] = FULL;
                        grid[Gr][i2][j2] = EMPTY;
                    }
                    else if (grid[Gr][i2][j2] == FADING)
                    {
                        grid[Gr][i2][j2+1] = FADING;
                        grid[Gr][i2][j2] = EMPTY;
                    }
                }
            }

             deletedLines++;
        }
    }

    return deletedLines;
}
