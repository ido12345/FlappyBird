#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#define DEG2RAD(deg) ((deg) * 3.14159265358979323846 / 180.0)

#include <stdio.h>
void AttachConsoleToWindow() {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stderr);
}

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "resources.h"

#include "ML.h"

// #define NN
// #define DEBUG_CONSOLE
#define DEBUG_FPS
#define AUTOPILOT

#ifdef NN
Network BirdNN;
Network BirdNNGrad;
Network BirdTargetNN;
#    define RANGE_DISTANCE 1

#    ifdef RANGE_DISTANCE
#        define RAND_RANGE -RANGE_DISTANCE, RANGE_DISTANCE
#    else // RANGE_DISTANCE
#        define RAND_RANGE 0, 0.5
#    endif // RANGE_DISTANCE

#    define ACTIONS_AMOUNT 2

double learnRate = 0.1;

const double PipePassReward = 1;
const double AliveReward = 0.1;
const double DeathReward = -1;
const double PipeDeathReward = -0.5;

#    define STEPS_AMOUNT 10000000

unsigned long long stepIndex = 0;
unsigned long long stepCounter = 0;
unsigned long long gameCounter = 0;
Step birdSteps[STEPS_AMOUNT];

#    define INPUT_MAT 1, 2

bool NewNetwork = false;
#endif // NN
double timeSpeedup = 1;

HDC hDoubleBufferDC = NULL;
HBITMAP hCopyBitmap = NULL;
HBITMAP hSavedBitmap = NULL;
int hTempDCWidth = 0, hTempDCHeight = 0;

HDC hBackgroundDC = NULL;

HDC hBirdDC = NULL;

HDC hPipeDC = NULL;

HDC hDigitDC = NULL;

// HDC hNetworkDC = NULL;

#ifdef DEBUG_RECTS
HBRUSH hBackgroundBrush;
HBRUSH hBirdBrush;
HBRUSH hPipeBrush;
#endif // DEBUG_RECTS

HBITMAP hBackgroundBitmap;

BITMAP BackgroundBitmap;

HBITMAP hBirdBitmap;
HBITMAP hBirdBitmapMidflap;
HBITMAP hBirdBitmapFlap;

BITMAP *BirdBitmapPTR;
BITMAP BirdBitmap;
BITMAP BirdBitmapMidflap;
BITMAP BirdBitmapFlap;

HBITMAP hPipeBitmapTopEnd;
HBITMAP hPipeBitmapBottomEnd;
HBITMAP hPipeBitmapRepeat;

BITMAP PipeBitmapTopEnd;
BITMAP PipeBitmapBottomEnd;
BITMAP PipeBitmapRepeat;

HFONT hDebugFont;

HBITMAP hDigitBitmaps[10];
BITMAP DigitBitmaps[10];

// HBITMAP hNetworkBitmap;
// BITMAP NetworkBitmap;

ma_engine SoundEngine;

void InitSoundEngine() {
    ma_engine_init(NULL, &SoundEngine);
}

void ShutdownSoundEngine() {
    ma_engine_uninit(&SoundEngine);
}

const void *LoadResourceData(UINT resourceID, LPCSTR resourceType, DWORD *size) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceID), resourceType);
    if (!hResource)
        return NULL;

    HGLOBAL hGlobal = LoadResource(NULL, hResource);
    if (!hGlobal)
        return NULL;

    *size = SizeofResource(NULL, hResource);
    return LockResource(hGlobal);
}

void SoundFreeCallback(void* pUserData, ma_sound* sound) {
    ma_decoder* decoder = (ma_decoder*)pUserData;
    
    // ma_sound_uninit(sound);
    // ma_decoder_uninit(decoder);
    // free(sound);
    // free(decoder);
}

void PlayMemorySound(UINT resourceID) {
    if (timeSpeedup > 10)
        return;

    DWORD dataSize = 0;
    const void *data = LoadResourceData(resourceID, "WAV", &dataSize);

    if (!data || !dataSize) {
        MessageBox(NULL, "Failed to load sound resource.", "Error!", MB_OK | MB_ICONERROR);
        return;
    }

    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    ma_decoder *decoder = malloc(sizeof(ma_decoder));
    if (ma_decoder_init_memory(data, dataSize, &decoderConfig, decoder) != MA_SUCCESS) {
        free(decoder);
        return;
    }

    ma_sound *sound = malloc(sizeof(ma_sound));
    ma_result result = ma_sound_init_from_data_source(
        &SoundEngine,
        decoder,
        MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_DECODE,
        NULL,
        sound
    );
    if (result != MA_SUCCESS) {
        ma_decoder_uninit(decoder);
        free(decoder);
        free(sound);
        return;
    }

    ma_sound_set_end_callback(sound, SoundFreeCallback, decoder);
    ma_sound_start(sound);
}
// typedef struct {
//     ma_sound sound;
//     ma_decoder decoder;
//     void* dataCopy;
//     int finished;
// } SoundInstance;

// #define MAX_SOUNDS 64
// SoundInstance* activeSounds[MAX_SOUNDS];

// void SoundEndCallback(void* pUserData, ma_sound* sound)
// {
//     SoundInstance* inst = (SoundInstance*)pUserData;
//     inst->finished = 1;
// }

// void UpdateSounds()
// {
//     for (int i = 0; i < MAX_SOUNDS; i++) {
//         SoundInstance* inst = activeSounds[i];
//         if (!inst) continue;

//         if (inst->finished) {
//             ma_sound_uninit(&inst->sound);
//             ma_decoder_uninit(&inst->decoder);

//             free(inst->dataCopy);
//             free(inst);

//             activeSounds[i] = NULL;
//         }
//     }
// }

// void PlayMemorySound(UINT resourceID)
// {
//     DWORD dataSize = 0;
//     const void* data = LoadResourceData(resourceID, "WAV", &dataSize);
//     if (!data || !dataSize) return;

//     SoundInstance* inst = malloc(sizeof(SoundInstance));
//     memset(inst, 0, sizeof(SoundInstance));

//     inst->dataCopy = malloc(dataSize);
//     memcpy(inst->dataCopy, data, dataSize);

//     ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);

//     if (ma_decoder_init_memory(inst->dataCopy, dataSize, &decoderConfig, &inst->decoder) != MA_SUCCESS) {
//         free(inst->dataCopy);
//         free(inst);
//         return;
//     }

//     if (ma_sound_init_from_data_source(&SoundEngine, &inst->decoder, MA_SOUND_FLAG_ASYNC, NULL, &inst->sound) != MA_SUCCESS) {
//         ma_decoder_uninit(&inst->decoder);
//         free(inst->dataCopy);
//         free(inst);
//         return;
//     }

