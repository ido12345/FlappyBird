#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define DEG2RAD(deg) ((deg) * 3.14159265358979323846 / 180.0)

#include <stdio.h>
void AttachConsoleToWindow()
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stderr);
}

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "resources.h"

#include "ML.h"

#define NN
#define DEBUG_FPS

#ifdef NN
Network *BirdNN;
#define RANGE_DISTANCE 100
#ifdef RANGE_DISTANCE
#define RAND_RANGE -RANGE_DISTANCE, RANGE_DISTANCE
#else
#define RAND_RANGE 0, 0.5
#endif

Network *BirdNNGrad;
Network *BirdTargetNN;
double learnRate = 1;

const double PassPipeReward = 10;
const double DeathReward = -5;

#define STEPS_AMOUNT 5000

int stepIndex = 0;
int stepCounter = 0;
int gameCounter = 0;
Step *birdSteps[STEPS_AMOUNT];

#define INPUT_MAT 1, 2

BOOL NewNetwork = FALSE;
#endif // NN
double timeSpeedup = 1;

HDC hTempDC = NULL;
HBITMAP hCopyBitmap = NULL;
HBITMAP hSavedBitmap = NULL;
int hTempDCWidth = 0, hTempDCHeight = 0;

HDC BackgroundDC = NULL;

HDC BirdDC = NULL;

HDC PipeDC = NULL;

HDC hDigitDC = NULL;

HBRUSH hBackgroundBrush;
HBRUSH hBirdBrush;
HBRUSH hPipeBrush;

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

ma_engine SoundEngine;

void InitSoundEngine()
{
    ma_engine_init(NULL, &SoundEngine);
}

void ShutdownSoundEngine()
{
    ma_engine_uninit(&SoundEngine);
}

const void *LoadResourceData(UINT resourceID, LPCSTR resourceType, DWORD *size)
{
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceID), resourceType);
    if (!hResource)
        return NULL;

    HGLOBAL hGlobal = LoadResource(NULL, hResource);
    if (!hGlobal)
        return NULL;

    *size = SizeofResource(NULL, hResource);
    return LockResource(hGlobal);
}

void PlayMemorySound(UINT resourceID)
{
    if (timeSpeedup > 10)
        return;

    DWORD dataSize = 0;
    const void *data = LoadResourceData(resourceID, "WAV", &dataSize);

    if (!data || !dataSize)
    {
        MessageBox(NULL, "Failed to load sound resource.", "Error!", MB_OK | MB_ICONERROR);
        return;
    }

    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    ma_decoder *decoder = malloc(sizeof(ma_decoder));
    if (ma_decoder_init_memory(data, dataSize, &decoderConfig, decoder) != MA_SUCCESS)
    {
        free(decoder);
        return;
    }

    ma_sound *sound = malloc(sizeof(ma_sound));
    if (ma_sound_init_from_data_source(&SoundEngine, decoder, MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_DECODE, NULL, sound) != MA_SUCCESS)
    {
        ma_decoder_uninit(decoder);
        free(decoder);
        free(sound);
        return;
    }

    ma_sound_start(sound);
}

#define SCREEN_HEIGHT 1000
#define SCREEN_WIDTH 600
#define BIRD_HEIGHT 50
#define BIRD_WIDTH 60
// #define DELTA_TIME 0.01
#define DISTANCE_RATIO 200 // 377.95 // (10 cm) in (pixels)

// make the bird jump x times his height
#define FLAP_HEIGHT (BIRD_HEIGHT * 2.5)

// distance between 2 pipes
#define PIPE_DISTANCE 400
#define PIPE_GAP 200
#define PIPE_WIDTH 100

#define RECT_WIDTH(rect) (((rect).right) - ((rect).left))
#define RECT_HEIGHT(rect) (((rect).bottom) - ((rect).top))

#define GETBIRDRECT() {                                 \
    .top = (long)(BirdPosition.y) - BIRD_HEIGHT / 2,    \
    .left = (long)(BirdPosition.x) - BIRD_WIDTH / 2,    \
    .bottom = (long)(BirdPosition.y) + BIRD_HEIGHT / 2, \
    .right = (long)(BirdPosition.x) + BIRD_WIDTH / 2,   \
};