//     ma_sound_set_end_callback(&inst->sound, SoundEndCallback, inst);
//     ma_sound_start(&inst->sound);

//     for (int i = 0; i < MAX_SOUNDS; i++) {
//         if (!activeSounds[i]) {
//             activeSounds[i] = inst;
//             break;
//         }
//     }
// }

#define SCREEN_HEIGHT 1000
#define SCREEN_WIDTH 1500
#define BIRD_HEIGHT 50
#define BIRD_WIDTH 60
// #define DELTA_TIME 0.01
#define DISTANCE_RATIO 200 // 377.95 // (10 cm) in (pixels)

// make the bird jump x times his height
#define FLAP_HEIGHT (BIRD_HEIGHT * 2.5)

// distance between 2 pipe obstacles
#define PIPE_DISTANCE 400
// gap the bird has to jump through
#define PIPE_GAP 200
// the width of a pipe obstacle
#define PIPE_WIDTH 100

#define RECT_WIDTH(rect) (((rect).right) - ((rect).left))
#define RECT_HEIGHT(rect) (((rect).bottom) - ((rect).top))

#define GETBIRDRECT() {                                 \
    .top = (long)(BirdPosition.y) - BIRD_HEIGHT / 2,    \
    .left = (long)(BirdPosition.x) - BIRD_WIDTH / 2,    \
    .bottom = (long)(BirdPosition.y) + BIRD_HEIGHT / 2, \
    .right = (long)(BirdPosition.x) + BIRD_WIDTH / 2,   \
};

typedef struct PointDouble {
    double x;
    double y;
} PointDouble;

typedef struct ObstacleNode {
    PointDouble obstacle;
    struct ObstacleNode *next;
} ObstacleNode;

HWND mainHwnd;

PointDouble BirdPosition = {
    .x = min(SCREEN_WIDTH / 2, 360),
    .y = SCREEN_HEIGHT / 2,
};
ObstacleNode *PipesList = NULL;
ObstacleNode *NextObstacle = NULL;
int PassedPipes = 0;
const double gravity = 10;
double birdVelocity = 0;
double pipeVelocity = 225;

double backgroundVelocity = 45;
PointDouble BackgroundPosition = {
    .x = 0,
    .y = 0,
};

bool BirdFlap = false;

bool RectCollision(RECT r1, RECT r2) {
    return ((r2.bottom > r1.top) && (r1.bottom > r2.top) && // veritical alignment
            (r2.right > r1.left) && (r1.right > r2.left));  // horizontal alignment
}

// r1 is a rectangle smaller than r2, this will scale r1 and save its aspect ratio so that it fits into r2
RECT RectFitBigger(RECT r1, RECT r2) {
    if (RECT_WIDTH(r1) > RECT_WIDTH(r2) && RECT_HEIGHT(r1) > RECT_HEIGHT(r2)) {
        // r1 is already covering r2
        return r1;
    }
    RECT scaledR1 = r1;
    double scale = min((double)((double)RECT_WIDTH(r2) / (double)RECT_WIDTH(r1)),
        (double)((double)RECT_HEIGHT(r2) / (double)RECT_HEIGHT(r1)));
    scaledR1.right = scaledR1.left + (RECT_WIDTH(r1) * scale);
    scaledR1.bottom = scaledR1.top + (RECT_HEIGHT(r1) * scale);
    return scaledR1;
}

// r1 is a rectangle smaller than r2, this will scale r1 and save its aspect ratio so that it covers r2
RECT RectCoverBigger(RECT r1, RECT r2) {
    if (RECT_WIDTH(r1) > RECT_WIDTH(r2) && RECT_HEIGHT(r1) > RECT_HEIGHT(r2)) {
        // r1 is already covering r2
        return r1;
    }
    RECT scaledR1 = r1;
    double scale = max((double)((double)RECT_WIDTH(r2) / (double)RECT_WIDTH(r1)),
        (double)((double)RECT_HEIGHT(r2) / (double)RECT_HEIGHT(r1)));
    scaledR1.right = scaledR1.left + (RECT_WIDTH(r1) * scale);
    scaledR1.bottom = scaledR1.top + (RECT_HEIGHT(r1) * scale);
    return scaledR1;
}

RECT RectScaleBy(RECT r1, double scaleBy) {
    r1.right = r1.left + RECT_WIDTH(r1) * scaleBy;
    r1.bottom = r1.top + RECT_HEIGHT(r1) * scaleBy;
    return r1;
}

RECT RectHorizontalCenterTo(RECT r1, RECT r2) {
    int xCenter = (r2.right + r2.left) / 2;

    int originalWidth = RECT_WIDTH(r1);

    r1.left = xCenter - originalWidth / 2;
    r1.right = xCenter + originalWidth / 2;

    return r1;
}

RECT RectVeritcalCenterTo(RECT r1, RECT r2) {
    int yCenter = (r2.top + r2.bottom) / 2;

    int originalHeight = RECT_HEIGHT(r1);

    r1.top = yCenter - originalHeight / 2;
    r1.bottom = yCenter + originalHeight / 2;

    return r1;
}

// puts r1 in the center of r2
RECT RectCenterTo(RECT r1, RECT r2) {
    int xCenter = (r2.right + r2.left) / 2;
    int yCenter = (r2.top + r2.bottom) / 2;

    int originalWidth = RECT_WIDTH(r1);
    int originalHeight = RECT_HEIGHT(r1);

    r1.left = xCenter - originalWidth / 2;
    r1.top = yCenter - originalHeight / 2;
    r1.right = r1.left + originalWidth;
    r1.bottom = r1.top + originalHeight;

    return r1;
}

bool BirdPipeCollision() {
    RECT ClientRect;
    GetClientRect(mainHwnd, &ClientRect);

    if (!NextObstacle)
        return false;

    // for (ObstacleList *i = PipesList; i; i = i->next) {

    RECT topPipe = {
        .top = ClientRect.top,
        .left = NextObstacle->obstacle.x,
        .bottom = NextObstacle->obstacle.y,
        .right = NextObstacle->obstacle.x + PIPE_WIDTH,
    };
    RECT bottomPipe = {
        .top = topPipe.bottom + PIPE_GAP,
        .left = NextObstacle->obstacle.x,
        .bottom = ClientRect.bottom,
        .right = NextObstacle->obstacle.x + PIPE_WIDTH,
    };

    RECT BirdRect = GETBIRDRECT();
    if (RectCollision(BirdRect, topPipe) || RectCollision(BirdRect, bottomPipe)) {
        return true;
    }
    // }
    return false;
}

bool BirdWindowCollision() {
    RECT ClientRect;
    GetClientRect(mainHwnd, &ClientRect);

    if (BirdPosition.y + BIRD_HEIGHT / 2 > ClientRect.bottom)
        return true;
    if (BirdPosition.y - BIRD_HEIGHT / 2 < ClientRect.top)
        return true;
    if (BirdPosition.x + BIRD_WIDTH / 2 > ClientRect.right)
        return true;
    if (BirdPosition.x - BIRD_WIDTH / 2 < ClientRect.left)
        return true;
    return false;
}

void freeObstacleList(ObstacleNode *head) {
    if (!head)
        return;
    freeObstacleList(head->next);
    // if (head->next) {
    //     free(head->next);
    //     head->next = NULL;
    // }
    free(head);
}

ObstacleNode *NewObstacleList(PointDouble data) {
    ObstacleNode *temp = malloc(sizeof(*temp));
    temp->obstacle = data;
    temp->next = NULL;
    return temp;
}

bool CheckPassedPipeObstacle() {
    if (!NextObstacle) {
        NextObstacle = PipesList;
        return false;
    }
    if ((NextObstacle->obstacle.x + PIPE_WIDTH) < (BirdPosition.x - BIRD_WIDTH / 2)) {
        PassedPipes++;
        NextObstacle = NextObstacle->next;
        PlayMemorySound(IDS_POINT);
        return true;
    }
    return false;
}

void CheckNewPipeObstacle() {
    RECT ClientRect;
    GetClientRect(mainHwnd, &ClientRect);
    if (!PipesList) {
        int obstacleY = rand_int(ClientRect.top + 10, ClientRect.bottom - 10 - PIPE_GAP);
        PipesList = NewObstacleList((PointDouble){ClientRect.right, obstacleY});
        return;
    }
    ObstacleNode *last = PipesList;
    while (last && last->next) {
        last = last->next;
    }
    if (last->obstacle.x + PIPE_DISTANCE <= ClientRect.right) {
        int obstacleY = rand_int(ClientRect.top + 10, ClientRect.bottom - 10 - PIPE_GAP);
        last->next = NewObstacleList((PointDouble){ClientRect.right, obstacleY});
    }
}

void CheckClearPipeObstacle() {
    ObstacleNode *first = PipesList;
    if (first && first->obstacle.x + PIPE_WIDTH <= 0) {
        PipesList = PipesList->next;
        free(first);
    }
}

void PhysicsStep(double deltaTime) {
    double prevVelocity = birdVelocity;
    birdVelocity += gravity * deltaTime * DISTANCE_RATIO;
    double deltaY = (birdVelocity + prevVelocity) * deltaTime / 2;
    BirdPosition.y += deltaY;

    for (ObstacleNode *i = PipesList; i; i = i->next) {
        i->obstacle.x -= pipeVelocity * deltaTime;
    }

    BackgroundPosition.x -= backgroundVelocity * deltaTime;
}

LARGE_INTEGER frequency;
LARGE_INTEGER prevTime;
LARGE_INTEGER prevFlapTime;

void AnimateBirdFlap() {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    double elapsedTimeSinceFlap = (double)(currentTime.QuadPart - prevFlapTime.QuadPart) / frequency.QuadPart;
    if (elapsedTimeSinceFlap < 0.1) {
        BirdBitmapPTR = &BirdBitmapFlap;
        SelectObject(hBirdDC, hBirdBitmapFlap);
    } else if (elapsedTimeSinceFlap < 0.2) {
        BirdBitmapPTR = &BirdBitmapMidflap;
        SelectObject(hBirdDC, hBirdBitmapMidflap);
    } else {
        BirdBitmapPTR = &BirdBitmap;
        SelectObject(hBirdDC, hBirdBitmap);
    }
}

double TargetFPS = 20000.0; // Frames Per Second
double accumulatedRenderTime = 0;

double TargetPPS = 60.0; // Physics Per Second
double accumulatedPhysicsTime = 0;

#ifdef NN
double TargetSPS = 10.0; // Steps Per Second (bird steps)
double accumulatedStepTime = 0;
#endif // NN

double countedFPS = 0;

void InitializeGame() {
    RECT ClientRect;
    GetClientRect(mainHwnd, &ClientRect);

    birdVelocity = 0;
    BirdFlap = false;
    BirdPosition.x = min(RECT_WIDTH(ClientRect) / 2, 360),
    BirdPosition.y = RECT_HEIGHT(ClientRect) / 2;

    if (PipesList) {
        freeObstacleList(PipesList);
        PipesList = NULL;
    }
    int obstacleY = rand_int(ClientRect.top + 10, ClientRect.bottom - 10 - PIPE_GAP);

    PipesList = NewObstacleList((PointDouble){ClientRect.right, obstacleY});
    NextObstacle = PipesList;

    PassedPipes = 0;

    BackgroundPosition.x = 0;
    BackgroundPosition.y = 0;

    BirdBitmapPTR = &BirdBitmap;
    SelectObject(hBirdDC, hBirdBitmap);
    prevFlapTime.QuadPart = 0;
}

void GameOver() {
    InitializeGame();
}

#ifdef NN
void ReinforcementLearning() {
    // PRINT_NETWORK(BirdNN);

    for (int batching = 0; batching < 5; batching++) {
        int batchSize = 64;
        batchSize = min(stepCounter - 1, batchSize);

        // int batchIndexes[batchSize] = {-1};
        Step *batchSteps = malloc(sizeof(*batchSteps) * batchSize);

        for (int i = 0; i < batchSize; i++) {
            int randIdx = rand_int(0, stepCounter - 2);
            batchSteps[i] = birdSteps[randIdx];
            // PRINT_MAT(&batchSteps[i].state);                                                                                                                                           // PRINT
            // printf("action = %d | output = %f | reward = %f, death = %s\n", batchSteps[i].action, batchSteps[i].output, batchSteps[i].reward, batchSteps[i].death ? "true" : "false"); // PRINT
        }

        Matrix QTargets = matrix_new(batchSize, ACTIONS_AMOUNT);

        calc_QTargets(&BirdTargetNN, &QTargets, batchSteps);

        // PRINT_MAT(&QTargets); // PRINT

        Network_Q_backprop(&BirdNN, &BirdNNGrad, &QTargets, batchSteps);

        // Network_print(&BirdNNGrad, "BirdNNGrad", true); // PRINT

        Network_gradient_descent(&BirdNN, &BirdNNGrad, learnRate);

        matrix_free(&QTargets);
        free(batchSteps);
    }

    // printf("Rate: %f\n", learnRate);        // PRINT
    // printf("Step Counter: %llu\n", stepCounter); // PRINT
    // printf("Step Index: %llu\n", stepIndex);     // PRINT
    // Network_print(&BirdNN, "BirdNN", true); // PRINT

    if (gameCounter % 10 == 0) {
        Network_copy(&BirdTargetNN, &BirdNN);
    }
}
#endif