typedef struct PointDouble
{
    double x;
    double y;
} PointDouble;

typedef struct ObstacleList
{
    PointDouble *obstacle;
    struct ObstacleList *next;
} ObstacleList;

HWND mainHwnd;

PointDouble BirdPosition = {
    .x = min(SCREEN_WIDTH / 2, 360),
    .y = SCREEN_HEIGHT / 2,
};
ObstacleList *PipesList = NULL;
ObstacleList *NextObstacle = NULL;
int PassedPipes = 0;
const double gravity = 10;
double birdVelocity = 0;
double pipeVelocity = 225;

double backgroundVelocity = 45;
PointDouble BackgroundPosition = {
    .x = 0,
    .y = 0,
};

BOOL BirdFlap = FALSE;

BOOL RectCollision(RECT r1, RECT r2)
{
    return ((r2.bottom > r1.top) && (r1.bottom > r2.top) && // veritical alignment
            (r2.right > r1.left) && (r1.right > r2.left));  // horizontal alignment
}

// r1 is a rectangle smaller than r2, this will scale r1 and save its aspect ratio so that it fits into r2
RECT RectFitBigger(RECT r1, RECT r2)
{
    if (RECT_WIDTH(r1) > RECT_WIDTH(r2) && RECT_HEIGHT(r1) > RECT_HEIGHT(r2))
    {
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
RECT RectCoverBigger(RECT r1, RECT r2)
{
    if (RECT_WIDTH(r1) > RECT_WIDTH(r2) && RECT_HEIGHT(r1) > RECT_HEIGHT(r2))
    {
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

RECT RectScaleBy(RECT r1, double scaleBy)
{
    r1.right = r1.left + RECT_WIDTH(r1) * scaleBy;
    r1.bottom = r1.top + RECT_HEIGHT(r1) * scaleBy;
    return r1;
}

RECT RectHorizontalCenterTo(RECT r1, RECT r2)
{
    int xCenter = (r2.right + r2.left) / 2;

    int originalWidth = RECT_WIDTH(r1);

    r1.left = xCenter - originalWidth / 2;
    r1.right = r1.left + originalWidth;

    return r1;
}

RECT RectVeritcalCenterTo(RECT r1, RECT r2)
{
    int yCenter = (r2.top + r2.bottom) / 2;

    int originalHeight = RECT_HEIGHT(r1);

    r1.top = yCenter - originalHeight / 2;
    r1.bottom = r1.top + originalHeight;

    return r1;
}

// puts r1 in the center of r2
RECT RectCenterTo(RECT r1, RECT r2)
{
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

BOOL BirdPipeCollision()
{
    RECT ClientRect;
    GetClientRect(mainHwnd, &ClientRect);

    for (ObstacleList *i = PipesList; i && i->obstacle; i = i->next)
    {
        RECT topPipe = {
            .top = 0,
            .left = i->obstacle->x,
            .bottom = i->obstacle->y,
            .right = i->obstacle->x + PIPE_WIDTH,
        };
        RECT bottomPipe = {
            .top = i->obstacle->y + PIPE_GAP,
            .left = i->obstacle->x,
            .bottom = ClientRect.bottom,
            .right = i->obstacle->x + PIPE_WIDTH,
        };

        RECT BirdRect = GETBIRDRECT();
        if (RectCollision(BirdRect, topPipe) || RectCollision(BirdRect, bottomPipe))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL BirdWindowCollision()
{
    RECT ClientRect;
    GetClientRect(mainHwnd, &ClientRect);

    if (BirdPosition.y + BIRD_HEIGHT / 2 > ClientRect.bottom)
        return TRUE;
    if (BirdPosition.y - BIRD_HEIGHT / 2 < ClientRect.top)
        return TRUE;
    if (BirdPosition.x + BIRD_WIDTH / 2 > ClientRect.right)
        return TRUE;
    if (BirdPosition.x - BIRD_WIDTH / 2 < ClientRect.left)
        return TRUE;
    return FALSE;
}

void freeObstacleList(ObstacleList *head)
{
    if (!head)
        return;
    freeObstacleList(head->next);
    if (head->obstacle)
    {
        GlobalFree(head->obstacle);
        head->obstacle = NULL;
    }
    if (head->next)
    {
        GlobalFree(head->next);
        head->next = NULL;
    }
    GlobalFree(head);
}

ObstacleList *NewObstacleList(PointDouble data)
{
    ObstacleList *temp = (ObstacleList *)GlobalAlloc(GMEM_FIXED, sizeof(*temp));
    temp->obstacle = (PointDouble *)GlobalAlloc(GMEM_FIXED, sizeof(PointDouble));
    temp->obstacle->x = data.x;
    temp->obstacle->y = data.y;
    temp->next = NULL;
    return temp;
}

void CheckPassedPipeObstacle()
{
    if (!NextObstacle || !NextObstacle->obstacle)
    {
        NextObstacle = PipesList;
        return;
    }
    if ((NextObstacle->obstacle->x + PIPE_WIDTH) < (BirdPosition.x - BIRD_WIDTH / 2))
    {
        PassedPipes++;
        NextObstacle = NextObstacle->next;
        PlayMemorySound(IDS_POINT);
#ifdef NN
        birdSteps[stepIndex]->reward = PassPipeReward;
#endif
    }
}

void CheckNewPipeObstacle()
{
    if (!PipesList)
    {
        RECT ClientRect;
        GetClientRect(mainHwnd, &ClientRect);
        int obstacleY = rand_int(ClientRect.top + 10, ClientRect.bottom - 10 - PIPE_GAP);
        PipesList = NewObstacleList((PointDouble){ClientRect.right, obstacleY});
        return;
    }
    ObstacleList *last = PipesList;
    while (last && last->next)
    {
        last = last->next;
    }
    if (last)
    {
        RECT ClientRect;
        GetClientRect(mainHwnd, &ClientRect);
        if (last->obstacle->x + PIPE_DISTANCE <= ClientRect.right)
        {
            int obstacleY = rand_int(ClientRect.top + 10, ClientRect.bottom - 10 - PIPE_GAP);
            last->next = NewObstacleList((PointDouble){ClientRect.right, obstacleY});
        }
    }
}

void CheckClearPipeObstacle()
{
    ObstacleList *first = PipesList;
    if (first && first->obstacle->x + PIPE_WIDTH <= 0)
    {
        PipesList = PipesList->next;
        GlobalFree(first->obstacle);
        GlobalFree(first);
    }
}

void PhysicsStep(double deltaTime)
{
    double prevVelocity = birdVelocity;
    birdVelocity += gravity * deltaTime * DISTANCE_RATIO;
    double deltaY = (birdVelocity + prevVelocity) * deltaTime / 2;
    BirdPosition.y += deltaY;

    for (ObstacleList *i = PipesList; i && i->obstacle; i = i->next)
    {
        i->obstacle->x -= pipeVelocity * deltaTime;
    }

    BackgroundPosition.x -= backgroundVelocity * deltaTime;
}

LARGE_INTEGER frequency;
LARGE_INTEGER prevPhysicsTime;
LARGE_INTEGER prevFlapTime;

void CheckBirdFlap()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    double elapsedTimeSinceFlap = (double)(currentTime.QuadPart - prevFlapTime.QuadPart) / frequency.QuadPart;
    if (elapsedTimeSinceFlap < 0.1)
    {
        BirdBitmapPTR = &BirdBitmapFlap;
        SelectObject(BirdDC, hBirdBitmapFlap);
    }
    else if (elapsedTimeSinceFlap < 0.2)
    {
        BirdBitmapPTR = &BirdBitmapMidflap;
        SelectObject(BirdDC, hBirdBitmapMidflap);
    }
    else
    {
        BirdBitmapPTR = &BirdBitmap;
        SelectObject(BirdDC, hBirdBitmap);
    }
}

double TargetFPS = 200.0; // Frames Per Second
double TargetPPS = 200.0; // Physics Per Second
double accumulatedPhysicsTime = 0;
double accumulatedRenderTime = 0;
double physicsDelta;
double renderDelta;
double countedFPS = 0;

void SetTiming()
{
    physicsDelta = (1.0 / TargetPPS);
    renderDelta = (1.0 / TargetFPS);
}

void InitializeGame()
{
    RECT ClientRect;
    GetClientRect(mainHwnd, &ClientRect);

    birdVelocity = 0;
    BirdFlap = FALSE;
    BirdPosition.x = min(RECT_WIDTH(ClientRect) / 2, 360),
    BirdPosition.y = RECT_HEIGHT(ClientRect) / 2;

    if (PipesList)
    {
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
    SelectObject(BirdDC, hBirdBitmap);
    prevFlapTime.QuadPart = 0;
}

void GameOver()
{
    InitializeGame();
}

void calc_QTargets(Network *TargetNN, Matrix *QTargets, Step *steps[], int *indexes)
{
    float gamma = 0.99;
    for (int i = 0; i < QTargets->rows; i++)
    {
        int curRandIdx = indexes[i];
        if (steps[curRandIdx]->death == false)
        {
            mat_copy(NETWORK_IN(TargetNN), steps[curRandIdx + 1]->state);
            Network_forward(TargetNN);

            float maxQ = MAT_AT(NETWORK_OUT(TargetNN), 0, 0);
            for (int j = 1; j < NETWORK_OUT(TargetNN)->cols; j++)
            {
                if (MAT_AT(NETWORK_OUT(TargetNN), 0, j) > maxQ)
                {
                    maxQ = MAT_AT(NETWORK_OUT(TargetNN), 0, j);
                }
            }
            MAT_AT(QTargets, i, 0) = steps[curRandIdx]->reward + (gamma * maxQ);
        }
        else // steps[curRandIdx]->death = true
        {
            MAT_AT(QTargets, i, 0) = steps[curRandIdx]->reward;
        }
    }
}

#ifdef NN
void ReinforcementLearning()
{
    // PRINT_NETWORK(BirdNN);

    for (int batching = 0; batching < 1; batching++)
    {
        int batchSize = 32;
        batchSize = min(stepCounter - 1, batchSize);

        // int batchIndexes[batchSize] = {-1};
        int *batchIndexes = (int *)malloc(sizeof(*batchIndexes) * batchSize);

        for (int i = 0; i < batchSize; i++)
        {
            int randIdx;
        NewIdx:
            randIdx = rand_int(0, stepCounter - 2);
            for (int j = 0; j < i; j++)
            {
                if (batchIndexes[j] == randIdx)
                {
                    goto NewIdx;
                }
            }
            batchIndexes[i] = randIdx;
        }

        Matrix *QTargets = mat_alloc(batchSize, 1);

        calc_QTargets(BirdTargetNN, QTargets, birdSteps, batchIndexes);

        // PRINT_MAT(QTargets);

        Network_Q_backprop(BirdNN, BirdNNGrad, QTargets, birdSteps, batchIndexes);

        // print_Network((BirdNNGrad), "BirdNNGrad", true);

        Network_gradient_descent(BirdNN, BirdNNGrad, learnRate);

        mat_destroy(QTargets);
    }

    // printf("Rate: %f\n", learnRate);
    // printf("Epsilon: %f\n", epsilon);
    // printf("Step Index: %d\n", stepIndex);
    // print_Network((BirdNN), "BirdNN", true);

    if (gameCounter % 10 == 0)
    {
        Network_copy(BirdTargetNN, BirdNN);
    }
}
#endif

#ifdef DEBUG_FPS
unsigned long long framesPerSecond = 0;
LARGE_INTEGER lastFramesCheck;
#endif

DWORD WINAPI GameLoop(LPVOID lpParam)
{
    (void)lpParam;

    while (1)
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        double elapsedTime = (double)(currentTime.QuadPart - prevPhysicsTime.QuadPart) / frequency.QuadPart;
        prevPhysicsTime = currentTime;

        accumulatedPhysicsTime += elapsedTime * timeSpeedup;
        accumulatedRenderTime += elapsedTime * timeSpeedup;

#ifdef DEBUG_FPS
        double elapsedTimeSinceFrames = (double)(currentTime.QuadPart - lastFramesCheck.QuadPart) / frequency.QuadPart;
        if (elapsedTimeSinceFrames >= 1)
        {
            countedFPS = (double)framesPerSecond / elapsedTimeSinceFrames;
            lastFramesCheck = currentTime;
            framesPerSecond = 0;
        }
#endif

        while (accumulatedPhysicsTime >= physicsDelta)
        {
#ifdef AUTOPILOT
            if (NextObstacle)
            {
                if (BirdPosition.y + BIRD_HEIGHT >= NextObstacle->obstacle->y + PIPE_GAP + 10)
                {
                    BirdFlap = TRUE;
                }
            }
#endif

#ifdef NN
            if (NewNetwork == TRUE)
            {
                Network_rand(BirdNN, RAND_RANGE);
                NewNetwork = FALSE;
            }

            stepCounter++;
            if (stepCounter > STEPS_AMOUNT)
                stepCounter = STEPS_AMOUNT;

            RECT ClientRect;
            GetClientRect(mainHwnd, &ClientRect);

            Matrix *netIn = mat_alloc(INPUT_MAT);
            MAT_AT(netIn, 0, 0) = BirdPosition.y / ClientRect.bottom;
            MAT_AT(netIn, 0, 1) = NextObstacle->obstacle->y / ClientRect.bottom;

            mat_copy(NETWORK_IN(BirdNN), netIn);

            Network_forward(BirdNN);

            if (MAT_AT(NETWORK_OUT(BirdNN), 0, 0) > MAT_AT(NETWORK_OUT(BirdNN), 0, 1))
            {
                BirdFlap = TRUE;
                birdSteps[stepIndex]->action = 0;
                birdSteps[stepIndex]->output = MAT_AT(NETWORK_OUT(BirdNN), 0, 0);
            }
            else
            {
                birdSteps[stepIndex]->action = 1;
                birdSteps[stepIndex]->output = MAT_AT(NETWORK_OUT(BirdNN), 0, 1);
            }
            mat_copy(birdSteps[stepIndex]->state, netIn);

            mat_destroy(netIn);
#endif

            if (BirdFlap == TRUE)
            {
                prevFlapTime = currentTime;
                birdVelocity = -sqrt(2 * gravity * DISTANCE_RATIO * FLAP_HEIGHT);
                BirdFlap = FALSE;
#if !defined(AUTOPILOT) && !defined(NN)
                PlayMemorySound(IDS_FLAP);
#endif
            }
            PhysicsStep(physicsDelta);
            CheckClearPipeObstacle();
            CheckNewPipeObstacle();
            CheckPassedPipeObstacle();
            CheckBirdFlap();

            if (BirdWindowCollision() || BirdPipeCollision())
            {
                PlayMemorySound(IDS_DIE);
#ifdef NN
                birdSteps[stepIndex]->death = TRUE;
                birdSteps[stepIndex]->reward = DeathReward;

                gameCounter++;
                ReinforcementLearning();
#endif
                GameOver();
            }
#ifdef NN
            stepIndex++;
            if (stepIndex >= STEPS_AMOUNT)
            {
                stepIndex = 0;
            }
#endif

            accumulatedPhysicsTime -= physicsDelta;
        }

        if (accumulatedRenderTime >= renderDelta)
        {
            accumulatedRenderTime -= renderDelta;
            InvalidateRect(mainHwnd, NULL, FALSE);
            UpdateWindow(mainHwnd);
        }
        // UpdateWindow(mainHwnd);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if ((lParam & 0x40000000) == 0)
        {
            switch (wParam)
            {
            case ' ':
                BirdFlap = TRUE;
                break;
            case 'W':
                timeSpeedup *= 2;
                break;
            case 'S':
                timeSpeedup /= 2;
                break;
            case 'N':
                NewNetwork = TRUE;
                break;
            }
            if (wParam <= '9' && wParam >= '0')
            {
                timeSpeedup = wParam - '0';
            }
        }
        break;
    case WM_PAINT:
    {
#ifdef DEBUG_FPS
        framesPerSecond++;
#endif

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT ClientRect;
        GetClientRect(hwnd, &ClientRect);

        int clientWidth = RECT_WIDTH(ClientRect);
        int clientHeight = RECT_HEIGHT(ClientRect);

        if (!hTempDC || hTempDCWidth != clientWidth || hTempDCHeight != clientHeight)
        {
            if (hTempDC)
            {
                SelectObject(hTempDC, hSavedBitmap);
                DeleteObject(hCopyBitmap);
                DeleteDC(hTempDC);
                hTempDC = NULL;
            }

            hTempDC = CreateCompatibleDC(hdc);
            hCopyBitmap = CreateCompatibleBitmap(hdc, clientWidth, clientHeight);
            hSavedBitmap = (HBITMAP)SelectObject(hTempDC, hCopyBitmap);

            hTempDCWidth = clientWidth;
            hTempDCHeight = clientHeight;
        }

        {
#ifdef DEBUG_RECTS
            FillRect(hTempDC, &ClientRect, hBackgroundBrush);
#endif

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

            if (BackgroundPosition.x + RECT_WIDTH(BackgroundRect) <= 0)
            {
                BackgroundPosition.x += RECT_WIDTH(BackgroundRect);
            }
            int startPosition = BackgroundPosition.x;
            while (startPosition < clientWidth)
            {
                StretchBlt(hTempDC,
                           startPosition, BackgroundPosition.y,
                           RECT_WIDTH(BackgroundRect), RECT_HEIGHT(BackgroundRect),
                           BackgroundDC,
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
#endif

            RECT BirdBitmapRect = {
                .top = BirdRect.top,
                .left = BirdRect.left,
                .bottom = BirdBitmapRect.top + BirdBitmapPTR->bmHeight,
                .right = BirdBitmapRect.left + BirdBitmapPTR->bmWidth,
            };
            RECT BirdBitmapCoverRect = RectCoverBigger(BirdBitmapRect, BirdRect);
            BirdBitmapCoverRect = RectCenterTo(BirdBitmapCoverRect, BirdRect);

            int savedDC = SaveDC(hTempDC);

            SetGraphicsMode(hTempDC, GM_ADVANCED);
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

            SetWorldTransform(hTempDC, &xform);

            COLORREF transparentColor = RGB(255, 255, 255);
            TransparentBlt(
                hTempDC,
                -width / 2, -height / 2,
                width, height,
                BirdDC,
                0, 0,
                BirdBitmapPTR->bmWidth, BirdBitmapPTR->bmHeight,
                transparentColor);

            RestoreDC(hTempDC, savedDC);
        }

        {
            for (ObstacleList *i = PipesList; i && i->obstacle; i = i->next)
            {
                RECT topPipe = {
                    .top = 0,
                    .left = i->obstacle->x,
                    .bottom = i->obstacle->y,
                    .right = i->obstacle->x + PIPE_WIDTH,
                };
                RECT bottomPipe = {
                    .top = i->obstacle->y + PIPE_GAP,
                    .left = i->obstacle->x,
                    .bottom = clientHeight,
                    .right = i->obstacle->x + PIPE_WIDTH,
                };

#ifdef DEBUG_RECTS
                FillRect(hTempDC, &topPipe, hPipeBrush);
                FillRect(hTempDC, &bottomPipe, hPipeBrush);
#endif

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
                SelectObject(PipeDC, hPipeBitmapBottomEnd);
                TransparentBlt(hTempDC,
                               PipeBitmapBottomEndRect.left, PipeBitmapBottomEndRect.top,
                               RECT_WIDTH(PipeBitmapBottomEndRect), RECT_HEIGHT(PipeBitmapBottomEndRect),
                               PipeDC,
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

                SelectObject(PipeDC, hPipeBitmapTopEnd);
                TransparentBlt(hTempDC,
                               PipeBitmapTopEndRect.left, PipeBitmapTopEndRect.top,
                               RECT_WIDTH(PipeBitmapTopEndRect), RECT_HEIGHT(PipeBitmapTopEndRect),
                               PipeDC,
                               0, 0,
                               PipeBitmapTopEnd.bmWidth, PipeBitmapTopEnd.bmHeight,
                               transparentColor);
                RECT PipeBitmapBottomRepeatRect = {
                    .top = PipeBitmapBottomEndRect.bottom,
                    .left = bottomPipe.left,
                    .bottom = bottomPipe.bottom,
                    .right = bottomPipe.right,
                };

                SelectObject(PipeDC, hPipeBitmapRepeat);
                StretchBlt(hTempDC,
                           PipeBitmapBottomRepeatRect.left, PipeBitmapBottomRepeatRect.top,
                           RECT_WIDTH(PipeBitmapBottomRepeatRect), RECT_HEIGHT(PipeBitmapBottomRepeatRect),
                           PipeDC,
                           0, 0,
                           PipeBitmapRepeat.bmWidth, PipeBitmapRepeat.bmHeight,
                           SRCCOPY);

                RECT PipeBitmapTopRepeatRect = {
                    .top = topPipe.top,
                    .left = topPipe.left,
                    .bottom = PipeBitmapTopEndRect.top,
                    .right = topPipe.right,
                };
                StretchBlt(hTempDC,
                           PipeBitmapTopRepeatRect.left, PipeBitmapTopRepeatRect.top,
                           RECT_WIDTH(PipeBitmapTopRepeatRect), RECT_HEIGHT(PipeBitmapTopRepeatRect),
                           PipeDC,
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
            if (score == 0)
            {
                length = 1;
                ScoreDigits[0] = 0;
            }
            else
            {
                while (score > 0)
                {
                    ScoreDigits[length] = score % 10;
                    length++;
                    score /= 10;
                }
            }
            int scoreWidth = 0;
            for (int i = 0; i < length; i++)
            {
                BITMAP *DigitBM = &DigitBitmaps[ScoreDigits[i]];
                double scale = (double)((double)RECT_HEIGHT(ScoreRect) / (double)DigitBM->bmHeight);
                scoreWidth += DigitBM->bmWidth * scale;
            }
            ScoreRect.left = (clientWidth - scoreWidth) / 2;
            ScoreRect.right = ScoreRect.left + scoreWidth;
            COLORREF transparentColor = RGB(255, 0, 0); // hard coded
            int widthCounter = ScoreRect.left;
            for (int i = length - 1; i >= 0; i--)
            {
                BITMAP *DigitBM = &DigitBitmaps[ScoreDigits[i]];
                double scale = (double)((double)RECT_HEIGHT(ScoreRect) / (double)DigitBM->bmHeight);
                SelectObject(hDigitDC, hDigitBitmaps[ScoreDigits[i]]);
                TransparentBlt(hTempDC,
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
            SetBkColor(hTempDC, RGB(0, 255, 0));
            SetBkMode(hTempDC, TRANSPARENT);
            SelectObject(hTempDC, hDebugFont);
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
                DrawText(hTempDC, FPSbuffer, -1, &FPSRect, DT_TOP | DT_LEFT);
            }
#endif
            if (timeSpeedup != 1)
            {
                RECT TimeRect = {
                    .top = ClientRect.top + 64,
                    .left = ClientRect.left,
                    .bottom = TimeRect.top + 64,
                    .right = ClientRect.right,
                };
                char TimeBuffer[20];
                snprintf(TimeBuffer, sizeof(TimeBuffer), "x%.2lf", timeSpeedup);
                DrawText(hTempDC, TimeBuffer, -1, &TimeRect, DT_TOP | DT_LEFT);
            }
        }

        BitBlt(hdc,
               0, 0,
               clientWidth, clientHeight,
               hTempDC,
               0, 0,
               SRCCOPY);

        EndPaint(hwnd, &ps);
    }
    break;
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

void InitializeAssets()
{
    InitSoundEngine();
    hBackgroundBrush = CreateSolidBrush(RGB(78, 192, 202));
    hBirdBrush = CreateSolidBrush(RGB(255, 255, 0));
    hPipeBrush = CreateSolidBrush(RGB(0, 255, 0));
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
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        "Arial");

    for (int i = 0; i < (int)ARR_LEN(hDigitBitmaps); i++)
    {
        hDigitBitmaps[i] = (HBITMAP)LoadImage(GetModuleHandle(NULL),
                                              MAKEINTRESOURCE(IDB_DIGIT_0 + i),
                                              IMAGE_BITMAP,
                                              0, 0,
                                              LR_CREATEDIBSECTION);
        GetObject(hDigitBitmaps[i], sizeof(DigitBitmaps[i]), &DigitBitmaps[i]);
    }

    BackgroundDC = CreateCompatibleDC(NULL);
    SelectObject(BackgroundDC, hBackgroundBitmap);

    BirdDC = CreateCompatibleDC(NULL);

    PipeDC = CreateCompatibleDC(NULL);

    hDigitDC = CreateCompatibleDC(NULL);
}

void DeleteAssets()
{
    ShutdownSoundEngine();
    DeleteObject(hBackgroundBrush);
    DeleteObject(hBirdBrush);
    DeleteObject(hPipeBrush);

    DeleteObject(hBackgroundBitmap);

    DeleteObject(hBirdBitmap);
    DeleteObject(hBirdBitmapMidflap);
    DeleteObject(hBirdBitmapFlap);

    DeleteObject(hPipeBitmapBottomEnd);
    DeleteObject(hPipeBitmapTopEnd);
    DeleteObject(hPipeBitmapRepeat);

    DeleteObject(hDebugFont);

    for (int i = 0; i < (int)ARR_LEN(hDigitBitmaps); i++)
    {
        DeleteObject(hDigitBitmaps[i]);
    }
    if (hTempDC)
    {
        SelectObject(hTempDC, hSavedBitmap);
        DeleteObject(hCopyBitmap);
        DeleteDC(hTempDC);
    }
    DeleteDC(BackgroundDC);
    DeleteDC(BirdDC);
    DeleteDC(PipeDC);
    DeleteDC(hDigitDC);
}

#ifdef NN
void InitializeNetwork()
{
    int layers[] = {2, 2};
    ActivationType acts[] = {SIGMOID};

    BirdNN = NeuralNetwork(layers, ARR_LEN(layers), acts);
    BirdTargetNN = NeuralNetwork(layers, ARR_LEN(layers), acts);
    BirdNNGrad = GradientNetwork(layers, ARR_LEN(layers));
    Network_rand(BirdNN, RAND_RANGE);
    Network_copy(BirdTargetNN, BirdNN);

    for (int i = 0; i < STEPS_AMOUNT; i++)
    {
        birdSteps[i] = (Step *)calloc(sizeof(*birdSteps[i]), 1);
        birdSteps[i]->state = mat_alloc(INPUT_MAT);
    }
}
#endif

const char g_szClassName[] = "FlappyBirdWindowClass";
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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
    SetTiming();

    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = 0,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = hBackgroundBrush,
        .lpszMenuName = NULL,
        .lpszClassName = g_szClassName,
        .hIconSm = LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR),
    };
    if (!RegisterClassEx(&wc))
    {
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
    if (!hwnd)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_OK | MB_ICONERROR);
        return 0;
    }
    mainHwnd = hwnd;

    InitializeGame();

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevPhysicsTime);

    DWORD physicsThreadId;
    HANDLE physicsThread = CreateThread(
        NULL,
        0,
        GameLoop,
        NULL,
        0,
        &physicsThreadId);
    if (!physicsThread)
    {
        MessageBox(NULL, "Physics Thread Creation Failed!", "Error!", MB_OK | MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    DeleteAssets();

    return Msg.wParam;
}