#ifdef DEBUG_FPS
unsigned long long framesPerSecond = 0;
LARGE_INTEGER lastFramesCheck;
#endif

DWORD WINAPI GameLoop(void *lpParam) {
    (void)lpParam;
#ifdef NN
    double stepDelta = 1 / TargetSPS;
    int lastStepIndex = -1;
    int currentStepIndex = -1;
#endif
    double physicsDelta = 1 / TargetPPS;
    double renderDelta = 1 / TargetFPS;

    while (1) {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        double elapsedTime = (double)(currentTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;
        prevTime = currentTime;

        accumulatedPhysicsTime += elapsedTime * timeSpeedup;
        accumulatedRenderTime += elapsedTime; // * timeSpeedup;
#ifdef NN
        accumulatedStepTime += elapsedTime * timeSpeedup;
#endif // NN

#ifdef DEBUG_FPS
        double elapsedTimeSinceFrames = (double)(currentTime.QuadPart - lastFramesCheck.QuadPart) / frequency.QuadPart;
        if (elapsedTimeSinceFrames >= 1) {
            countedFPS = (double)framesPerSecond / elapsedTimeSinceFrames;
            lastFramesCheck = currentTime;
            framesPerSecond = 0;
        }
#endif

#ifdef NN
        if (NewNetwork == true) {
            Network_xavier_init(&BirdNN);
            stepCounter = 0;
            stepIndex = 0;
            NewNetwork = false;
        }
#endif // NN

        while (accumulatedPhysicsTime >= physicsDelta) {
            accumulatedPhysicsTime -= physicsDelta;

#ifdef NN
            currentStepIndex = stepIndex;
            if (accumulatedStepTime >= stepDelta) {
                accumulatedStepTime -= stepDelta;

                stepIndex++;
                if (stepIndex >= STEPS_AMOUNT) {
                    stepIndex = 0;
                }

                stepCounter = min(stepCounter + 1, STEPS_AMOUNT);

                RECT ClientRect;
                GetClientRect(mainHwnd, &ClientRect);

                NETWORK_IN(&BirdNN).data[0] = BirdPosition.y / RECT_HEIGHT(ClientRect);
                NETWORK_IN(&BirdNN).data[1] = NextObstacle->obstacle.y / RECT_HEIGHT(ClientRect);

                // PRINT_MAT(&NETWORK_IN(BirdNN)); // PRINT
                // print_Network(&BirdNN, "BirdNN", true); // PRINT

                Network_forward(&BirdNN);

                // PRINT_MAT(&NETWORK_OUT(BirdNN)); // PRINT
                // print_Network(&BirdNN, "BirdNN", true); // PRINT

                // printf("\n"); // PRINT

                if (MAT_AT(&NETWORK_OUT(&BirdNN), 0, 0) > MAT_AT(&NETWORK_OUT(&BirdNN), 0, 1)) {
                    BirdFlap = true;
                    birdSteps[currentStepIndex].action = 0;
                    birdSteps[currentStepIndex].output = MAT_AT(&NETWORK_OUT(&BirdNN), 0, 0);
                } else {
                    birdSteps[currentStepIndex].action = 1;
                    birdSteps[currentStepIndex].output = MAT_AT(&NETWORK_OUT(&BirdNN), 0, 1);
                }
                matrix_copy(&birdSteps[currentStepIndex].state, &NETWORK_IN(&BirdNN));
                // printf("step index = %d\n", currentStepIndex); // PRINT
            }
#endif

#ifdef AUTOPILOT
            if (NextObstacle) {
                if (BirdPosition.y + BIRD_HEIGHT >= NextObstacle->obstacle.y + PIPE_GAP) {
                    BirdFlap = true;
                }
            }
#endif

            if (BirdFlap == true) {
                prevFlapTime = currentTime;
                birdVelocity = -sqrt(2 * gravity * DISTANCE_RATIO * FLAP_HEIGHT);
                BirdFlap = false;
#if !defined(AUTOPILOT) && !defined(NN)
                PlayMemorySound(IDS_FLAP);
#endif
            }
            PhysicsStep(physicsDelta);
            CheckClearPipeObstacle();
            CheckNewPipeObstacle();
            #ifdef NN
            bool passedPipe = CheckPassedPipeObstacle();
            #else // NN
            CheckPassedPipeObstacle();
            #endif // NN
            AnimateBirdFlap();

#ifdef NN
            if (lastStepIndex != currentStepIndex) {
                lastStepIndex = currentStepIndex;
                birdSteps[currentStepIndex].reward = passedPipe ? PipePassReward : AliveReward;
            }
#endif // NN

            bool windowCollision = BirdWindowCollision();
            bool pipeCollision = BirdPipeCollision();
            if (windowCollision || pipeCollision) {
#ifdef NN
                lastStepIndex = currentStepIndex;
                birdSteps[currentStepIndex].death = true;
                if (windowCollision)
                    birdSteps[currentStepIndex].reward = DeathReward;
                else
                    birdSteps[currentStepIndex].reward = PipeDeathReward;

                // printf("reward = %f\n", birdSteps[currentStepIndex].reward); // PRINT

                gameCounter++;
                ReinforcementLearning();
#endif // NN

#if !defined(AUTOPILOT) && !defined(NN)
                PlayMemorySound(IDS_DIE);
#endif
                GameOver();
            }
        }

        if (accumulatedRenderTime >= renderDelta) {
            accumulatedRenderTime -= renderDelta;
            InvalidateRect(mainHwnd, NULL, false);
            UpdateWindow(mainHwnd);
        }
    }
}

void DrawLine(HDC hdc, int x1, int y1, int x2, int y2, HPEN hPen) {
    HPEN hOldPen = SelectObject(hdc, hPen);

    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);

    SelectObject(hdc, hOldPen);
}

COLORREF NetworkWeightColor(float weight) {
    if (weight >= 0) {
        return RGB(0, sigmoidf(weight) * 255.0, 0);
    } else {
        return RGB(-sigmoidf(weight) * 255.0, 0, 0);
    }
}
COLORREF NetworkBiasColor(float bias) {
    if (bias >= 0) {
        return RGB(0, sigmoidf(bias) * 255.0, 0);
    } else {
        return RGB(-sigmoidf(bias) * 255.0, 0, 0);
    }
}

void DrawNetwork(HDC hdc, Network *nn, RECT NetworkRect) {
#ifdef DEBUG_RECTS
    FillRect(hdc, &NetworkRect, GetStockObject(LTGRAY_BRUSH));
#endif // DEBUG_RECTS

    // i is the layer
    // j is the node
    // k is the amount of nodes on that layer
    /*
#define NODE_POINT(i, j, k) ((PointDouble){                                                                                     \
    .x = NetworkRect.left + ((i) *RECT_WIDTH(NetworkRect) / (nn->count + 1)) + (RECT_WIDTH(NetworkRect) / (nn->count + 1) / 2), \
    .y = NetworkRect.top + ((j) * (RECT_HEIGHT(NetworkRect) / (k))) + (RECT_HEIGHT(NetworkRect) / (k) / 2),                     \
})
*/
    // i took the above and optimized it with math to the below
    // there is logic on the above math operations, down below it doesnt make sense

#define NODE_POINT(i, j, k) ((PointDouble){                                                    \
    .x = NetworkRect.left + (RECT_WIDTH(NetworkRect) * (2 * (i) + 1)) / (2 * (nn->count + 1)), \
    .y = NetworkRect.top + (RECT_HEIGHT(NetworkRect) * (2 * (j) + 1)) / (2 * (k)),             \
})

    int nodeRadius = 15;
    int weightLineWidth = 2;

    for (int i = 0; i < nn->count; i++) {
        Matrix *curWeights = &nn->weights[i];
        // print_mat(&curWeights, "BirdWeights", 0, "%f"); // PRINT

        for (int j = 0; j < curWeights->rows; j++) {
            for (int k = 0; k < curWeights->cols; k++) {
                PointDouble nodePointFrom = NODE_POINT(i, j, curWeights->rows);
                PointDouble nodePointTo = NODE_POINT(i + 1, k, nn->layers[i + 1].cols);

                COLORREF weightColor = NetworkWeightColor(MAT_AT(curWeights, j, k));

                HPEN hWeightPen = CreatePen(PS_SOLID, weightLineWidth, weightColor);

                DrawLine(hdc,
                    nodePointFrom.x, nodePointFrom.y,
                    nodePointTo.x, nodePointTo.y,
                    hWeightPen);

                DeleteObject(hWeightPen);
            }

            PointDouble nodePoint = NODE_POINT(i, j, curWeights->rows);
            RECT nodeRect = {
                .top = nodePoint.y - nodeRadius,
                .left = nodePoint.x - nodeRadius,
                .bottom = nodePoint.y + nodeRadius,
                .right = nodePoint.x + nodeRadius,
            };

            if (i > 0) {
                Matrix *curBiases = &nn->biases[i - 1];

                COLORREF biasColor = NetworkBiasColor(MAT_AT(curBiases, 0, j));

                HBRUSH hNodeBrush = CreateSolidBrush(biasColor);
                HBRUSH hOldBrush = SelectObject(hdc, hNodeBrush);

                Ellipse(hdc, nodeRect.left, nodeRect.top, nodeRect.right, nodeRect.bottom);

                SelectObject(hdc, hOldBrush);
                DeleteObject(hNodeBrush);
            } else {
                HBRUSH hOldBrush = SelectObject(hdc, GetStockObject(GRAY_BRUSH));

                Ellipse(hdc, nodeRect.left, nodeRect.top, nodeRect.right, nodeRect.bottom);

                SelectObject(hdc, hOldBrush);
            }
        }
    }
    Matrix *lastBiases = &nn->biases[nn->count - 1];
    for (int i = 0; i < lastBiases->cols; i++) {
        PointDouble nodePoint = NODE_POINT(nn->count, i, lastBiases->cols);
        RECT nodeRect = {
            .top = nodePoint.y - nodeRadius,
            .left = nodePoint.x - nodeRadius,
            .bottom = nodePoint.y + nodeRadius,
            .right = nodePoint.x + nodeRadius,
        };

        COLORREF biasColor = NetworkBiasColor(MAT_AT(lastBiases, 0, i));

        HBRUSH hNodeBrush = CreateSolidBrush(biasColor);
        HBRUSH hOldBrush = SelectObject(hdc, hNodeBrush);

        Ellipse(hdc, nodeRect.left, nodeRect.top, nodeRect.right, nodeRect.bottom);

        SelectObject(hdc, hOldBrush);
        DeleteObject(hNodeBrush);
    }
}

#undef NODE_POINT

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            if ((lParam & 0x40000000) == 0) {
                switch (wParam) {
                    case ' ':
                        BirdFlap = true;
                        break;
                    case 'W':
                        timeSpeedup *= 2;
                        break;
                    case 'S':
                        timeSpeedup /= 2;
                        break;
#ifdef NN
                    case 'N':
                        NewNetwork = true;
                        break;
#endif
                }
                if (wParam >= '0' && wParam <= '9') {
                    timeSpeedup = wParam - '0';
                }
            }
            break;
        case WM_PAINT: {
#ifdef DEBUG_FPS
            framesPerSecond++;
#endif

            PAINTSTRUCT ps;
            HDC hMainDC = BeginPaint(hwnd, &ps);

            RECT ClientRect;
            GetClientRect(hwnd, &ClientRect);

            int clientWidth = RECT_WIDTH(ClientRect);
            int clientHeight = RECT_HEIGHT(ClientRect);

            if (!hDoubleBufferDC || hTempDCWidth != clientWidth || hTempDCHeight != clientHeight) {
                if (hDoubleBufferDC) {
                    SelectObject(hDoubleBufferDC, hSavedBitmap);
                    DeleteObject(hCopyBitmap);
                    DeleteDC(hDoubleBufferDC);
                    hDoubleBufferDC = NULL;
                }

                hDoubleBufferDC = CreateCompatibleDC(hMainDC);
                hCopyBitmap = CreateCompatibleBitmap(hMainDC, clientWidth, clientHeight);
                hSavedBitmap = (HBITMAP)SelectObject(hDoubleBufferDC, hCopyBitmap);

                hTempDCWidth = clientWidth;
                hTempDCHeight = clientHeight;
            }

            {
#ifdef DEBUG_RECTS
                FillRect(hTempDC, &ClientRect, hBackgroundBrush);
#endif // DEBUG_RECTS

                BITMAP *BackgroundBitmapPTR = &BackgroundBitmap;
                RECT BackgroundRect = {
                    .top = 0,
                    .left = 0,
                    .bottom = BackgroundBitmapPTR->bmHeight,
                    .right = BackgroundBitmapPTR->bmWidth,
                };
                double scale = (double)((double)clientHeight / (double)BackgroundBitmapPTR->bmHeight);
                BackgroundRect.bottom = BackgroundRect.top + RECT_HEIGHT(BackgroundRect) * scale;
                BackgroundRect.right = BackgroundRect.left + RECT_WIDTH(BackgroundRect) * scale;

                if (BackgroundPosition.x + RECT_WIDTH(BackgroundRect) <= 0) {
                    BackgroundPosition.x += RECT_WIDTH(BackgroundRect);
                }
                int startPosition = BackgroundPosition.x;
                while (startPosition < clientWidth) {
                    StretchBlt(hDoubleBufferDC,
                        startPosition, BackgroundPosition.y,
                        RECT_WIDTH(BackgroundRect), RECT_HEIGHT(BackgroundRect),
                        hBackgroundDC,
                        0, 0,
                        BackgroundBitmapPTR->bmWidth, BackgroundBitmapPTR->bmHeight,
                        SRCCOPY);

                    startPosition += RECT_WIDTH(BackgroundRect);
                }
            }

            {
                RECT BirdRect = GETBIRDRECT();

#ifdef DEBUG_RECTS
                FillRect(hTempDC, &BirdRect, hBirdBrush);
#endif // DEBUG_RECTS

                RECT BirdBitmapRect = {
                    .top = BirdRect.top,
                    .left = BirdRect.left,
                    .bottom = BirdBitmapRect.top + BirdBitmapPTR->bmHeight,
                    .right = BirdBitmapRect.left + BirdBitmapPTR->bmWidth,
                };
                RECT BirdBitmapCoverRect = RectCoverBigger(BirdBitmapRect, BirdRect);
                BirdBitmapCoverRect = RectCenterTo(BirdBitmapCoverRect, BirdRect);

                int savedDC = SaveDC(hDoubleBufferDC);

                SetGraphicsMode(hDoubleBufferDC, GM_ADVANCED);
                XFORM xform;

                float angle = birdVelocity / 30;
                float radians = DEG2RAD(angle);
                float cosTheta = cos(radians);
                float sinTheta = sin(radians);

                xform.eM11 = cosTheta;
                xform.eM12 = sinTheta;
                xform.eM21 = -sinTheta;
                xform.eM22 = cosTheta;

                int width = RECT_WIDTH(BirdBitmapCoverRect);
                int height = RECT_HEIGHT(BirdBitmapCoverRect);
                xform.eDx = BirdBitmapCoverRect.left + width / 2;
                xform.eDy = BirdBitmapCoverRect.top + height / 2;

                SetWorldTransform(hDoubleBufferDC, &xform);

                COLORREF transparentColor = RGB(255, 255, 255);
                TransparentBlt(
                    hDoubleBufferDC,
                    -width / 2, -height / 2,
                    width, height,
                    hBirdDC,
                    0, 0,
                    BirdBitmapPTR->bmWidth, BirdBitmapPTR->bmHeight,
                    transparentColor);

                RestoreDC(hDoubleBufferDC, savedDC);
            }

            {
                for (ObstacleNode *i = PipesList; i; i = i->next) {
                    RECT topPipe = {
                        .top = ClientRect.top,
                        .left = i->obstacle.x,
                        .bottom = i->obstacle.y,
                        .right = i->obstacle.x + PIPE_WIDTH,
                    };
                    RECT bottomPipe = {
                        .top = topPipe.bottom + PIPE_GAP,
                        .left = i->obstacle.x,
                        .bottom = ClientRect.bottom,
                        .right = i->obstacle.x + PIPE_WIDTH,
                    };

#ifdef DEBUG_RECTS
                    FillRect(hTempDC, &topPipe, hPipeBrush);
                    FillRect(hTempDC, &bottomPipe, hPipeBrush);
#endif // DEBUG_RECTS

                    RECT PipeBottomEndRect = {
                        .top = bottomPipe.top,
                        .left = bottomPipe.left,
                        .bottom = bottomPipe.top + PipeBitmapBottomEnd.bmHeight,
                        .right = bottomPipe.right,
                    };
                    RECT PipeBitmapBottomEndRect = {
                        .top = bottomPipe.top,
                        .left = bottomPipe.left,
                        .bottom = bottomPipe.top + PipeBitmapBottomEnd.bmHeight,
                        .right = bottomPipe.left + PipeBitmapRepeat.bmWidth,
                    };
                    PipeBitmapBottomEndRect = RectCoverBigger(PipeBitmapBottomEndRect, PipeBottomEndRect);
                    PipeBitmapBottomEndRect = RectScaleBy(PipeBitmapBottomEndRect, (double)PipeBitmapBottomEnd.bmWidth / (double)PipeBitmapRepeat.bmWidth);
                    PipeBitmapBottomEndRect = RectHorizontalCenterTo(PipeBitmapBottomEndRect, PipeBottomEndRect);

                    COLORREF transparentColor = RGB(255, 255, 255);
                    SelectObject(hPipeDC, hPipeBitmapBottomEnd);
                    TransparentBlt(hDoubleBufferDC,
                        PipeBitmapBottomEndRect.left, PipeBitmapBottomEndRect.top,
                        RECT_WIDTH(PipeBitmapBottomEndRect), RECT_HEIGHT(PipeBitmapBottomEndRect),
                        hPipeDC,
                        0, 0,
                        PipeBitmapBottomEnd.bmWidth, PipeBitmapBottomEnd.bmHeight,
                        transparentColor);

                    RECT PipeTopEndRect = {
                        .top = topPipe.bottom - PipeBitmapTopEnd.bmHeight,
                        .left = topPipe.left,
                        .bottom = topPipe.bottom,
                        .right = topPipe.right,
                    };
                    RECT PipeBitmapTopEndRect = {
                        .top = topPipe.bottom - PipeBitmapTopEnd.bmHeight,
                        .left = topPipe.left,
                        .bottom = topPipe.bottom,
                        .right = bottomPipe.left + PipeBitmapRepeat.bmWidth,
                    };
                    PipeBitmapTopEndRect = RectCoverBigger(PipeBitmapTopEndRect, PipeTopEndRect);
                    PipeBitmapTopEndRect = RectScaleBy(PipeBitmapTopEndRect, (double)PipeBitmapTopEnd.bmWidth / (double)PipeBitmapRepeat.bmWidth);
                    PipeBitmapTopEndRect = RectHorizontalCenterTo(PipeBitmapTopEndRect, PipeTopEndRect);

                    int origHeight = RECT_HEIGHT(PipeBitmapTopEndRect);
                    PipeBitmapTopEndRect.bottom = topPipe.bottom;
                    PipeBitmapTopEndRect.top = PipeBitmapTopEndRect.bottom - origHeight;

                    SelectObject(hPipeDC, hPipeBitmapTopEnd);
                    TransparentBlt(hDoubleBufferDC,
                        PipeBitmapTopEndRect.left, PipeBitmapTopEndRect.top,
                        RECT_WIDTH(PipeBitmapTopEndRect), RECT_HEIGHT(PipeBitmapTopEndRect),
                        hPipeDC,
                        0, 0,
                        PipeBitmapTopEnd.bmWidth, PipeBitmapTopEnd.bmHeight,
                        transparentColor);
                    RECT PipeBitmapBottomRepeatRect = {
                        .top = PipeBitmapBottomEndRect.bottom,
                        .left = bottomPipe.left,
                        .bottom = bottomPipe.bottom,
                        .right = bottomPipe.right,
                    };

                    SelectObject(hPipeDC, hPipeBitmapRepeat);
                    StretchBlt(hDoubleBufferDC,
                        PipeBitmapBottomRepeatRect.left, PipeBitmapBottomRepeatRect.top,
                        RECT_WIDTH(PipeBitmapBottomRepeatRect), RECT_HEIGHT(PipeBitmapBottomRepeatRect),
                        hPipeDC,
                        0, 0,
                        PipeBitmapRepeat.bmWidth, PipeBitmapRepeat.bmHeight,
                        SRCCOPY);

                    RECT PipeBitmapTopRepeatRect = {
                        .top = topPipe.top,
                        .left = topPipe.left,
                        .bottom = PipeBitmapTopEndRect.top,
                        .right = topPipe.right,
                    };
                    StretchBlt(hDoubleBufferDC,
                        PipeBitmapTopRepeatRect.left, PipeBitmapTopRepeatRect.top,
                        RECT_WIDTH(PipeBitmapTopRepeatRect), RECT_HEIGHT(PipeBitmapTopRepeatRect),
                        hPipeDC,
                        0, 0,
                        PipeBitmapRepeat.bmWidth, PipeBitmapRepeat.bmHeight,
                        SRCCOPY);
                }
            }

            {
                RECT ScoreRect = {
                    .top = 100,
                    .left = 0,
                    .bottom = 200,
                    .right = clientWidth,
                };
                unsigned char ScoreDigits[100] = {0};
                int length = 0;
                int score = PassedPipes;
                if (score == 0) {
                    length = 1;
                    ScoreDigits[0] = 0;
                } else {
                    while (score > 0) {
                        ScoreDigits[length] = score % 10;
                        length++;
                        score /= 10;
                    }
                }
                int scoreWidth = 0;
                for (int i = 0; i < length; i++) {
                    BITMAP *DigitBM = &DigitBitmaps[ScoreDigits[i]];
                    double scale = (double)((double)RECT_HEIGHT(ScoreRect) / (double)DigitBM->bmHeight);
                    scoreWidth += DigitBM->bmWidth * scale;
                }
                ScoreRect.left = (clientWidth - scoreWidth) / 2;
                ScoreRect.right = ScoreRect.left + scoreWidth;
                COLORREF transparentColor = RGB(255, 0, 0); // hard coded
                int widthCounter = ScoreRect.left;
                for (int i = length - 1; i >= 0; i--) {
                    BITMAP *DigitBM = &DigitBitmaps[ScoreDigits[i]];
                    double scale = (double)((double)RECT_HEIGHT(ScoreRect) / (double)DigitBM->bmHeight);
                    SelectObject(hDigitDC, hDigitBitmaps[ScoreDigits[i]]);
                    TransparentBlt(hDoubleBufferDC,
                        widthCounter, ScoreRect.top,
                        DigitBM->bmWidth * scale, DigitBM->bmHeight * scale,
                        hDigitDC,
                        0, 0,
                        DigitBM->bmWidth, DigitBM->bmHeight,
                        transparentColor);
                    widthCounter += DigitBM->bmWidth * scale;
                }
            }

            {
                SetBkColor(hDoubleBufferDC, RGB(0, 255, 0));
                SetBkMode(hDoubleBufferDC, TRANSPARENT);
                SelectObject(hDoubleBufferDC, hDebugFont);
#ifdef DEBUG_FPS
                {
                    RECT FPSRect = {
                        .top = ClientRect.top,
                        .left = ClientRect.left,
                        .bottom = FPSRect.top + 64,
                        .right = ClientRect.right,
                    };
                    char FPSbuffer[20];
                    snprintf(FPSbuffer, sizeof(FPSbuffer), "FPS: %.3lf", countedFPS);
                    DrawText(hDoubleBufferDC, FPSbuffer, -1, &FPSRect, DT_TOP | DT_LEFT);
                }
#endif
                if (timeSpeedup != 1) {
                    RECT TimeRect = {
                        .top = ClientRect.top + 64,
                        .left = ClientRect.left,
                        .bottom = TimeRect.top + 64,
                        .right = ClientRect.right,
                    };
                    /*unsigned*/ char TimeBuffer[100];
                    snprintf(TimeBuffer, sizeof(TimeBuffer), "x%.2lf", timeSpeedup);
                    DrawText(hDoubleBufferDC, TimeBuffer, -1, &TimeRect, DT_TOP | DT_LEFT);
                }
            }

#ifdef NN
            {
                RECT NetworkRect = {
                    .top = ClientRect.top + clientHeight * 0.05,
                    .left = ClientRect.left + clientWidth * 0.55,
                    .bottom = ClientRect.top + clientHeight * 0.4,
                    .right = ClientRect.left + clientWidth * 0.95,
                };

                // TODO: drawing the network every frame is useless because it only changes on death
                DrawNetwork(hDoubleBufferDC, &BirdNN, NetworkRect);
            }
#endif

            BitBlt(hMainDC,
                0, 0,
                clientWidth, clientHeight,
                hDoubleBufferDC,
                0, 0,
                SRCCOPY);

            EndPaint(hwnd, &ps);
        } break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void InitializeAssets() {
    InitSoundEngine();
#ifdef DEBUG_RECTS
    hBackgroundBrush = CreateSolidBrush(RGB(78, 192, 202));
    hBirdBrush = CreateSolidBrush(RGB(255, 255, 0));
    hPipeBrush = CreateSolidBrush(RGB(0, 255, 0));
#endif // DEBUG_RECTS
    hBackgroundBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_BACK_DAY),
        IMAGE_BITMAP,
        0, 0,
        LR_CREATEDIBSECTION);
    GetObject(hBackgroundBitmap, sizeof(BackgroundBitmap), &BackgroundBitmap);

    hBirdBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_BIRD),
        IMAGE_BITMAP,
        0, 0,
        LR_CREATEDIBSECTION);
    hBirdBitmapMidflap = (HBITMAP)LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_BIRD_MIDFLAP),
        IMAGE_BITMAP,
        0, 0,
        LR_CREATEDIBSECTION);
    hBirdBitmapFlap = (HBITMAP)LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_BIRD_FLAP),
        IMAGE_BITMAP,
        0, 0,
        LR_CREATEDIBSECTION);
    GetObject(hBirdBitmap, sizeof(BirdBitmap), &BirdBitmap);
    GetObject(hBirdBitmapMidflap, sizeof(BirdBitmapMidflap), &BirdBitmapMidflap);
    GetObject(hBirdBitmapFlap, sizeof(BirdBitmapFlap), &BirdBitmapFlap);

    hPipeBitmapRepeat = (HBITMAP)LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_PIPE_REPEAT),
        IMAGE_BITMAP,
        0, 0,
        LR_CREATEDIBSECTION);
    hPipeBitmapBottomEnd = (HBITMAP)LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_PIPE_END_BOTTOM),
        IMAGE_BITMAP,
        0, 0,
        LR_CREATEDIBSECTION);
    hPipeBitmapTopEnd = (HBITMAP)LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDB_PIPE_END_TOP),
        IMAGE_BITMAP,
        0, 0,
        LR_CREATEDIBSECTION);

    GetObject(hPipeBitmapTopEnd, sizeof(PipeBitmapTopEnd), &PipeBitmapTopEnd);
    GetObject(hPipeBitmapBottomEnd, sizeof(PipeBitmapBottomEnd), &PipeBitmapBottomEnd);
    GetObject(hPipeBitmapRepeat, sizeof(PipeBitmapRepeat), &PipeBitmapRepeat);
    hDebugFont = CreateFont(
        64,
        0,
        0,
        0,
        FW_NORMAL,
        false,
        false,
        false,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        "Arial");

    for (int i = 0; i < (int)ARR_LEN(hDigitBitmaps); i++) {
        hDigitBitmaps[i] = (HBITMAP)LoadImage(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDB_DIGIT_0 + i),
            IMAGE_BITMAP,
            0, 0,
            LR_CREATEDIBSECTION);
        GetObject(hDigitBitmaps[i], sizeof(DigitBitmaps[i]), &DigitBitmaps[i]);
    }

    hBackgroundDC = CreateCompatibleDC(NULL);
    SelectObject(hBackgroundDC, hBackgroundBitmap);

    hBirdDC = CreateCompatibleDC(NULL);

    hPipeDC = CreateCompatibleDC(NULL);

    hDigitDC = CreateCompatibleDC(NULL);
}

void DeleteAssets() {
    ShutdownSoundEngine();
#ifdef DEBUG_RECTS
    DeleteObject(hBackgroundBrush);
    DeleteObject(hBirdBrush);
    DeleteObject(hPipeBrush);
#endif // DEBUG_RECTS

    DeleteObject(hBackgroundBitmap);

    DeleteObject(hBirdBitmap);
    DeleteObject(hBirdBitmapMidflap);
    DeleteObject(hBirdBitmapFlap);

    DeleteObject(hPipeBitmapBottomEnd);
    DeleteObject(hPipeBitmapTopEnd);
    DeleteObject(hPipeBitmapRepeat);

    DeleteObject(hDebugFont);

    for (int i = 0; i < (int)ARR_LEN(hDigitBitmaps); i++) {
        DeleteObject(hDigitBitmaps[i]);
    }
    if (hDoubleBufferDC) {
        SelectObject(hDoubleBufferDC, hSavedBitmap);
        DeleteObject(hCopyBitmap);
        DeleteDC(hDoubleBufferDC);
    }
    DeleteDC(hBackgroundDC);
    DeleteDC(hBirdDC);
    DeleteDC(hPipeDC);
    DeleteDC(hDigitDC);
}

#ifdef NN
void InitializeNetwork() {
    int layers[] = {2, 8, 8, ACTIONS_AMOUNT};
#    if 1
    ActivationType acts[] = {SIGMOID, SIGMOID};
#    else
#        define acts NULL
#    endif

    BirdNN = NeuralNetwork(layers, ARR_LEN(layers), acts);
    BirdTargetNN = NeuralNetwork(layers, ARR_LEN(layers), acts);
    BirdNNGrad = GradientNetwork(layers, ARR_LEN(layers));
    Network_xavier_init(&BirdNN);
    Network_copy(&BirdTargetNN, &BirdNN);

    memset(&birdSteps, 0, sizeof(birdSteps));
    for (int i = 0; i < STEPS_AMOUNT; i++) {
        birdSteps[i].state = matrix_new(INPUT_MAT);
        birdSteps[i].nextStep = ((i < STEPS_AMOUNT - 1) ? &birdSteps[i + 1] : NULL);
    }
}
#endif

const char g_szClassName[] = "FlappyBirdWindowClass";
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    srand(time(0));

#if defined(DEBUG_CONSOLE)
    AttachConsoleToWindow();
#endif

    InitializeAssets();
#ifdef NN
    InitializeNetwork();
#endif
    // SetTiming();

    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = 0,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
#ifdef DEBUG_RECTS
        .hbrBackground = hBackgroundBrush,
#else  // DEBUG_RECTS
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
#endif // DEBUG_RECTS
        .lpszMenuName = NULL,
        .lpszClassName = g_szClassName,
        .hIconSm = LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR),
    };
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_OK | MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        g_szClassName,
        "Flappy Bird",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
        NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_OK | MB_ICONERROR);
        return 0;
    }
    mainHwnd = hwnd;

    InitializeGame();

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevTime);

    DWORD physicsThreadId;
    HANDLE physicsThread = CreateThread(
        NULL,
        0,
        GameLoop,
        NULL,
        0,
        &physicsThreadId);
    if (!physicsThread) {
        MessageBox(NULL, "Physics Thread Creation Failed!", "Error!", MB_OK | MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    DeleteAssets();

    return Msg.wParam;
}